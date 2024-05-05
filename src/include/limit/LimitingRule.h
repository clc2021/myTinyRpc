// 限流和降级策略的封装
#ifndef LIMITING_RULE_H
#define LIMITING_RULE_H
#include <iostream>
#include <string>
#include <functional>
#include "BlockStrategy.h"

// id：movie_streaming_rule
// fallBackClass：FallbackMovieService.class
// fallBackMethod：getFallbackMovie() 方法提供者
// BlockStrategy：BlockStrategy.REJECT（直接拒绝）
// limitKey：用户的ID
// limitValue：请求的时间戳
// maxQPS：100 （每秒最多处理100个请求）

// 这是一个整体的降级类，这个降级类里有2个降级方法
class FallBackClass { // 降级类。最初的降级类，之后可以用别的类去扩展它
public:
    void handleFallback1() {
        std::cout << "降级类的方法1..." << std::endl;
    }

    void handleFallback2() {
        std::cout << "降级类的方法2..." << std::endl;
    }
};

// 外部声明 FallBackMethod 函数
extern void FallBackMethod();

class LimitingRule {
private:
    std::string id; // 限流标识id
    FallBackClass* fallBackClass; // 降级类对象指针
    std::function<void()> fallBackMethod; // 降级方法 // to_do 这里降级方法用这个可以吗？
    BLOCK_STRATEGY blockStrategy; // 限流策略
    std::string limitKey; // 限流key
    void* limitValue; // 限流值
    int maxQPS; // 最大QPS

public:
    LimitingRule() {
        fallBackClass = nullptr;
        blockStrategy = IMMEDIATE_REFUSE;
        limitKey = "";
        maxQPS = 0;
        fallBackMethod = std::bind(FallBackMethod);
    }

    LimitingRule(const std::string& _id, 
                std::function<void()> _fallBackMethod,
                BLOCK_STRATEGY _blockStrategy, 
                const std::string& _limitKey, 
                void* _limitValue, 
                int _maxQPS)
        : id(_id), fallBackClass(nullptr), fallBackMethod(_fallBackMethod), blockStrategy(_blockStrategy),
          limitKey(_limitKey), limitValue(_limitValue), maxQPS(_maxQPS) {}
    
    ~LimitingRule() { delete fallBackClass; }

    int getMaxQPS() const { return maxQPS; }
    std::string getId() const { return id; }
    void* getLimitValue() const { return limitValue; }
    std::function<void()> getFallBackMethod() const { return fallBackMethod; }
    FallBackClass* getFallBackClass() const { return fallBackClass; }
    std::string getLimitKey() { return limitKey; }
    
    void setId(std::string _id) { id = _id; }
    void setFallBackClass(FallBackClass* _fallBackClass) { fallBackClass = _fallBackClass; }
    void setLimitValue(void* _limitValue) { limitValue = _limitValue; }
};

#endif