#include <string>
#include <winsock2.h>

#pragma once

class Client {
public:
	Client(std::string&& srvIpAddress, const int srvPort);

	int connectToServer();

private:
	int initWSA();
	int openClientSocket();
	int handleError(std::string&& prefix);
	void shutdownConn(SOCKET connSocket);
	void handleConnection();
	

	std::string srvIpAddress;
	int srvPort;

	SOCKET clientSocket;
	sockaddr_in clientService;
};
