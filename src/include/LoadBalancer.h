#ifndef __LOADBALANCER_H__
#define __LOADBALANCER_H__
#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <climits>
#include <map> // 用来做哈希环
#include <functional> // 
#include <set>

struct ServiceAddress
{
    std::string ip;
    uint16_t port;

    bool operator< (const ServiceAddress & other) const {
        return ip != other.ip ? ip < other.ip : port < other.port;
    }
};

// 这个类可以专门用来返回给Channel，这样可以准备当前节点和剩余节点
class ServiceAddressRes {
private:
    ServiceAddress curServiceAddress;
    std::set<ServiceAddress> otherServiceAddress;
public:
    std::set<ServiceAddress> getOtherServiceAddress() {
        return otherServiceAddress;
    }

    ServiceAddress getCurServiceAddress() {
        return curServiceAddress;
    }

    ServiceAddressRes(ServiceAddress cur, std::set<ServiceAddress> others) {
        curServiceAddress = cur;
        otherServiceAddress = std::move(others);
    }

    ServiceAddressRes() {}

    ServiceAddressRes build(ServiceAddress cur, std::set<ServiceAddress> others) {
        if (others.size() == 1)
            others.clear();
        else
            others.erase(cur);
        ServiceAddressRes serviceAddressRes(cur, others);
        return serviceAddressRes;
    }
};

class LoadBalancer { // 抽象类
public:
// 这里的discoveries就是所有的地址集合
    virtual ServiceAddressRes select(std::set<ServiceAddress>& discoveries) = 0;
};


class RoundRobinLoadBalancer: public LoadBalancer {
private:
    static std::atomic<int> roundRobinId; // 创建一个私有静态的原子整数变量，初始化为0   
    std::vector<ServiceAddress> getDiscoveries(std::set<ServiceAddress>& discoveries);

public:
    ServiceAddressRes select(std::set<ServiceAddress>& discoveries);
};

class ConsistentHashLoadBalancer: public LoadBalancer { // 一致性哈希
// 需要造出环，虚拟的节点用来分配？
private:
    static const int VALUE_NODE_SIZE = 10; // 每个物理节点在哈希环上的数量
    std::string m_uuid;
    std::string buildServiceInstanceKey(ServiceAddress address);
    std::string buildClientInstanceKey(ServiceAddress address);
    std::map<int, ServiceAddress> makeConsistentHashRing(std::set<ServiceAddress> services);
    ServiceAddress allocateNode(std::map<int, ServiceAddress> ring, int hashCode);

public:
    ConsistentHashLoadBalancer(std::string uuid) : m_uuid(uuid) {}
    ServiceAddressRes select(std::set<ServiceAddress>& discoveries);
};
// const std::string ConsistentHashLoadBalancer::VALUE_NODE_SPLIT = "$"; // 静态整型变量可以类内初始化，但是非整数类型的需要类外定义和初始化
#endif