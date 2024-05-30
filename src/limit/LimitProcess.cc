// /home/ubuntu/projects/tinyrpcGai/src/limit/LimitProcess.cc
#include "../include/limit/LimitProcess.h"
bool LimitProcess::limitHandle(const LimitingRule& rule) {
    int Max_QPS = rule.getMaxQPS(); // 100
    std::string id = rule.getId(); // Login

    // 检查是否存在对应的限流策略，如果不存在则创建新的策略
    {
        std::lock_guard<std::mutex> lock(mapMtx); // 加锁保护对 limitStrategyMap 的操作
        auto it = limitStrategyMap.find(id);
        if (it == limitStrategyMap.end()) {
            std::cout << "std::shared_ptr<SlidingWindow> limitStrategy = std::make_shared<SlidingWindow>() 前" << std::endl;
            std::shared_ptr<SlidingWindow> limitStrategy = std::make_shared<SlidingWindow>(); // 使用默认配置
            std::cout << "std::shared_ptr<SlidingWindow> limitStrategy = std::make_shared<SlidingWindow>() 后" << std::endl;
            limitStrategyMap[id] = limitStrategy;
            it = limitStrategyMap.find(id); // 更新迭代器
        }
    }

    float curQPS;
    if (rule.getLimitValue()) { // 如果limitValue不空
        std::cout << "在LimitProcess中, limitValue不空, " << reinterpret_cast<intptr_t>(rule.getLimitValue()) << std::endl;
        curQPS = limitStrategyMap[id]->getQPS(rule.getLimitValue());
        if (curQPS < Max_QPS) {
            std::cout << "请求接受了, 最大QPS: " << Max_QPS << ", 当前QPS: " << curQPS << std::endl;
            limitStrategyMap[id]->incrPassCount(rule.getLimitValue());
            return true;
        } else {
            std::cout << "请求拒绝, 最大QPS: " << Max_QPS << ", 当前QPS: " << curQPS << std::endl;
            limitStrategyMap[id]->incrBlockCount(rule.getLimitValue());
            return false;
        }
    } 

    // 这个是我的limitValue的方法，然后我主要是针对RPC调用的Login()和Register()
    else { // 如果limitValue空
        std::cout << "在LimitProcess中, limitValue空, " << reinterpret_cast<intptr_t>(rule.getLimitValue()) << std::endl;
        curQPS = limitStrategyMap[id]->getQPS();
        if (curQPS < Max_QPS) {
            std::cout << "请求接受了, 最大QPS: " << Max_QPS << ", 当前QPS: " << curQPS << std::endl;
            limitStrategyMap[id]->incrPassCount();
            return true;
        } else {
            std::cout << "请求拒绝, 最大QPS: " << Max_QPS << ", 当前QPS: " << curQPS << std::endl;
            limitStrategyMap[id]->incrBlockCount();
            return false;
        }
    }
}