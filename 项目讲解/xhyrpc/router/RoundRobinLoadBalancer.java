package org.xhystudy.rpc.router;

import org.xhystudy.rpc.common.ServiceMeta;
import org.xhystudy.rpc.common.constants.LoadBalancerRules;
import org.xhystudy.rpc.config.RpcProperties;
import org.xhystudy.rpc.registry.RegistryService;
import org.xhystudy.rpc.spi.ExtensionLoader;

import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

/* 轮询算法 */

public class RoundRobinLoadBalancer implements LoadBalancer {

    private static AtomicInteger roundRobinId = new AtomicInteger(0); // 内部的一个静态变量

    @Override
    public ServiceMetaRes select(Object[] params, String serviceName) {
        // 获取注册中心 ExtensionLoader是SPI,=>.get() 选择注册类型
        // RegistryService是有注册 取消注册和discoveries()的注册中心接口
        RegistryService registryService = ExtensionLoader.getInstance().get(RpcProperties.getInstance().getRegisterType());
        List<ServiceMeta> discoveries = registryService.discoveries(serviceName); // 发现的所有服务元数据链表
        // 1.获取所有服务
        int size = discoveries.size();
        // 2.根据当前轮询ID取余服务长度得到具体服务
        roundRobinId.addAndGet(1);
        if (roundRobinId.get() == Integer.MAX_VALUE){
            roundRobinId.set(0);
        }
        
        return ServiceMetaRes.build(discoveries.get(roundRobinId.get() % size),discoveries); 
    }

}
