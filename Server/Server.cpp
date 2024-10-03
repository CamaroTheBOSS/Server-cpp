#include <iostream>
#include <string>
#include <mutex>
#include <unordered_map>
#include <future>

#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

class Server {

public:
    Server(std::string&& ipAddress, const int port, const int threadPoolSize):
        ipAddress(std::move(ipAddress)),
        port(port),
        threadPoolSize(threadPoolSize) {
        initWSA();
        openServerSocket();
        bindServerSocket();
    }

    int listenForConns() {
        std::cout << "LISTENING\n";
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
            // Assign connection to the next thread and notify it
            std::scoped_lock lock(threadPoolLock);
            auto threadInfo = threadPool.begin() + workerIndex;
            FD_SET(newConnection, &threadInfo->connectionSet);
            workerIndex = (workerIndex + 1) % threadPoolSize;
            int sendByteCount = send(threadInfo->notifier, "", 1, 0);
            if (sendByteCount == SOCKET_ERROR) {
                logError("[NOTIFY ERROR]");
            }   
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
        std::cout << "Started " << std::to_string(threadPool.size()) << " workers\n";
    }

    void logError(std::string&& prefix) {
        std::cout << prefix << ' ' << WSAGetLastError() << '\n';
    }

    void shutdownConn(const SOCKET connection, const int threadIdx) {
        // Shutdown connection and remove it from FD_SET
        int shutDownResult = shutdown(connection, SD_SEND);
        closesocket(connection);
        std::scoped_lock lock{threadPoolLock};
        auto threadInfo = threadPool.begin() + threadIdx;
        FD_CLR(connection, &threadInfo->connectionSet);
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
            int socketCount = select(0, &observedConnections, nullptr, nullptr, nullptr);
            if (socketCount < 0) {
                logError("[SELECT ERROR]");
                continue;
            }
            for (int i = 0; i < socketCount; i++) {
                SOCKET client = observedConnections.fd_array[i];
                char recvBuffer[200];
                int recvByteCount = 0;
                recvByteCount = recv(client, recvBuffer, 200, 0);
                if (recvByteCount > 0) {
                    std::cout << recvBuffer << "\n";
                    int sendByteCount = send(client, recvBuffer, 200, 0);
                    if (sendByteCount == SOCKET_ERROR) {
                        logError("[SEND ERROR]");
                        shutdownConn(client, threadIdx);
                    }
                }
                else if (recvByteCount == 0) {
                    std::cout << "Closing connection\n";
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
};

int main()
{
    Server server{ "127.0.0.1", 8081, 2 };
    server.listenForConns();
}
