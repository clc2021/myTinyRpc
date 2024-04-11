package com.rpc.easy_rpc_consumer.netServer.handler;

import com.rpc.easy_rpc_consumer.bean.ConsumerContent;
import com.rpc.easy_rpc_govern.utils.SnowFlakeUtil;
import com.rpc.easy_rpc_protocol.cach.ConnectCache;
import com.rpc.easy_rpc_govern.config.RpcConfigProperties;
import com.rpc.domain.pojo.RpcServiceList;
import com.rpc.domain.pojo.*;
import com.rpc.easy_rpc_govern.fuse.FuseProtector;
import com.rpc.domain.enumeration.RequestType;
import com.rpc.easy_rpc_protocol.utils.ClusterUtil;
import com.rpc.easy_rpc_protocol.utils.ConnectUtil;
import com.rpc.easy_rpc_protocol.utils.SyncResultUtil;
import io.netty.bootstrap.Bootstrap;
import io.netty.channel.*;
import io.netty.util.ReferenceCountUtil;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.stereotype.Component;
import javax.annotation.Resource;
import java.util.List;
import java.util.stream.Collectors;

@ChannelHandler.Sharable
@Component
@Slf4j
public class ConsumerHandler extends ChannelInboundHandlerAdapter {

    @Autowired
    @Qualifier("consumerServiceList")
    RpcServiceList rpcServiceList;

    @Resource
    FuseProtector fuseProtector;

    @Resource
    @Qualifier("consumerConnectCache")
    ConnectCache connectCache;

    @Resource
    @Qualifier("consumerBootstrap")
    Bootstrap bootstrap;

    @Resource
    RpcConfigProperties.RpcRegistry rpcRegistry;

    @Resource
    ConsumerContent consumerContent;

    /**
     * 这里进行服务提供者返回数据接收
     * 将接受到的数据放入响应缓存中，方便在service层中获取响应数据
     * @throws Exception
     */
    @Override
    public void channelRead(ChannelHandlerContext ctx, Object msg){

        try {
            RpcRequestHolder requestHolder = (RpcRequestHolder) msg;
            RequestHeader requestHeader = requestHolder.getRequestHeader();
            RequestType type = requestHeader.getType();
            Long requestId = requestHeader.getRequestId();
            // 服务消费响应处理
            if(type.equals(RequestType.PROVIDE_SERVICE)){
                SyncResultUtil.setResult(requestId, requestHolder.getData());
            }

            // 服务列表添加
            else if(type.equals(RequestType.ADD_SERVICE_LIST)){
                List<ServiceMeta> serviceMeta = (List) requestHolder.getData();
                for (ServiceMeta meta : serviceMeta) {
                    rpcServiceList.addServiceMeta(meta);
                }
                log.info("成功添加新的服务："+serviceMeta.stream().map(ServiceMeta::getServiceName).collect(Collectors.toList()));
                fuseProtector.initCache(rpcServiceList.getServiceList());
            }
            // 服务列表删除
            else if(type.equals(RequestType.REMOVE_SERVICE_LIST)){
                List<ServiceMeta> serviceMeta = (List) requestHolder.getData();
                for (ServiceMeta meta : serviceMeta) {
                    rpcServiceList.removeServiceMeta(meta);
                }
                log.info("成功删除服务："+serviceMeta.stream().map(ServiceMeta::getServiceName).collect(Collectors.toList()));
                fuseProtector.initCache(rpcServiceList.getServiceList());
            }
            // 获取leader地址
            else if(type.equals(RequestType.RESPONSE_LEADER_ADDRESS)){
                String clusterLeaderAddress = (String) requestHolder.getData();
                SyncResultUtil.setResult(requestId,clusterLeaderAddress);
            }
        }finally {
            if (msg != null) {
                ReferenceCountUtil.release(msg);
            }
        }
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, Throwable cause) {
        log.error("远程服务调用失败！错误信息:{}",cause.getMessage());
        cause.printStackTrace();
        ctx.close();
    }

    @Override
    public void channelInactive(ChannelHandlerContext ctx) throws Exception {
        String disConnectAddress = ctx.channel().remoteAddress().toString().substring(1);
        connectCache.removeChannelFuture(disConnectAddress);
        log.error("注册中心连接断开，正在重新连接...");
        RequestHeader header = new RequestHeader(RequestType.REQUEST_SERVICE_LIST, SnowFlakeUtil.getNextId());
        RpcRequestHolder requestHolder = new RpcRequestHolder(header,null);
        ChannelFuture channelFuture = null;
        String leaderAddress = consumerContent.getLeaderAddress();
        if (leaderAddress != null && leaderAddress.equals(disConnectAddress)){
            // 如果断开的是leader节点，那么就将leaderAddress置空，等待重新获取
            consumerContent.setLeaderAddress(null);
            while (true){
                // 判断是否为集群环境
                if(rpcRegistry.getCluster()){
                    // 集群模式下，需要获取集群leader
                    channelFuture = ClusterUtil.getClusterLeader(rpcRegistry.getClusterAddress(), bootstrap, connectCache);
                }else{
                    // 单机模式则直接连接注册中心
                    channelFuture = ConnectUtil.connect(rpcRegistry.getHost(),rpcRegistry.getPort(),bootstrap,connectCache);
                }
                if(channelFuture != null){
                    Channel channel = channelFuture.channel();
                    channel.writeAndFlush(requestHolder);
                    if (channelFuture.isSuccess()){
                        log.info("注册中心连接成功！");
                        consumerContent.setLeaderAddress(disConnectAddress);
                        return;
                    }
                }
                Thread.sleep(3000L);
            }
        }
    }
}
