#include <string>
#include <winsock2.h>
#include <future>

#pragma once

class Client {
public:
	Client(std::string& username, std::string&& srvIpAddress, const int srvPort);

	int connectToServer();

private:
	int initWSA();
	int openClientSocket();
	int handleError(std::string&& prefix);
	void shutdownConn(SOCKET connSocket);
	void listenResponse();
	void startSending();
	
	std::string username;
	std::string srvIpAddress;
	int srvPort;

	SOCKET clientSocket;
	sockaddr_in clientService;
	std::thread sendThread;
};
