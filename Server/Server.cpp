#include "server.h"
#include "messages.h"
#include "load_balancer.h"

#include <WS2tcpip.h>
#pragma push_macro("ERROR")
#undef ERROR

#include <iostream>

Server::Server(std::string ip, const int port, const int threadPoolSize, std::string logFile) :
    ip(ip),
    port(port),
    threadPoolSize(threadPoolSize),
	logger(logFile),
    loadBalancer(threadInfos) {
		listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listenSocket == INVALID_SOCKET) {
			logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when creating listening socket");
			return;
		}
        
		listenSocketAddress.sin_family = AF_INET;
		listenSocketAddress.sin_port = htons(port);
		std::wstring ipStr{ip.begin(), ip.end()};
		InetPton(AF_INET, ipStr.c_str(), &listenSocketAddress.sin_addr.s_addr);
		if (bind(listenSocket, reinterpret_cast<SOCKADDR*>(&listenSocketAddress), sizeof(listenSocketAddress)) == SOCKET_ERROR) {
			logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when binding listening socket");
		}
	}

void Server::open() {
    if (listen(listenSocket, SOMAXCONN)) {
        logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when starting listening");
        return;
    }
    initThreadPool();

    while (true) {
        SOCKET newConnection = accept(listenSocket, nullptr, nullptr);
        if (newConnection == INVALID_SOCKET) {
            logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when accepting new connection");
            closesocket(newConnection);
            continue;
        }
        sync(newConnection);
        ThreadMapIterator threadInfo;
        {
            std::scoped_lock lock{threadInfosLock};
            threadInfo = loadBalancer.select();
            FD_SET(newConnection, &threadInfo->second.clients);
        }
        int sendBytes= send(threadInfo->second.notifier, "", 1, 0);
        if (sendBytes < 0) {
            logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when notifying thread ", threadInfo->first, " about new connection");
            continue;
        }
        logger.log(logs::Level::DEBUG, "Connection ", newConnection, " has been forwarded to thread ", threadInfo->first);
    }
    return;
}

void Server::sync(SOCKET dst) {
    // TODO Support for messages sent in chunks, cause 4096 buffer can be easly overflowed
    std::string text = doc.getText();
    if (text.empty()) {
        return;
    }
    msg::Sync msg{1, 0, text};
    msg::Buffer sendBuff{ 4096 };
    msg.serializeTo(sendBuff);
    int sendBytes = send(dst, sendBuff.get(), sendBuff.size, 0);
    if (sendBytes < 0) {
        closesocket(dst);
        shutdown(dst, SD_SEND);
        logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error on document synchronization with ", dst);
        return;
    }
    logger.log(logs::Level::DEBUG, "Document has been synchronized with ", dst);
}

void Server::initThreadPool() {
    std::scoped_lock lock{threadInfosLock};
    for (int i = 0; i < threadPoolSize; i++) {
        std::thread worker{&Server::handleConnection, this};
        auto notifySocket = accept(listenSocket, nullptr, nullptr);
        if (notifySocket == INVALID_SOCKET) {
            closesocket(notifySocket);
            logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error on opening notify socket to thread ", worker.get_id());
            continue;
        }
        FD_SET workerClients;
        FD_ZERO(&workerClients);
        auto [it, newOne] = threadInfos.emplace(worker.get_id(), ThreadInfo{ std::move(workerClients), INVALID_SOCKET, INVALID_SOCKET });
        it->second.notifier = notifySocket;
        threads.emplace_back(std::move(worker));
    }
    logger.log(logs::Level::DEBUG, "Created ", threads.size(), " threads");
}

void Server::removeThread() {
    std::scoped_lock lock{threadInfosLock};
    threadInfos.erase(std::this_thread::get_id());
}

