#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>

#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include "document.h"
#include "logger.h"
#include "message_manager.h"

class Server {

public:
    Server(std::string&& ipAddress, const int port, const int threadPoolSize):
        ipAddress(std::move(ipAddress)),
        port(port),
        threadPoolSize(threadPoolSize),
        document(),
        logger("server.log"),
        msgManager() {
        initWSA();
        openServerSocket();
        bindServerSocket();
    }

    int listenForConns() {
        logger.log("LISTENING");
        auto listenVal = listen(serverSocket, SOMAXCONN);
        if (listenVal == SOCKET_ERROR) {
            logError("[LISTENING ERROR]");
            closesocket(serverSocket);
            WSACleanup();
        }
        initThreadPool();
        int workerIndex = 0;
        while (true) {
            // Accept incoming connection
            auto newConnection = accept(serverSocket, nullptr, nullptr);
            if (newConnection == INVALID_SOCKET) {
                logError("[CONNECTION REFUSED]");
                closesocket(newConnection);
                closesocket(serverSocket);
                WSACleanup();
                return -1;
            }

            // Send current version od document to the new client
            std::string text;
            for (const auto& line : document.get()) {
                text += line;
            }
            if (!text.empty()) {
                char sendBuffer[4096];
                u_long X = htonl(0);
                u_long Y = htonl(0);
                memcpy(sendBuffer + 1, &X, sizeof(u_long));
                memcpy(sendBuffer + 5, &Y, sizeof(u_long));
                memcpy(sendBuffer + 9, text.c_str(), text.size() + 1);
                int msgSize = 10 + text.size();
                int sendByteCount = send(newConnection, sendBuffer, msgSize, 0);
                if (sendByteCount == SOCKET_ERROR) {
                    int shutDownResult = shutdown(newConnection, SD_SEND);
                    closesocket(newConnection);
                    logError("[SEND ERROR]");
                    continue;
                }
            }

            // Assign connection to the next thread and notify it
            std::scoped_lock lock(threadPoolLock);
            auto threadInfo = threadPool.begin() + workerIndex;
            FD_SET(newConnection, &threadInfo->connectionSet);
            workerIndex = (workerIndex + 1) % threadPoolSize;
            int sendByteCount = send(threadInfo->notifier, "", 1, 0);
            if (sendByteCount == SOCKET_ERROR) {
                logError("[NOTIFY ERROR]");
            }
            std::stringstream ss;
            ss << "New connection " << newConnection << " arrived->thread " << threadInfo->thread.get_id() << " notified.";
            logger.log(ss.str());
        }
        return 0;
    }

private:
    void initThreadPool() {
        // Start specified number of workers
        std::scoped_lock lock{threadPoolLock};
        for (int i = 0; i < threadPoolSize; i++) {
            std::thread worker{&Server::handleConnections, this, i};
            auto notifier = accept(serverSocket, nullptr, nullptr);
            if (notifier == INVALID_SOCKET) {
                closesocket(notifier);
                logError("[NOTIFY ERROR]");
                continue;
            }
            fd_set workerConnections;
            FD_ZERO(&workerConnections);
            threadPool.emplace_back(ThreadConns{ std::move(worker), std::move(workerConnections) });
            threadPool[threadPool.size() - 1].notifier = notifier;
        }
        logger.log("Started " + std::to_string(threadPool.size()) + " workers");
    }

    void logError(std::string&& prefix) {
        logger.log(prefix + ' ' + std::to_string(WSAGetLastError()));
    }

    void shutdownConn(const SOCKET connection, const int threadIdx) {
        // Shutdown connection and remove it from FD_SET
        int shutDownResult = shutdown(connection, SD_SEND);
        closesocket(connection);
        std::scoped_lock lock{threadPoolLock};
        auto threadInfo = threadPool.begin() + threadIdx;
        FD_CLR(connection, &threadInfo->connectionSet);
    }

    void broadcastMessage(const char* buffer, const int size, SOCKET sender) {
        std::scoped_lock lock{threadPoolLock};
        for (const auto& threadInfo : threadPool) {
            for (int i = 0; i < threadInfo.connectionSet.fd_count; i++) {
                SOCKET client = threadInfo.connectionSet.fd_array[i];
                int sendByteCount = 0;
                if (client == sender) {
                    std::unique_ptr<char[]> bufferForSender = std::make_unique<char[]>(size);
                    memcpy(bufferForSender.get(), buffer, size);
                    bufferForSender.get()[0] = 1;
                    sendByteCount = send(client, bufferForSender.get(), size, 0);
                }
                else {
                    sendByteCount = send(client, buffer, size, 0);
                }
                if (sendByteCount == SOCKET_ERROR) {
                    logError("[SEND ERROR]");
                }
            }
        }
    }

