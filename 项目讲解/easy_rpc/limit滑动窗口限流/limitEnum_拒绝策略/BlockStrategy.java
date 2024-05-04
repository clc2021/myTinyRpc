package com.rpc.easy_rpc_govern.limit.limitEnum;

/**
 * 拒绝策略
 */
public enum BlockStrategy {
//    立即拒绝
    IMMEDIATE_REFUSE,
//    冷启动
    WARM_UP,
//    匀速排队
    UNIFORM_QUEUE
}
