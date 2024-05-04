// 有FuseProtector类和ServiceState类
#ifndef FUSE_PROTECTOR_H
#define FUSE_PROTECTOR_H
#include <iostream>
#include <atomic> //c11引入的原子
#include "FuseState.h"
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <cmath>
#include <set>
#include "../ServiceAddress.h"

/////////////////////////////////// ServiceState ///////////////////////////////////////////
class ServiceState {
private:
    std::string serviceName; // 服务名
    std::atomic<int> request; // 总的请求的数量 
    std::atomic<int> excepts; // 异常请求的数量
    FUSE_STATE_CODE fuseState;  // 当前服务熔断状态, 可选FULL_OPEN, HALF_OPEN, CLOSE
    float interceptRate; // 当前服务拦截lv
    float k; // 熔断比率

public:
    ServiceState() = default; // 默认构造
    
    ServiceState(std::string serviceName) { // 自定义构造
        this->serviceName = serviceName;
        this->request.store(0);
        this->excepts.store(0);
        this->interceptRate = 0;
        this->fuseState = CLOSE;
        this->k = 1.2;
    }

     ServiceState(const ServiceState& other) { // 拷贝构造
        serviceName = other.serviceName;
        request.store(other.request.load());
        excepts.store(other.excepts.load());
        fuseState = other.fuseState;
        interceptRate = other.interceptRate;
        k = other.k;
    }

    ServiceState& operator=(const ServiceState& other) { // 拷贝赋值
        if (this != &other) {
            serviceName = other.serviceName;
            request.store(other.request.load());
            excepts.store(other.excepts.load());
            fuseState = other.fuseState;
            interceptRate = other.interceptRate;
            k = other.k;
        }
        return *this;
    }

    FUSE_STATE_CODE getFuseState() {
        return fuseState;
    }
    
    float getInterceptRate() {
        return interceptRate;
    }

    void incrRequest();
    
    void incrExcepts();
};

/////////////////////////////////// FuseProtector ///////////////////////////////////////////
class FuseProtector {
private:
    std::unordered_map<std::string, ServiceState> serviceStateCache;
    std::mutex mtx; // 用来做线程安全

public:
    FuseProtector() {}
    
    void initCache(std::unordered_map<std::string, std::set<ServiceAddress>> serviceList);

    bool fuseHandle(std::string serviceName);    
    
    void incrSuccess(std::string serviceName); // 添加成功请求
    
    void incrExcept(std::string serviceName); // 添加异常请求

    void refreshCache(); // 数据定期刷新，确保全开状态下进行重新检查
};
#endif