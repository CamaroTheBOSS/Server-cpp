﻿#include "tcp_client.h"
#include <iostream>

int main()
{
    std::string username;
    std::cout << "Write your username: ";
    std::cin >> username;
    Client client{ username, "192.168.1.10", 8081};
    client.connectToServer();
}
