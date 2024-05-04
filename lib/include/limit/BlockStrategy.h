// 拒绝策略
#ifndef BLOCK_STRATEGY_H
#define BLOCK_STRATEGY_H
enum BLOCK_STRATEGY {
    IMMEDIATE_REFUSE = 0, // 立即拒绝
    WARM_UP, // 冷启动
    UNIFORM_QUEUE // 匀速排队
};
#endif