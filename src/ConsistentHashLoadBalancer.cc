#include "LoadBalancer.h"
#include <algorithm>

bool compareAddress(const ServiceAddress &a, const ServiceAddress& b) {
    return a.port < b.port;
}

std::string ConsistentHashLoadBalancer::buildServiceInstanceKey(ServiceAddress address) { // 这里用的是eg. "127.0.0.1:4545"做哈希键
    std::string res = address.ip + ":" + std::to_string(address.port);
    return res;
}

std::map<int, ServiceAddress> ConsistentHashLoadBalancer::makeConsistentHashRing(std::vector<ServiceAddress> services) { // 做哈希环
    std::map<int, ServiceAddress> ring;
    std::hash<std::string> str_hash; // 声明一个哈希函数对象
    std::sort(services.begin(), services.end(), compareAddress);
    for (const auto& instance: services) {
        for (int i = 0; i < VALUE_NODE_SIZE; i++) {
            ring[str_hash(buildServiceInstanceKey(instance) + "$" + std::to_string(i))] = instance;
        }
    }
    return ring; // 返回哈希环
}

ServiceAddress ConsistentHashLoadBalancer::allocateNode(std::map<int, ServiceAddress> ring, int hashCode) { // 在哈希环上找位置
    // 获取最近的哈希环上节点位置
    auto entry = ring.upper_bound(hashCode);
    if (entry == ring.end()) {
        entry = ring.begin(); // 如果没有找到就选择最小的节点
    }
    return entry->second; //int和地址信息 
} 

ServiceAddress ConsistentHashLoadBalancer::select(std::vector<ServiceAddress>& discoveries) {
    // discoveries就是注册中心获得的地址们：
    // eg. [127.0.0.1:4544, 127.0.0.1:3000, 127.0.0.1:3002]
    
    ServiceAddress res;
    std::hash<std::string> str_hash; // 声明一个哈希函数对象
    int hashCode = str_hash(buildServiceInstanceKey(discoveries[0]));
    std::cout << "hashcode = " << hashCode << std::endl;
    // make每次都一样，那么hashcode呢？
    res = allocateNode(makeConsistentHashRing(discoveries), hashCode);
    return res;
}
