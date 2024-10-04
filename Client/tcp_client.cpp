#include "tcp_client.h"
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

Client::Client(std::string&& srvIpAddress, const int srvPort) :
    srvIpAddress(srvIpAddress),
    srvPort(srvPort) {
    initWSA();
    openClientSocket();
}

int Client::initWSA() {
    WSADATA wsaData;
    int wsaError;
    WORD mVersionRequired = MAKEWORD(2, 2);
    wsaError = WSAStartup(mVersionRequired, &wsaData);
    if (wsaError) {
        std::cout << "[ERROR:" + std::to_string(wsaError) + "] Error on WSAStartup.\n";
        return -1;
    }
    std::cout << "Winsock properly started. Status " << wsaData.szSystemStatus << "\n";
    return 0;
}

int Client::openClientSocket() {
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "[SOCKET OPENING ERROR] " << WSAGetLastError() << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
}

int Client::connectToServer() {
    clientService.sin_family = AF_INET;
    std::wstring stemp{srvIpAddress.begin(), srvIpAddress.end()};
    InetPton(AF_INET, stemp.c_str(), &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(srvPort);

    if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&clientService), sizeof(clientService)) == SOCKET_ERROR) {
        return handleError("[CONNECTION ERROR]");
    }
    handleConnection();
    return 0;
}

int Client::handleError(std::string&& prefix) {
    std::cout << prefix << ' ' << WSAGetLastError() << '\n';
    closesocket(clientSocket);
    WSACleanup();
    return -1;
}

void Client::shutdownConn(SOCKET connSocket) {
    int shutDownResult = shutdown(connSocket, SD_SEND);
    closesocket(connSocket);
}

void Client::handleConnection() {
    char sendBuffer[200];
    char receiveBuffer[200];
    int recvByteCount = 0;
    do {
        std::cin.getline(sendBuffer, 200);
        int sendByteCount = send(clientSocket, sendBuffer, 200, 0);
        if (sendByteCount == SOCKET_ERROR) {
            shutdownConn(clientSocket);
            handleError("[SEND ERROR]");
            return;
        }
        recvByteCount = recv(clientSocket, receiveBuffer, 200, 0);
        if (recvByteCount > 0) {
            std::cout << receiveBuffer << "\n";
        }
        else if (recvByteCount == 0) {
            std::cout << "Closing connection\n";
            shutdownConn(clientSocket);
        }
        else if (recvByteCount < 0) {
            shutdownConn(clientSocket);
            handleError("[RECV ERROR]");
        }
    } while (recvByteCount > 0);
    return;
}
