package com.rpc.easy_rpc_govern.limit.entity;

import com.rpc.easy_rpc_govern.limit.limitAnnotation.RateLimit;
import com.rpc.easy_rpc_govern.limit.limitEnum.BlockStrategy;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import java.util.function.Supplier;

@Data
@AllArgsConstructor
@NoArgsConstructor
@Slf4j
public class LimitingRule {
    // 限流标识id
    private String id;
    // 降级类
    private Class<?> fallBackClass;
    // 降级方法
    private Supplier<Object> fallBackMethod;
    // 限流策略
    private BlockStrategy BlockStrategy;
    // 限流key
    private String limitKey;
    // 限流值
    private Object limitValue;
    // 最大QPS
    private int maxQPS;

    public LimitingRule(RateLimit limitingStrategy){
        this.maxQPS = limitingStrategy.QPS();
        this.BlockStrategy = limitingStrategy.BlockStrategy();
        this.fallBackClass = limitingStrategy.fallBack();
        this.limitKey = limitingStrategy.limitKey();
    }

}
