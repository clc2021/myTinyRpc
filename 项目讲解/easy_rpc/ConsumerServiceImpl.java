package com.rpc.easy_rpc_consumer.service;
import com.rpc.domain.annotation.RpcConsumer;
import com.rpc.domain.pojo.*;
import com.rpc.easy_rpc_consumer.bean.ConsumerContent;
import com.rpc.easy_rpc_govern.config.RpcConfigProperties;
import com.rpc.easy_rpc_govern.context.RpcPipeline;
import com.rpc.easy_rpc_govern.filter.ResponseFilterHandler;
import com.rpc.easy_rpc_govern.fuse.FuseProtector;
import com.rpc.easy_rpc_govern.utils.SnowFlakeUtil;
import com.rpc.easy_rpc_govern.utils.SpringContextUtil;
import com.rpc.easy_rpc_protocol.cach.ConnectCache;
import com.rpc.domain.enumeration.RequestType;
import com.rpc.easy_rpc_protocol.utils.ClusterUtil;
import com.rpc.easy_rpc_protocol.utils.ConnectUtil;
import com.rpc.easy_rpc_protocol.utils.SyncResultUtil;
import io.netty.bootstrap.Bootstrap;
import io.netty.channel.*;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.stereotype.Service;
import javax.annotation.Resource;
import java.lang.reflect.Method;

@Service
@Slf4j
public class ConsumerServiceImpl implements ConsumerService{
    @Autowired
    @Qualifier("consumerBootstrap")
    Bootstrap bootstrap;

    @Resource
    RpcConfigProperties.RpcRegistry registry;

    @Autowired
    @Qualifier("consumerConnectCache")
    ConnectCache connectCache;

    @Resource
    RpcConfigProperties.RpcConsumer consumer;

    @Resource
    FuseProtector fuseProtector;

    @Autowired(required = false)
    ResponseFilterHandler responseFilterHandler;

    @Resource
    RpcPipeline rpcPipeline;

    @Resource
    ConsumerContent consumerContent;

    /**
     *  进行服务列表获取
     * @return
     */
    @Override
    public void getServiceList() {
        RequestHeader header = new RequestHeader(RequestType.REQUEST_SERVICE_LIST, SnowFlakeUtil.getNextId());
        RpcRequestHolder requestHolder = new RpcRequestHolder(header,null);
        // 判断是否为集群模式
        if(registry.getCluster()) {
            // 集群模式下，需要获取集群leader
            ChannelFuture channelFuture = ClusterUtil.getClusterLeader(registry.getClusterAddress(), bootstrap, connectCache);
            if (channelFuture != null) {
                Channel channel = channelFuture.channel();
                channel.writeAndFlush(requestHolder);
                consumerContent.setLeaderAddress(channelFuture.channel().remoteAddress().toString().substring(1));
            }
        }else{
            ChannelFuture channelFuture = ConnectUtil.connect(registry.getHost(),registry.getPort(),bootstrap,connectCache);
            if(channelFuture != null){
                Channel channel = channelFuture.channel();
                channel.writeAndFlush(requestHolder);
                consumerContent.setLeaderAddress(registry.getHost()+":"+registry.getPort());
            }
        }

    }

    @Override
    public Object sendRequest(RpcConsumer rpcConsumer, ConsumeRequest consumeRequest, RequestHeader requestHeader){
        ServiceMeta serviceMeta = rpcPipeline.preProcessing(rpcConsumer, consumeRequest);
        // 如果服务列表为空，那么就执行降级
        if(serviceMeta == null){
            return fallBackHandler(rpcConsumer.fallback(),consumeRequest);
        }
        Long requestId = requestHeader.getRequestId();
        ChannelFuture channelFuture = ConnectUtil.connect(serviceMeta.getServiceHost(), serviceMeta.getServicePort(),bootstrap,connectCache);
        // 连接失败处理
        if(channelFuture == null) {
            log.error("远程服务调用失败！无法连接到服务提供者");
            return null;
        }
        Channel channel = channelFuture.channel();
        try {
            // 构造请求体，发送数据
            RpcRequestHolder rpcRequestHolder = new RpcRequestHolder(requestHeader,consumeRequest);
            SyncResultUtil.setRequestId(requestId);
            channel.writeAndFlush(rpcRequestHolder);
            // 结果获取
            ProviderResponse result = SyncResultUtil.get(requestId, ProviderResponse.class,consumer.getConsumeWaitInMs());
            if (responseFilterHandler != null) {
                responseFilterHandler.filterHandler(result);
            }
            // 如果开启了熔断器，则添加一次成功次数
            if (consumer.getFuseEnable()){
                fuseProtector.incrSuccess(serviceMeta.getServiceName());
            }
            return result == null ? null : result.getResult();
        }catch (Exception e){
            // 如果开启了熔断器，则添加一次异常次数
            if (consumer.getFuseEnable()){
                fuseProtector.incrExcept(serviceMeta.getServiceName());
            }
            log.error("远程服务调用失败！错误信息:{}",e.getMessage());
        }
        return null;
    }

    private Object fallBackHandler(Class<?> fallBack, ConsumeRequest consumeRequest){
        if(fallBack != void.class){
            try {
                Object bean = SpringContextUtil.getBean(fallBack);
                Method method = fallBack.getMethod(consumeRequest.getMethodName(),consumeRequest.getParameterTypes());
                return method.invoke(bean, consumeRequest.getArgs());
            }catch (Exception e){
                log.error("限流降级执行错误!信息：{}",e.getMessage());
            }
        }
        return null;
    }

}
