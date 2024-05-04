#ifndef LIMIT_PROCESS_H
#define LIMIT_PROCESS_H

#include "LimitingRule.h"
#include "SlidingWindow.h"
#include <unordered_map>
#include <memory>
 // 实际上应该是limitStrategy.h 但是我们没写这个文件，而且只有
class LimitProcess {
private:
// 这是字符串-限流策略
//id-限流策略*
// to_do：这里用了智能指针？什么时候用智能指针呢
// 也许这个string可以是用户id,可以是用户uuid，可以是函数名？
    std::unordered_map<std::string, std::shared_ptr<SlidingWindow>> limitStrategyMap;
public:
    bool limitHandle(const LimitingRule& rule);
};

#endif