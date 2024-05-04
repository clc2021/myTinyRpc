package com.rpc.easy_rpc_govern.fuse;

import com.rpc.easy_rpc_govern.config.RpcConfigProperties;
import com.rpc.easy_rpc_govern.utils.SpringContextUtil;
import lombok.Data;
import lombok.extern.slf4j.Slf4j;

import java.util.concurrent.atomic.AtomicInteger;
@Data
@Slf4j
public class ServiceState {
    /**
     * 服务名
     */
    private String serviceName;
    /**
     * 请求数量
     */
    private AtomicInteger request;
    /**
     * 异常的请求数量
     */
    private AtomicInteger excepts;
    /**
     * 当前服务熔断状态
     */
    private FuseState fuseState;

    /**
     * 当前服务拦截lv
     * @param serviceName
     */
    private float interceptRate;

    public ServiceState(String serviceName) {
        this.serviceName = serviceName;
        this.request = new AtomicInteger(0);
        this.excepts = new AtomicInteger(0);
        this.interceptRate = 0;
        this.fuseState = FuseState.CLOSE;
    }

    public void incrRequest(){
        request.incrementAndGet();
        int requestNum = request.incrementAndGet();
        int exceptsNum = excepts.incrementAndGet();
        RpcConfigProperties.RpcConsumer bean = SpringContextUtil.getBean(RpcConfigProperties.RpcConsumer.class);
        if( this.fuseState != FuseState.CLOSE){
            float rate = (requestNum - bean.k*(requestNum - exceptsNum))/(requestNum + 1);
            this.interceptRate = rate;
            if(rate < 0.1f) {
                this.fuseState = FuseState.CLOSE;
            }
        }
    }

    public void incrExcepts(){
        int requestNum = request.incrementAndGet();
        int exceptsNum = excepts.incrementAndGet();
        RpcConfigProperties.RpcConsumer bean = SpringContextUtil.getBean(RpcConfigProperties.RpcConsumer.class);
//        此外拦截率，当拦截率超过0.3时进入半开模式，同时开启拦截模式，即请求会被一定概率拦截，概率为拦截率。
//        k值越大，则越不容易开启拦截，k值越小，则越容易开启拦截
        float rate = (requestNum - bean.k*(requestNum - exceptsNum))/(requestNum + 1);
        this.interceptRate = rate;
        if(rate < 0.1f){
            this.fuseState = FuseState.CLOSE;
        }else if(rate > 0.3f){
            log.info("熔断器当前切换至半开");
            this.fuseState = FuseState.HALF_OPEN;
        }else if(rate > 0.7f){
            log.info("熔断器当前切换至全开");
            this.fuseState = FuseState.FALL_OPEN;
        }


    }
}
