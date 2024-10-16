#include "tcp_client.h"
#include <WS2tcpip.h>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")
#pragma push_macro("ERROR")
#undef ERROR

Client::Client(std::string srvIp, const int srvPort, std::string logFile,
    Document& doc, TerminalManager& terminal) :
    srvIp(srvIp),
    srvPort(srvPort),
    doc(doc),
    terminal(terminal),
    logger(logFile),
    msgProcessor(doc, terminal, logger, userId) {

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == INVALID_SOCKET) {
        logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error on creating client socket");
        closesocket(client);
    }
}

std::string Client::getUserId() {
    return userId;
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
        msg::Buffer recvBuff{128};
        recvBuff.size = recv(client, recvBuff.get(), recvBuff.capacity, 0);
        if (recvBuff.size > 0) {
            msgProcessor.process(recvBuff);
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

#pragma pop_macro("ERROR")