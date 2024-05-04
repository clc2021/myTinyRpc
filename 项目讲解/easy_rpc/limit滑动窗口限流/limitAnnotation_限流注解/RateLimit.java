package com.rpc.easy_rpc_govern.limit.limitAnnotation;

import com.rpc.easy_rpc_govern.limit.limitEnum.BlockStrategy;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface RateLimit {
//    降级处理类
    Class<?> fallBack() default void.class;
//    拒绝策略,默认直接拒绝
    BlockStrategy BlockStrategy() default BlockStrategy.IMMEDIATE_REFUSE;
//      限流请求的主键key，如用户的id
    String limitKey() default "";
//    限流QPS
    int QPS() default 0;

}
