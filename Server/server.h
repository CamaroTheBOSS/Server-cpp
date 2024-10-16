#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>

#include <winsock2.h>

#include "logger.h"
#include "messages.h"
#include "load_balancer.h"
#include "repository.h"

#pragma comment(lib, "Ws2_32.lib")

class Server {
public:
	Server(std::string ip, const int port, const int threadPoolSize, std::string logFile);
	void open();
	void close();

private:
	void sync(SOCKET dst);
	void process(int socketCount, FD_SET& connections);
	void broadcast(msg::Buffer& buffer);
	void unicast(msg::Buffer& buffer, SOCKET& src);
	void makeResponse(msg::Buffer& buffer, SOCKET& src);

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
	Repository repo;
	LoadBalancer loadBalancer;

};
