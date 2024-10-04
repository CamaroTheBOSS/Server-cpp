#include "tcp_client.h"
#include <WS2tcpip.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")

Client::Client(std::string& username, std::string&& srvIpAddress, const int srvPort) :
    username(username),
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
    sendThread = std::thread{ &Client::startSending, this };
    listenResponse();
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

void Client::listenResponse() {
    char receiveBuffer[4096];
    int recvByteCount = 0;
    do {
        recvByteCount = recv(clientSocket, receiveBuffer, 4096, 0);
        if (recvByteCount > 0) {
            if (receiveBuffer[0] != '\0') {
                std::cout << receiveBuffer << "\n";
            }
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

void Client::startSending() {
    do {
        std::string msg;
        std::getline(std::cin, msg);
        msg = "[" + username + "] " + msg;
        int sendByteCount = send(clientSocket, msg.c_str(), msg.size() + 1, 0);
        if (sendByteCount == SOCKET_ERROR) {
            shutdownConn(clientSocket);
            handleError("[SEND ERROR]");
            return;
        }
    } while (true);
}