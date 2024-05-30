#ifndef LIMIT_PROCESS_H
#define LIMIT_PROCESS_H

#include "LimitingRule.h"
#include "SlidingWindow.h"
#include <unordered_map>
#include <memory>
#include <mutex>
class LimitProcess {
private:
    std::unordered_map<std::string, std::shared_ptr<SlidingWindow>> limitStrategyMap;
    std::mutex mapMtx; // 锁住这个图

public:
    bool limitHandle(const LimitingRule& rule);
};

#endif