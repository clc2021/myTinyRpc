package com.rpc.easy_rpc_govern.limit.config;

import com.rpc.easy_rpc_govern.limit.aspect.LimitAspect;
import com.rpc.easy_rpc_govern.limit.entity.LimitingRule;
import com.rpc.easy_rpc_govern.limit.limitEnum.BlockStrategy;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Component;
import javax.annotation.Resource;
import java.util.function.Supplier;

@Component
@Slf4j
public abstract class UnifiedLimitConfig {
    @Resource
    LimitAspect limitAspect;
    public void configUnifiedLimitKey(String key, int maxQPS, Supplier<Object> fallBackMethod) {

        if (maxQPS < 1) {
            log.error("maxQPS不能小于1");
        }
        LimitingRule rule = new LimitingRule();
        rule.setId("unifiedLimitConfig");
        rule.setLimitValue(key);
        rule.setBlockStrategy(BlockStrategy.IMMEDIATE_REFUSE);
        rule.setMaxQPS(maxQPS);
        rule.setFallBackMethod(fallBackMethod);
        ThreadLocal<LimitingRule> threadLocal = new ThreadLocal<>();
        threadLocal.set(rule);
        limitAspect.threadLocal = threadLocal;
    }

}
