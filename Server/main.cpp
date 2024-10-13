#include <iostream>

#include "winsock2.h"
#include "server.h"

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    int wsaError;
    WORD wVersionRequested = MAKEWORD(2, 2);
    wsaError = WSAStartup(wVersionRequested, &wsaData);
    if (wsaError) {
        std::cout << wsaError << " Error on WSA startup\n";
        WSACleanup();
        return -1;
    }

    Server server{ "192.168.1.10", 8081, 8, "server.log"};
    server.open();
    server.close();
    WSACleanup();
    return 0;
}
