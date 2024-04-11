package com.rpc.easy_rpc_govern.context;

import com.rpc.domain.annotation.RpcConsumer;
import com.rpc.domain.pojo.*;
import com.rpc.easy_rpc_govern.trace.Strategy.TraceStrategyContext;
import com.rpc.easy_rpc_govern.Interceptor.RequestInterceptHandler;
import com.rpc.easy_rpc_govern.filter.RequestFilterHandler;
import com.rpc.easy_rpc_govern.filter.ResponseFilterHandler;
import com.rpc.easy_rpc_govern.fuse.FuseHandler;
import com.rpc.easy_rpc_govern.limit.LimitHandler;
import com.rpc.easy_rpc_govern.loadBalancer.LoadBalancerHandler;
import com.rpc.easy_rpc_govern.config.RpcConfigProperties;
import com.rpc.easy_rpc_govern.route.RouteHandler;
import com.rpc.easy_rpc_govern.utils.SnowFlakeUtil;
import lombok.Data;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Scope;
import org.springframework.stereotype.Service;

import java.util.HashMap;
import java.util.List;

@Service
@Scope(value="prototype")
@Slf4j
@Data
public class RpcPipeline {

    private FuseHandler fuseHandler;

    private LimitHandler limitHandler;

    private RouteHandler routeHandler;

    private LoadBalancerHandler loadBalancerHandler;

    @Autowired(required = false)
    private RequestFilterHandler requestFilterHandler;

    @Autowired(required = false)
    private RequestInterceptHandler requestInterceptHandler;

    private ResponseFilterHandler responseFilterHandler;

    private RpcConfigProperties.RpcConsumer rpcConsumer;

    private TraceStrategyContext traceStrategyContext;
    
    private RpcServiceList rpcServiceList;
    @Autowired
    private RpcConfigProperties.RpcProvider rpcProvider;


    @Autowired
    public RpcPipeline(FuseHandler fuseHandler, LimitHandler limitHandler,
                       LoadBalancerHandler loadBalancerHandler,
                       RpcConfigProperties.RpcConsumer rpcConsumer,
                       RouteHandler routeHandler,
                       TraceStrategyContext traceStrategyContext,
                       @Qualifier("consumerServiceList") RpcServiceList rpcServiceList) {
        this.fuseHandler = fuseHandler;
        this.limitHandler = limitHandler;
        this.loadBalancerHandler = loadBalancerHandler;
        this.rpcConsumer = rpcConsumer;
        this.routeHandler = routeHandler;
        this.traceStrategyContext = traceStrategyContext;
        this.rpcServiceList = rpcServiceList;
    }

    /**
     * 熔断器熔断
     * 路由层获取匹配的服务
     * 负责均衡选择最佳服务机器
     * 请求过滤器处理
     * 请求拦截层处理
     * 链路层判断是否需要跟踪处理
     * 返回服务元数据
     * @return
     */
    public ServiceMeta preProcessing(RpcConsumer consumerAnnotation, ConsumeRequest consumeRequest){
        Boolean pass = null;
        String serviceName = consumerAnnotation.serviceName();
        String group = consumerAnnotation.group();
        // 熔断层处理
        if(rpcConsumer.fuseEnable){
            pass = fuseHandler.fuseHandle(consumerAnnotation.serviceName());
            if (!pass) {
                return null;
            }
        }

        ServiceMeta targetService = null;

        // 路由层处理
        List<ServiceMeta> serviceMetas = routeHandler.routeHandle(group,rpcServiceList.getServiceByName(serviceName));

        // 负载均衡处理
        if(rpcConsumer.loadBalanceEnable){
            targetService = loadBalancerHandler.lbHandle(serviceMetas,consumerAnnotation.serviceName(),consumeRequest.getArgs());
        }else {
            targetService = serviceMetas.get(0);
        }
        if(targetService == null){
            log.error("无可用服务");
            return null;
        }

        // 请求过滤器处理
        if(requestFilterHandler != null){
            requestFilterHandler.filterHandler(consumeRequest);
        }

        // 请求拦截层处理
        if (requestInterceptHandler != null) {
            pass = requestInterceptHandler.interceptorHandle(consumeRequest);
            if (pass != null && !pass) {
                return null;
            }
        }

        // 链路层处理,首先判断请求上下文中是否有trace标识，如果有则直接进行链路跟踪
        // 即上一次请求是跟踪的，那么本次调用也需要跟踪，即使没有配置跟踪
        RpcContext context = RpcContext.getContext();
        HashMap<String, Object> attrs = context.getAttrs();
        if (attrs.get("trace") != null && (Boolean) attrs.get("trace")) {
            consumeRequest.setTrace(true);
            // 本次请求的traceId为上次请求的traceId
            attrs.put("sentTime", System.currentTimeMillis());
            attrs.put("spanId", (int)attrs.get("spanId")+1);
            consumeRequest.setRequestContext(attrs);
//            log.info("已自动配置链路跟踪上下文: {}",context.getAttrs());
        }
        else if(rpcConsumer.traceEnable){
            // 开启了跟踪并且本次请求会被采集跟踪，则初始化traceId和第一个Span
            if(traceStrategyContext.handle()){
                attrs.put("trace", true);
                attrs.put("sentTime", System.currentTimeMillis());
                attrs.put("spanId", 0);
                attrs.put("traceId", SnowFlakeUtil.getNextId());
            }
        }
        return targetService;
    }

}
