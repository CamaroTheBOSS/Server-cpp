#include <string>
#include <winsock2.h>
#include <future>

#include "terminal.h"
#include "document.h"
#include "messages.h"
#include "logger.h"

#pragma once

class Client {
public:
	Client(std::string username, std::string srvIp, const int srvPort, std::string logFile,
		Document& doc, TerminalManager& terminal);

	int connectToServer();
	void sendMsg(const COORD& cursorPos, const std::string& content);

private:
	void disconnect();
	void recvMsg();
	
	std::string username;
	
	std::string srvIp;
	int srvPort;
	sockaddr_in srvAddress;

	SOCKET client;
	std::thread recvThread;

	Document& doc;
	TerminalManager& terminal;
	logs::Logger logger;
};
