#include "tcp_client.h"
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

Client::Client(std::string username, std::string&& srvIpAddress, 
    const int srvPort, Document& document, TerminalManager& terminal) :
    username(username),
    srvIpAddress(srvIpAddress),
    srvPort(srvPort),
    document(document),
    terminal(terminal),
    logger("client.log") {
    initWSA();
    openClientSocket();
}

int Client::initWSA() {
    WSADATA wsaData;
    int wsaError;
    WORD mVersionRequired = MAKEWORD(2, 2);
    wsaError = WSAStartup(mVersionRequired, &wsaData);
    if (wsaError) {
        logger.log("[ERROR:" + std::to_string(wsaError) + "] Error on WSAStartup");
        return -1;
    }
    logger.log("Winsock properly started. Status " + std::string(wsaData.szSystemStatus));
    return 0;
}

int Client::openClientSocket() {
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        logger.log("[SOCKET OPENING ERROR] " + std::to_string(WSAGetLastError()));
        closesocket(clientSocket);
        WSACleanup();
        return 0;
    }
    return 1;
}

int Client::connectToServer() {
    clientService.sin_family = AF_INET;
    std::wstring stemp{srvIpAddress.begin(), srvIpAddress.end()};
    InetPton(AF_INET, stemp.c_str(), &clientService.sin_addr.s_addr);
    clientService.sin_port = htons(srvPort);
    if (connect(clientSocket, reinterpret_cast<SOCKADDR*>(&clientService), sizeof(clientService)) == SOCKET_ERROR) {
        return handleError("[CONNECTION ERROR]");
    }
    recvThread = std::thread{ &Client::listenResponse, this };
    return 0;
}

int Client::handleError(const std::string&& prefix) {
    logger.log(prefix + std::to_string(WSAGetLastError()));
    closesocket(clientSocket);
    WSACleanup();
    return -1;
}

void Client::shutdownConn(SOCKET connSocket) {
    logger.log("Connection shut down");
    int shutDownResult = shutdown(connSocket, SD_SEND);
    closesocket(connSocket);
}

void Client::listenResponse() {
    char recvBuffer[4096];
    int recvByteCount = 0;
    while (!isReady) {};
    do {
        recvByteCount = recv(clientSocket, recvBuffer, 4096, 0);
        if (recvByteCount > 0) {
            COORD cursorPos{ 0, 0 };
            bool myMsg = (bool)recvBuffer[0];
            cursorPos.X = (int)recvBuffer[1] << 24 | recvBuffer[2] << 16 | recvBuffer[3] << 8 | recvBuffer[4];
            cursorPos.Y = (int)recvBuffer[5] << 24 | recvBuffer[6] << 16 | recvBuffer[7] << 8 | recvBuffer[8];
            std::string msg{recvBuffer + 9};
            COORD docCursorPos = document.getCursorPos();
            if (document.setCursorPos(cursorPos)) {
                document.write(msg);
                if (!myMsg) {
                    document.setCursorPos(docCursorPos);
                }
                std::stringstream ss;
                ss << "got msg:" << " XY[" << cursorPos.X << "," << cursorPos.Y << "] " << msg;
                logger.log(ss.str());
            }
            terminal.render();
        }
        else if (recvByteCount == 0) {
            logger.log("Closing connection");
            shutdownConn(clientSocket);
        }
        else if (recvByteCount < 0) {
            shutdownConn(clientSocket);
            handleError("[RECV ERROR]");
        }
    } while (recvByteCount > 0);
    return;
}

void Client::sendMsg(const COORD& cursorPos, const std::string& content) {
    char sendBuffer[4096];
    u_long X = htonl(cursorPos.X);
    u_long Y = htonl(cursorPos.Y);
    sendBuffer[0] = '\0';
    memcpy(sendBuffer + 1, &X, sizeof(u_long));
    memcpy(sendBuffer + 5, &Y, sizeof(u_long));
    memcpy(sendBuffer + 9, content.c_str(), content.size() + 1);
    int msgSize = 10 + content.size();
    int sendByteCount = send(clientSocket, sendBuffer, msgSize, 0);
    if (sendByteCount == SOCKET_ERROR) {
        shutdownConn(clientSocket);
        handleError("[SEND ERROR]");
        return;
    }
    isReady = true;
    std::stringstream ss;
    ss << sendByteCount << " sent msg:" << " XY[" << cursorPos.X << "," << cursorPos.Y << "] " << content;
    logger.log(ss.str());
}

bool Client::ready() const {
    return isReady;
}