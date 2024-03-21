#include "LoadBalancer.h"
std::atomic<int> RoundRobinLoadBalancer::roundRobinId(0);

ServiceAddress RoundRobinLoadBalancer::select(std::vector<ServiceAddress>& discoveries) {
    int size = discoveries.size();
    int id = roundRobinId.load();
    roundRobinId.fetch_add(1); // 给它+1
    std::cout << "roundRobinId = " << id << std::endl;
    if (roundRobinId.load() == INT_MAX) {
        roundRobinId.store(0);
    }

    return discoveries[id % size]; // 返回
}