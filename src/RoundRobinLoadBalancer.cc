#include "LoadBalancer.h"
std::atomic<int> RoundRobinLoadBalancer::roundRobinId(0);

ServiceAddress RoundRobinLoadBalancer::select(std::vector<ServiceAddress>& discoveries) {
    int size = discoveries.size();
    roundRobinId.fetch_add(1); // 给它+1
    std::cout << "roundRobinId = " << roundRobinId << std::endl;
    if (roundRobinId.load() == INT_MAX) {
        roundRobinId.store(0);
    }

    return discoveries[roundRobinId.load() % size]; // 返回
}