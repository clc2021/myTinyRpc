#include "LoadBalancer.h"
#include <random>

std::vector<ServiceAddress> RandomLoadBalancer::getDiscoveries(std::set<ServiceAddress>& discoveries) {
    std::vector<ServiceAddress> res;
    for (auto & d : discoveries) {
        res.push_back(d);
    }
    return res;
}

ServiceAddressRes RandomLoadBalancer::select(std::set<ServiceAddress>& discoveries) {
    std::vector<ServiceAddress> addresses = getDiscoveries(discoveries);
    int size = addresses.size();
    if (size == 0) {
        // 如果发现的服务地址为空，则返回空的 ServiceAddressRes 对象
        return ServiceAddressRes();
    }

    // 使用随机数引擎来生成随机索引
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, size - 1);

    int randomIndex = dis(gen); // 生成一个随机索引

    // 从发现的服务地址中随机选择一个地址
    ServiceAddress randomAddress = addresses[randomIndex];
    ServiceAddressRes sar;
    return sar.build(randomAddress, discoveries);
}
