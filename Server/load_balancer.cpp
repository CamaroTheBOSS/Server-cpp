#include "load_balancer.h"

LoadBalancer::LoadBalancer(std::unordered_map<std::thread::id, ThreadInfo>& threadInfos) :
    threadInfos(threadInfos) {}

ThreadMapIterator LoadBalancer::select() {
    int leastConns = INT_MAX;
    ThreadMapIterator bestWorkerIt = threadInfos.begin();
    for (auto it = threadInfos.begin(); it != threadInfos.end(); it++) {
        if (it->second.clients.fd_count < leastConns) {
            leastConns = it->second.clients.fd_count;
            bestWorkerIt = it;
        }
    }
    return bestWorkerIt;
}