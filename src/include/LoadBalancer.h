#ifndef __LOADBALANCER_H__
#define __LOADBALANCER_H__
#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <climits>
#include <map> // 用来做哈希环
#include <functional> // 

struct ServiceAddress
{
    std::string ip;
    uint16_t port;
};

class LoadBalancer { // 抽象类
public:
// 这里的discoveries就是所有的地址集合
    virtual ServiceAddress select(std::vector<ServiceAddress>& discoveries) = 0;
};


class RoundRobinLoadBalancer: public LoadBalancer {
private:
    static std::atomic<int> roundRobinId; // 创建一个私有静态的原子整数变量，初始化为0   
    
public:
    ServiceAddress select(std::vector<ServiceAddress>& discoveries);
};

class ConsistentHashLoadBalancer: public LoadBalancer { // 一致性哈希
// 需要造出环，虚拟的节点用来分配？
private:
    static const int VALUE_NODE_SIZE = 10; // 每个物理节点在哈希环上的数量
    std::string m_uuid;
    std::string buildServiceInstanceKey(ServiceAddress address);
    std::string buildClientInstanceKey(ServiceAddress address);
    std::map<int, ServiceAddress> makeConsistentHashRing(std::vector<ServiceAddress> services);
    ServiceAddress allocateNode(std::map<int, ServiceAddress> ring, int hashCode);

public:
    ConsistentHashLoadBalancer(std::string uuid) : m_uuid(uuid) {}
    ServiceAddress select(std::vector<ServiceAddress>& discoveries);
};
// const std::string ConsistentHashLoadBalancer::VALUE_NODE_SPLIT = "$"; // 静态整型变量可以类内初始化，但是非整数类型的需要类外定义和初始化
#endif