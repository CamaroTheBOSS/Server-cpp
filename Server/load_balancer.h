#pragma once
#include <unordered_map>
#include <mutex>
#include <WinSock2.h>

struct ThreadInfo {
	FD_SET clients;
	SOCKET notifier;
	SOCKET notifyListener;
};

using ThreadMapIterator = std::unordered_map<std::thread::id, ThreadInfo>::iterator;

class LoadBalancer {
public:
	LoadBalancer(std::unordered_map<std::thread::id, ThreadInfo>& threadInfos);
	ThreadMapIterator select();

private:
	std::unordered_map<std::thread::id, ThreadInfo>& threadInfos;
};
