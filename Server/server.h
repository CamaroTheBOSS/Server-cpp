#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>

#include <winsock2.h>

#include "logger.h"
#include "document.h"
#include "messages.h"
#include "load_balancer.h"

#pragma comment(lib, "Ws2_32.lib")

class Server {
public:
	Server(std::string ip, const int port, const int threadPoolSize, std::string logFile);
	void open();
	void close();

private:
	void sync(SOCKET dst);
	void broadcast(msg::Buffer& buffer);

	void initThreadPool();
	void removeThread();

	void handleConnection();
	void shutdownConnection(SOCKET connection);
	

	const std::string ip;
	const int port;
	SOCKET listenSocket = INVALID_SOCKET;
	sockaddr_in listenSocketAddress = {0};

	const int threadPoolSize;
	std::vector<std::thread> threads;
	std::unordered_map<std::thread::id, ThreadInfo> threadInfos;
	std::mutex threadInfosLock;

	logs::Logger logger;
	Document doc;
	std::mutex docLock;
	LoadBalancer loadBalancer;
};
