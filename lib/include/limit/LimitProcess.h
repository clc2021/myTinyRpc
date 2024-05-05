#ifndef LIMIT_PROCESS_H
#define LIMIT_PROCESS_H

#include "LimitingRule.h"
#include "SlidingWindow.h"
#include <unordered_map>
#include <memory>
#include <mutex>
class LimitProcess {
private:
// to_do: 这里用了智能指针？什么时候用智能指针呢. 
// to_do: 也许这个string可以是用户id,可以是用户uuid，可以是函数名？
    std::unordered_map<std::string, std::shared_ptr<SlidingWindow>> limitStrategyMap;
    std::mutex mapMtx; // 锁住这个图

public:
    bool limitHandle(const LimitingRule& rule);
};

#endif