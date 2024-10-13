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
    logger(logFile) {

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
            auto header = msg::Header::parse(recvBuff, 0);
            if (header.type == msg::MessageType::write) {
                auto msg = msg::Write::parse(recvBuff, 0);
                COORD docCursorPos = doc.getCursorPos();
                if (doc.setCursorPos(msg.cursorPos)) {
                    doc.write(msg.msg);
                    /*if (!msg.isResponse) {
                        document.setCursorPos(docCursorPos);
                    }*/
                    logger.log(logs::Level::INFO, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] wrote '", msg.msg, "'");
                }
                else {
                    logger.log(logs::Level::ERROR, "[", msg.cursorPos.X, ",", msg.cursorPos.Y, "] Cannot place cursor on write msg!");
                }
                terminal.render();
            }
            else if (header.type == msg::MessageType::erase) {
                auto msg = msg::Erase::parse(recvBuff, 0);
            }
            
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

void Client::sendMsg(const COORD& cursorPos, const std::string& content) {
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

#pragma pop_macro("ERROR")