package com.rpc.easy_rpc_govern.fuse;


import com.rpc.domain.pojo.ServiceMeta;
import lombok.Data;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.annotation.Async;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;


@Component
@Data
@Slf4j
public class FuseProtector implements FuseHandler{

    private ConcurrentHashMap<String, ServiceState> serviceStateCache = new ConcurrentHashMap<>();

    /**
     * 检查当前熔断状态，许可发送请求返回true，拦截返回false
     * @param
     * @return
     */
    public boolean fuseHandle(String serviceName){
        ServiceState serviceState = serviceStateCache.get(serviceName);
        switch (serviceState.getFuseState()) {
            case CLOSE:
                return true;
            case FALL_OPEN:
                return false;
            case HALF_OPEN:
                return Math.random() > serviceState.getInterceptRate();
        }
        return true;
    }

    /**
     * 初始化服务数据
     * @param serviceList
     */
    public void initCache(ConcurrentHashMap<String, List<ServiceMeta>> serviceList){
        for (String serviceName : serviceList.keySet()) {
            ServiceState serviceState = new ServiceState(serviceName);
                serviceStateCache.put(serviceName,serviceState);
        }
    }

    /**
     * 为服务添加一次成功请求次数
     * @param serviceName
     */
    @Async
    @Override
    public void incrSuccess(String serviceName){
        ServiceState serviceState = serviceStateCache.get(serviceName);
        serviceState.incrRequest();
    }

    /**
     * 为服务添加一次异常请求次数
     * @param serviceName
     */
    @Override
    public void incrExcept(String serviceName){
        ServiceState serviceState = serviceStateCache.get(serviceName);
        serviceState.incrExcepts();
    }

    /**
     * 数据定期刷新，确保全开状态下进行重新检查
     */
    @Scheduled(cron="0/10 * *  * * ? ")
    public void refreshCache(){
        serviceStateCache.replaceAll((n, v) -> new ServiceState(n));
    }
}
