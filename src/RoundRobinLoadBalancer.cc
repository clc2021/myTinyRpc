#include "LoadBalancer.h"
std::atomic<int> RoundRobinLoadBalancer::roundRobinId(0);

std::vector<ServiceAddress> RoundRobinLoadBalancer::getDiscoveries(std::set<ServiceAddress>& discoveries) {
    std::vector<ServiceAddress> res;
    for (auto & d : discoveries) {
        res.push_back(d);
    }
    return res;
}


ServiceAddressRes RoundRobinLoadBalancer::select(std::set<ServiceAddress>& discoveries) {
    int size = discoveries.size();
    int id = roundRobinId.load();
    roundRobinId.fetch_add(1); // 给它+1
    std::cout << "roundRobinId = " << id << std::endl;
    if (roundRobinId.load() == INT_MAX) {
        roundRobinId.store(0);
    }
    std::vector<ServiceAddress> res = getDiscoveries(discoveries);
    ServiceAddress cur = res[id % size];
    ServiceAddressRes sar;
    return sar.build(cur, discoveries); // 返回
}