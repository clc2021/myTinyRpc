package com.rpc.easy_rpc_consumer.proxy;

import com.rpc.domain.enumeration.RequestType;
import com.rpc.easy_rpc_consumer.service.ConsumerService;
import com.rpc.easy_rpc_govern.context.RpcContext;
import com.rpc.domain.annotation.RpcConsumer;
import com.rpc.easy_rpc_govern.utils.SnowFlakeUtil;
import com.rpc.easy_rpc_govern.utils.SpringContextUtil;
import lombok.extern.slf4j.Slf4j;
import com.rpc.domain.pojo.*;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;

@Slf4j
public class ConsumerProxy implements InvocationHandler{

    RpcConsumer rpcConsumer;

    public ConsumerProxy(RpcConsumer rpcConsumer){
        this.rpcConsumer = rpcConsumer;
    }

    /**
     * 核心！
     * 此方法用户真正执行服务请求
     * 1: 构建请求头，设置类型为服务消费类型
     * 2：构建请求体，创建一个ConsumeRequest
     * 3: 根据rpcConsumer 获取指定服务名
     * 4：更加服务名查询本地服务列表然后进行构建ConsumeRequest
     * @param proxy
     * @param method
     * @param args
     * @return
     * @throws Throwable
     */
    @Override
    public Object invoke(Object proxy, Method method, Object[] args) {
        ConsumerService consumerService = SpringContextUtil.getBean(ConsumerService.class);
        RpcContext context = RpcContext.getContext();
        ConsumeRequest consumeRequest = new ConsumeRequest(rpcConsumer.serviceName(),method.getName(),method.getParameterTypes(),args,false,context.getAttrs());
        RequestHeader requestHeader = new RequestHeader(RequestType.CONSUME_SERVICE, SnowFlakeUtil.getNextId(),true);

        return consumerService.sendRequest(rpcConsumer,consumeRequest,requestHeader);
    }



}
