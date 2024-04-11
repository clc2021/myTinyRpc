package com.rpc.easy_rpc_govern.fuse;


import com.rpc.domain.pojo.ServiceMeta; // ServiceMeta
import lombok.Data;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.annotation.Async; // 异步执行，不会阻塞当前线程。
import org.springframework.scheduling.annotation.Scheduled; // 定时数据同步。
import org.springframework.stereotype.Component;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;


@Component
@Data
@Slf4j
public class FuseProtector implements FuseHandler{
    // ConcurrentHashMap是一个线程安全的哈希表。
    // 在C++中，可以使用 std::unordered_map 或 std::unordered_map 结合互斥量来代替 ConcurrentHashMap。
    private ConcurrentHashMap<String, ServiceState> serviceStateCache = new ConcurrentHashMap<>();
    // 检查当前熔断状态，许可发送请求返回true，拦截返回false
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

    // 初始化服务数据
    public void initCache(ConcurrentHashMap<String, List<ServiceMeta>> serviceList){
        for (String serviceName : serviceList.keySet()) {
            ServiceState serviceState = new ServiceState(serviceName);
                serviceStateCache.put(serviceName,serviceState);
        }
    }

    // 为服务添加一次成功请求次数
    @Async
    @Override
    public void incrSuccess(String serviceName){
        ServiceState serviceState = serviceStateCache.get(serviceName);
        serviceState.incrRequest();
    }

    // 为服务添加一次异常请求次数
    @Override
    public void incrExcept(String serviceName){
        ServiceState serviceState = serviceStateCache.get(serviceName);
        serviceState.incrExcepts();
    }

    // 数据定期刷新，确保全开状态下进行重新检查
    @Scheduled(cron="0/10 * *  * * ? ")
    public void refreshCache(){
        serviceStateCache.replaceAll((n, v) -> new ServiceState(n));
    }
}
