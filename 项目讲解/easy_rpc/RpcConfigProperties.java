package com.rpc.easy_rpc_govern.config;

import lombok.*;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.context.annotation.Configuration;
import org.springframework.stereotype.Component;
import org.springframework.validation.annotation.Validated;

import java.util.List;

@Configuration
@ConfigurationProperties(prefix = "rpc")
@Validated
@Data
public class RpcConfigProperties {

    /**
     * 默认序列化方式
     */
    @NonNull
    public String serialization = "hessianSerialization";

    // 协议魔数
    public short magicNumber = 0x1ce2;

    // 协议版本
    public byte version = 1;

    // 序列化方式, 1: hessian, 2: json, 3: protobuf
    public byte serializationType = 2;


    /**
     * 此为服务端(服务提供者)配置
     */
    @Data
    @Component("rpcProvider")
    @ConfigurationProperties(prefix = "rpc.provider")
    public static class RpcProvider {
        /**
         * 服务提供运行主机
         */
        public String host;
        /**
         * 服务提供运行端口
         */
        public int port;
        /**
         * 服务提供实例id
         */
        public String clientId;

    }
    /**
     * 此为客户端配置
     */
    @Data
    @Component("rpcConsumer")
    @ConfigurationProperties(prefix = "rpc.consumer")
    public static class RpcConsumer {
        /**
         * 负载均衡策略,默认为轮询
         */
        public String loadBalance = "polling";
        /**
         * 熔断比率
         */
        public Float k = 1.2f;
        /**
         * 服务拉取时间,默认12秒
         */
        public Integer getServiceTime = 12;
        /**
         * 消费最大等待时长 单位ms
         */
        public Long consumeWaitInMs = 4000L;
        /**
         * 熔断开关
         */
        public Boolean fuseEnable = false;
        /**
         * 负载均衡开关
         */
        public Boolean loadBalanceEnable = true;
        /**
         * 链路跟踪开关
         */
        public Boolean traceEnable = false;
        /**
         * 服务链路跟踪策略
         * 策略为time 则serviceTraceInterval秒内跟踪serviceTraceNum次
         * 策略为num 则serviceTraceInterval次调用跟踪serviceTraceNum次
         */
        public String serviceTraceStrategy = "time";
        /**
         * 服务链路跟踪区间,策略为time则为秒,策略为num则为次数
         */
        public Integer serviceTraceInterval = 1;
        /**
         * 服务链路跟踪数量
         */
        public Integer serviceTraceNum = 10;

    }

    /**
     * 此为注册中心配置
     */
    @Data
    @Component("rpcRegistry")
    @ConfigurationProperties(prefix = "rpc.registry")
    public static class RpcRegistry {
        /**
         * 注册中心运行ip
         */
        public String host;

        /**
         * 注册中心运行端口
         */
        public Integer port;

        /**
         * 服务过期时间,默认6秒
         */
        public Long serviceSaveTime = 6000L;

        /**
         * 是否开始集群模式
         */
        public Boolean cluster = false;

        /**
         * 集群的ip配置
         */
        public List<String> clusterAddress;

        /**
         *  leader发送心跳时间间隔
         */
        public Long sendLeaderHeartBeatTime = 5000L;

        /**
         * 检测Leader心跳的执行间隔
         */
        public Long checkLeaderHeartBeatTime = 7000L;
    }

}