void Server::handleConnection() {
    // Create a communication pipe to master
    SOCKET notifyListenerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (notifyListenerSocket == INVALID_SOCKET) {
        logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error on creating notify listener socket in thread ", std::this_thread::get_id());
        closesocket(notifyListenerSocket);
        return removeThread();
    }

    // Connect to master
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    std::wstring ipStr{ip.begin(), ip.end()};
    InetPton(AF_INET, ipStr.c_str(), &address.sin_addr.s_addr);
    if (connect(notifyListenerSocket, reinterpret_cast<SOCKADDR*>(&address), sizeof(address))) {
        logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when connecting thread ", std::this_thread::get_id(), " to master");
        return removeThread();
    }
    {
        std::scoped_lock lock{threadInfosLock};
        auto& threadInfo = threadInfos[std::this_thread::get_id()];
        FD_SET(notifyListenerSocket, &threadInfo.clients);
        threadInfo.notifyListener = notifyListenerSocket;
    }
    while (true) {
        // Copy FD_SET and then select
        FD_SET threadClients;
        {
            std::scoped_lock lock{threadInfosLock};
            auto& threadInfo = threadInfos[std::this_thread::get_id()];
            threadClients = threadInfo.clients;
        }
        if (threadClients.fd_count == 0) {
            continue;
        }
        int socketCount = select(0, &threadClients, nullptr, nullptr, nullptr);
        if (socketCount < 0) {
            logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when selecting client");
            continue;
        }
        for (int i = 0; i < socketCount; i++) {
            SOCKET client = threadClients.fd_array[i];
            msg::Buffer recvBuff{ 4096 };
            recvBuff.size = recv(client, recvBuff.get(), recvBuff.capacity, 0);
            if (recvBuff.size == 1) {
                logger.log(logs::Level::DEBUG, "Thread ", std::this_thread::get_id(), " got new connection ", client);
                continue;
            }
            if (recvBuff.size > 1) {
                auto header = msg::Header::parse(recvBuff, 0);
                if (header.type == msg::MessageType::write) {
                    auto msg = msg::Write::parse(recvBuff, 0);
                    std::scoped_lock loc{docLock};
                    if (doc.setCursorPos(msg.cursorPos)) {
                        doc.write(msg.msg);
                        broadcast(recvBuff);
                        logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] wrote '", msg.msg, "'");
                    }
                    else {
                        logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on write msg!");
                    }
                }
                else if (header.type == msg::MessageType::erase) {
                    auto msg = msg::Erase::parse(recvBuff, 0);
                    std::scoped_lock loc{docLock};
                    if (doc.setCursorPos(msg.cursorPos)) {
                        doc.erase();
                        broadcast(recvBuff);
                        logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] erased '", msg.eraseSize, "'");
                    }
                    else {
                        logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on erase msg!");
                    }
                }
            }
            else if (recvBuff.size == 0) {
                logger.log(logs::Level::DEBUG, "Connection with ", client, " has been closed");
                shutdownConnection(client);
            }
            else {
                logger.log(logs::Level::ERROR, "Error on receiving data from", client, "! Closing connection");
                shutdownConnection(client);
            }
        }
    }
}

void Server::broadcast(msg::Buffer& buffer) {
    std::scoped_lock lock{threadInfosLock};
    for (const auto& threadInfo : threadInfos) {
        for (int i = 0; i < threadInfo.second.clients.fd_count; i++) {
            SOCKET client = threadInfo.second.clients.fd_array[i];
            if (client == threadInfo.second.notifyListener) {
                continue;
            }
            int sendBytes = send(client, buffer.get(), buffer.size, 0);
            if (sendBytes < 0) {
                logger.log(logs::Level::ERROR, "Error on broadcasting msg to ", client);
            }
        }
    }
}

void Server::shutdownConnection(SOCKET connection) {
    closesocket(connection);
    shutdown(connection, SD_SEND);
    std::scoped_lock lock{threadInfosLock};
    auto& threadInfo = threadInfos[std::this_thread::get_id()];
    FD_CLR(connection, &threadInfo.clients);
}

void Server::close() {
    logger.log(logs::Level::INFO, "Closing server...");
    closesocket(listenSocket);
    logger.log(logs::Level::INFO, "Server closed");
}

#pragma pop_macro("ERROR")