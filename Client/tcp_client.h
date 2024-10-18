#include <string>
#include <winsock2.h>
#include <future>

#include "terminal.h"
#include "document.h"
#include "messages.h"
#include "processor.h"
#include "logger.h"

#pragma once
#pragma push_macro("ERROR")
#undef ERROR

class Client {
public:
	Client(std::string srvIp, const int srvPort, std::string logFile,
		Document& doc, TerminalManager& terminal);

	int connectToServer();
	template<typename MESSAGE, typename... Args>
	bool sendMsg(Args&&... args) {
		msg::Buffer buffer{128};
		MESSAGE msg{ args... };
		msg.serializeTo(buffer);
		int sendBytes = send(client, buffer.get(), buffer.size, 0);
		if (sendBytes < 0) {
			logger.log(logs::Level::ERROR, WSAGetLastError(), ": Error when sending data to server");
			disconnect();
			return false;
		}
		return true;
	}
	std::pair<std::string, int> waitForResponse() {
		return msgProcessor.waitForResponse();
	}
	std::string getUserId();


private:
	void disconnect();
	void recvMsg();
	
	std::string userId;
	std::string srvIp;
	int srvPort;
	sockaddr_in srvAddress = {0};

	SOCKET client;
	std::thread recvThread;

	Document& doc;
	TerminalManager& terminal;
	logs::Logger logger;
	Processor msgProcessor;
};

#pragma pop_macro("ERROR")