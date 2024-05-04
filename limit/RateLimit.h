// #ifndef RATE_LIMIT_H
// #define RATE_LIMIT_H

// #include "BlockStrategy.h"
// #include <string>

// template <typename T> // to_do???暂时用模板
// class RateLimit {
// public:
//     RateLimit(): fallBack(nullptr), blockStrategy(IMMEDIATE_REFUSE), limitKey(""), QPS(0) {}
//     T* fallback; // 降级处理类
//     BlockStrategy blockStrategy; // 拒绝策略，默认直接拒绝
//     std::string limitKey; // 限流请求的主键key，如用户id
//     int QPS; // 限流QPS
// };