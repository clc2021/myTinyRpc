package org.xhystudy.rpc.registry;

import org.apache.curator.framework.CuratorFramework;
import org.apache.curator.framework.CuratorFrameworkFactory;
import org.apache.curator.retry.ExponentialBackoffRetry;
import org.apache.curator.x.discovery.ServiceDiscovery;
import org.apache.curator.x.discovery.ServiceDiscoveryBuilder; // 这两个都是服务发现的接口
import org.apache.curator.x.discovery.ServiceInstance;
import org.apache.curator.x.discovery.details.JsonInstanceSerializer;
import org.xhystudy.rpc.common.RpcServiceNameBuilder;
import org.xhystudy.rpc.common.ServiceMeta;
import org.xhystudy.rpc.config.RpcProperties;

import java.io.IOException;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

/**
zk注册中心
 */
public class ZookeeperRegistry implements RegistryService {

    // 连接失败等待重试时间
    public static final int BASE_SLEEP_TIME_MS = 1000;
    // 重试次数
    public static final int MAX_RETRIES = 3;
    // 跟路径
    public static final String ZK_BASE_PATH = "/xhy_rpc";

    private final ServiceDiscovery<ServiceMeta> serviceDiscovery;

    /**
     * 启动zk
     * @throws Exception
     */
    public ZookeeperRegistry() throws Exception {
        String registerAddr = RpcProperties.getInstance().getRegisterAddr();
        CuratorFramework client = CuratorFrameworkFactory.newClient(registerAddr, new ExponentialBackoffRetry(BASE_SLEEP_TIME_MS, MAX_RETRIES));
        client.start();
        JsonInstanceSerializer<ServiceMeta> serializer = new JsonInstanceSerializer<>(ServiceMeta.class);
        this.serviceDiscovery = ServiceDiscoveryBuilder.builder(ServiceMeta.class)
                .client(client)
                .serializer(serializer)
                .basePath(ZK_BASE_PATH)
                .build();
        this.serviceDiscovery.start();
    }



    /**
     * 服务注册
     * @param serviceMeta 服务数据
     * @throws Exception
     */
    @Override
    public void register(ServiceMeta serviceMeta) throws Exception { // 注册是注册ServiceMeta类型的数据
        ServiceInstance<ServiceMeta> serviceInstance = ServiceInstance
                .<ServiceMeta>builder()
                .name(RpcServiceNameBuilder.buildServiceKey(serviceMeta.getServiceName(), serviceMeta.getServiceVersion()))
                .address(serviceMeta.getServiceAddr())
                .port(serviceMeta.getServicePort())
                .payload(serviceMeta)
                .build();
        serviceDiscovery.registerService(serviceInstance);
    }

    /**
     * 服务注销
     * @param serviceMeta
     * @throws Exception
     */
    @Override
    public void unRegister(ServiceMeta serviceMeta) throws Exception {
        ServiceInstance<ServiceMeta> serviceInstance = ServiceInstance
                .<ServiceMeta>builder()
                .name(serviceMeta.getServiceName())
                .address(serviceMeta.getServiceAddr())
                .port(serviceMeta.getServicePort())
                .payload(serviceMeta)
                .build();
        serviceDiscovery.unregisterService(serviceInstance);
    }



    private List<ServiceMeta> listServices(String serviceName) throws Exception {
        Collection<ServiceInstance<ServiceMeta>> serviceInstances = serviceDiscovery.queryForInstances(serviceName);
        List<ServiceMeta> serviceMetas = serviceInstances.stream().map(serviceMetaServiceInstance -> serviceMetaServiceInstance.getPayload()).collect(Collectors.toList());
        return serviceMetas;
    }
    @Override
    public List<ServiceMeta> discoveries(String serviceName) {
        try {
            return listServices(serviceName); // 也就是说discoveries()会返回list<ServiceMeta>
        } catch (Exception e) {
            e.printStackTrace();
        }
        return Collections.EMPTY_LIST;
    }

    /**
     * 关闭
     * @throws IOException
     */
    @Override
    public void destroy() throws IOException {
        serviceDiscovery.close();
    }

}
