#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>

#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

class Server {

public:
    Server(std::string&& ipAddress, const int port):
        ipAddress(std::move(ipAddress)),
        port(port) {
        initWSA();
        openServerSocket();
        bindServerSocket();
    }

    int listenForConns() {
        std::cout << "LISTENING\n";
        auto listenVal = listen(serverSocket, SOMAXCONN);
        if (listenVal == SOCKET_ERROR) {
            return handleError("[LISTENING ERROR]");
        }
        while (true) {
            auto connSocket = accept(serverSocket, nullptr, nullptr);
            if (connSocket == INVALID_SOCKET) {
                closesocket(connSocket);
                return handleError("[CONNECTION REFUSED]");
            }
            std::thread connThread{&Server::handleConnection, this, connSocket};
            std::thread::id id = connThread.get_id();
            connMap.emplace(id, Connection{ std::move(connThread), connSocket });
            finishExpiredThreads();
            
        }
        return 0;
    }

private:
    void finishExpiredThreads() {
        std::scoped_lock lock{expThreadsLock};
        for (const auto& expThreadId : expiredThreads) {
            auto connMapEntry = connMap.find(expThreadId);
            if (connMapEntry == connMap.end()) {
                continue;
            }
            if (connMapEntry->second.thread.joinable()) {
                connMapEntry->second.thread.join();
            }
            connMap.erase(connMapEntry);
        }
        expiredThreads.clear();
    }

    int handleError(std::string&& prefix) {
        std::cout << prefix << ' ' << WSAGetLastError() << '\n';
        closesocket(serverSocket);
        WSACleanup();
        return -1;
    }

    void shutdownConn(SOCKET connSocket) {
        int shutDownResult = shutdown(connSocket, SD_SEND);
        closesocket(connSocket);
        std::scoped_lock lock{expThreadsLock};
        expiredThreads.push_back(std::this_thread::get_id());
    }

    int handleConnection(SOCKET connSocket) {
        char recvBuffer[200];
        int recvByteCount = 0;
        do {
            recvByteCount = recv(connSocket, recvBuffer, 200, 0);
            if (recvByteCount > 0) {
                std::cout << recvBuffer << "\n";
                int sendByteCount = send(connSocket, recvBuffer, 200, 0);
                if (sendByteCount == SOCKET_ERROR) {
                    shutdownConn(connSocket);
                }
            }
            else if (recvByteCount == 0){
                std::cout << "Closing connection\n";
                shutdownConn(connSocket);
            }
            else {
                shutdownConn(connSocket);
            }
        } while (recvByteCount > 0);
        return 0;
    }

    int initWSA() {
        WSADATA wsaData;
        int wsaError;
        WORD wVersionRequested = MAKEWORD(2, 2);
        wsaError = WSAStartup(wVersionRequested, &wsaData);
        if (wsaError) {
            std::cout << "[ERROR:" + std::to_string(wsaError) + "] Error on WSAStartup.\n";
            return -1;
        }
        else {
            std::cout << "Winsock properly started. Status " << wsaData.szSystemStatus << "\n";
        }
        return 0;
    }

    int openServerSocket() {
        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSocket == INVALID_SOCKET) {
            return handleError("[SOCKET OPENING ERROR]");
        }
        return 0;
    }

    int bindServerSocket() {
        service.sin_family = AF_INET;
        std::wstring stemp{ipAddress.begin(), ipAddress.end()};
        InetPton(AF_INET, stemp.c_str(), &service.sin_addr.s_addr);
        service.sin_port = htons(port);
        if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
            return handleError("[SOCKET BINDING ERROR]");
        }
        return 0;
    }

    struct Connection {
        std::thread thread;
        SOCKET socket;
    };
    std::unordered_map<std::thread::id, Connection> connMap;
    std::vector<std::thread::id> expiredThreads;
    std::mutex expThreadsLock;

    SOCKET serverSocket = INVALID_SOCKET;
    sockaddr_in service;

    std::string ipAddress;
    int port;
};

int main()
{
    Server server{ "127.0.0.1", 8081 };
    server.listenForConns();
}