    int handleConnections(const int threadIdx) {
        // Create socket for communication with master
        SOCKET masterListener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (masterListener == INVALID_SOCKET) {
            logError("[WORKER SOCKET OPENING ERROR]");
            closesocket(masterListener);
            return 0;
        }

        // Connect to master
        sockaddr_in service;
        service.sin_family = AF_INET;
        std::wstring stemp{ipAddress.begin(), ipAddress.end()};
        InetPton(AF_INET, stemp.c_str(), &service.sin_addr.s_addr);
        service.sin_port = htons(port);
        if (connect(masterListener, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
            logError("[MASTER CONNECTION ERROR]");
            return -1;
        }

        // Master communication channel must be added to FD_SET to make select() sensitive for incoming data from master
        {
            std::scoped_lock lock{threadPoolLock};
            auto threadInfo = threadPool.begin() + threadIdx;
            FD_SET(masterListener, &threadInfo->connectionSet);
        }
        while (true) {
            // Copy FD_SET and then select
            FD_SET observedConnections;
            {
                std::scoped_lock lock{threadPoolLock};
                auto threadInfo = threadPool.begin() + threadIdx;
                observedConnections = threadInfo->connectionSet;
            }
            if (observedConnections.fd_count == 0) {
                continue;
            }
            timeval timeout{ 1 };
            int socketCount = select(0, &observedConnections, nullptr, nullptr, &timeout);
            if (socketCount < 0) {
                logError("[SELECT ERROR]");
                continue;
            }
            for (int i = 0; i < socketCount; i++) {
                SOCKET client = observedConnections.fd_array[i];
                char recvBuffer[4096];
                int recvByteCount = 0;
                recvByteCount = recv(client, recvBuffer, 4096, 0);
                if (recvByteCount == 1) {
                    std::stringstream ss;
                    ss << std::this_thread::get_id() << " thread got new conn!";
                    logger.log(ss.str());
                }
                else if (recvByteCount > 0) {
                    std::cout << recvBuffer;
                    COORD cursorPos{0, 0};
                    cursorPos.X = (int)recvBuffer[1] << 24 | recvBuffer[2] << 16 | recvBuffer[3] << 8 | recvBuffer[4];
                    cursorPos.Y = (int)recvBuffer[5] << 24 | recvBuffer[6] << 16 | recvBuffer[7] << 8 | recvBuffer[8];
                    std::string msg{recvBuffer + 9};
                    std::scoped_lock lock{docMutex};
                    if (document.setCursorPos(cursorPos)) {
                        std::stringstream ss;
                        ss << std::this_thread::get_id() << "thread got msg:" << " XY[" << cursorPos.X << "," << cursorPos.Y << "] " << msg;
                        logger.log(ss.str());
                        document.write(msg);
                        broadcastMessage(recvBuffer, recvByteCount, client);
                    }
                    else {
                        logger.log("[ERROR] cannot place cursor!");
                    }
                }
                else if (recvByteCount == 0) {
                    logger.log("Connection closed");
                    shutdownConn(client, threadIdx);
                }
                else {
                    logError("[RECV ERROR]");
                    shutdownConn(client, threadIdx);
                }
            }
        }
        return 0;
    }

    int initWSA() {
        WSADATA wsaData;
        int wsaError;
        WORD wVersionRequested = MAKEWORD(2, 2);
        wsaError = WSAStartup(wVersionRequested, &wsaData);
        if (wsaError) {
            std::cout << "[WSA STARTUP ERROR] " << std::to_string(wsaError) << '\n';
            return -1;
        }
        return 0;
    }

    int openServerSocket() {
        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            logError("[LISTEN SOCKET OPENING ERROR]");
            return -1;
        }
        return 0;
    }

    int bindServerSocket() {
        service.sin_family = AF_INET;
        std::wstring stemp{ipAddress.begin(), ipAddress.end()};
        InetPton(AF_INET, stemp.c_str(), &service.sin_addr.s_addr);
        service.sin_port = htons(port);
        if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
            logError("[LISTEN SOCKET BINDING ERROR]");
            return -1;
        }
        return 0;
    }
    struct ThreadConns {
        std::thread thread;
        FD_SET connectionSet;
        SOCKET notifier;
    };
    std::vector<ThreadConns> threadPool;
    std::mutex threadPoolLock;

    SOCKET serverSocket = INVALID_SOCKET;
    sockaddr_in service;

    std::string ipAddress;
    int port;
    int threadPoolSize;

    std::mutex docMutex;
    Document document;
    Logger logger;
    MessageManager msgManager;
};

int main()
{
    Server server{ "192.168.1.10", 8081, 8 };
    server.listenForConns();
}
