#include "../include/limit/LimitProcess.h"
bool LimitProcess::limitHandle(const LimitingRule& rule) {
    int Max_QPS = rule.getMaxQPS();
    std::string id = rule.getId();

    std::shared_ptr<SlidingWindow> limitStrategy = limitStrategyMap[id];
    if (!limitStrategy) {
        limitStrategy = std::make_shared<SlidingWindow>(); // 使用默认配置
        limitStrategyMap[id] = limitStrategy;   
    }

    float curQPS;
    if (rule.getLimitValue()) {
        curQPS = limitStrategy->getQPS(rule.getLimitValue());
        if (curQPS < Max_QPS) {
            limitStrategy->incrPassCount(rule.getLimitValue());
            return true;
        } else {
            std::cout << "请求拒绝, 最大QPS: " << Max_QPS << ", 当前QPS: " << curQPS << std::endl;
            limitStrategy->incrBlockCount(rule.getLimitValue());
            return false;
        }
    } else {
        curQPS = limitStrategy->getQPS();
        if (curQPS < Max_QPS) {
            limitStrategy->incrPassCount();
            return true;
        } else {
            std::cout << "请求拒绝, 最大QPS: " << Max_QPS << ", 当前QPS: " << curQPS << std::endl;
            limitStrategy->incrBlockCount();
            return false;
        }
    }
}