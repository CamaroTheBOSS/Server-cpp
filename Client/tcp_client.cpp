#include "tcp_client.h"
#include <WS2tcpip.h>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")
#pragma push_macro("ERROR")
#undef ERROR

Client::Client(std::string username, std::string srvIp, const int srvPort, std::string logFile,
    Document& doc, TerminalManager& terminal) :
    username(username),
    srvIp(srvIp),
    srvPort(srvPort),
    doc(doc),
    terminal(terminal),
    logger(logFile),
    msgProcessor(doc, terminal, logger) {

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == INVALID_SOCKET) {
        logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error on creating client socket");
        closesocket(client);
    }
}

int Client::connectToServer() {
    srvAddress.sin_family = AF_INET;
    srvAddress.sin_port = htons(srvPort);
    std::wstring ipStr{srvIp.begin(), srvIp.end()};
    InetPton(AF_INET, ipStr.c_str(), &srvAddress.sin_addr.s_addr);
    if (connect(client, reinterpret_cast<SOCKADDR*>(&srvAddress), sizeof(srvAddress))) {
        closesocket(client);
        logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error on connecting to server");
        return -1;
    }
    recvThread = std::thread{ &Client::recvMsg, this };
    return 0;
}

void Client::disconnect() {
    logger.log(logs::Level::INFO, "Disconnected from the server");
    shutdown(client, SD_SEND);
    closesocket(client);
}

void Client::recvMsg() {
    while (true) {
        msg::Buffer recvBuff{4096};
        recvBuff.size = recv(client, recvBuff.get(), recvBuff.capacity, 0);
        if (recvBuff.size > 0) {
            msgProcessor.processMessage(recvBuff);
        }
        else if (recvBuff.size == 0) {
            disconnect();
            break;
        }
        else if (recvBuff.size < 0) {
            logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when receiving data from server");
            disconnect();
            break;
        }
    }
    return;
}

void Client::sendWriteMsg(const COORD& cursorPos, const std::string& content) {
    msg::Write msg{1, 1, cursorPos, content};
    msg::Buffer buff{4096};
    msg.serializeTo(buff);
    int sendBytes = send(client, buff.get(), buff.size, 0);
    if (sendBytes < 0) {
        logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when sending data to server");
        disconnect();
        return;
    }
}

void Client::sendEraseMsg(const COORD& cursorPos, const int eraseSize) {
    msg::Erase msg{1, 1, cursorPos, eraseSize};
    msg::Buffer buff{4096};
    msg.serializeTo(buff);
    int sendBytes = send(client, buff.get(), buff.size, 0);
    if (sendBytes < 0) {
        logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when sending data to server");
        disconnect();
        return;
    }
}

#pragma pop_macro("ERROR")