#include <string>
#include <winsock2.h>
#include <future>

#include "terminal.h"
#include "document.h"
#include "logger.h"

#pragma once

class Client {
public:
	Client(std::string username, std::string&& srvIpAddress, 
		const int srvPort, Document& document, TerminalManager& terminal);

	int connectToServer();
	void sendMsg(const COORD& cursorPos, const std::string& content);
	bool ready() const;

private:
	int initWSA();
	int openClientSocket();
	int handleError(const std::string&& prefix);
	void shutdownConn(SOCKET connSocket);
	void listenResponse();
	
	std::string username;
	std::string srvIpAddress;
	int srvPort;
	bool isReady = false;

	SOCKET clientSocket;
	sockaddr_in clientService;

	std::thread recvThread;
	Document& document;
	TerminalManager& terminal;
	Logger logger;
};
