package org.xhystudy.rpc.router;

import com.sun.org.apache.xalan.internal.xsltc.dom.ExtendedSAX;
import org.xhystudy.rpc.common.ServiceMeta;
import org.xhystudy.rpc.common.constants.LoadBalancerRules;
import org.xhystudy.rpc.config.RpcProperties;
import org.xhystudy.rpc.registry.RegistryService;
import org.xhystudy.rpc.spi.ExtensionLoader;

import java.util.List;
import java.util.Map; // 接口类
import java.util.TreeMap; // 实现类；相当于map 或 multimap

// 一致性哈希：ConsistentHash 

public class ConsistentHashLoadBalancer implements LoadBalancer {

    // 物理节点映射的虚拟节点,为了解决哈希倾斜
    private final static int VIRTUAL_NODE_SIZE = 10;  // 每个物理节点映射到了10个虚拟节点上
    private final static String VIRTUAL_NODE_SPLIT = "$";

    @Override
    public ServiceMetaRes select(Object[] params, String serviceName) {
        // 获取注册中心：获取类型为ServiceMeta链表的discoveries
        RegistryService registryService = ExtensionLoader.getInstance().get(RpcProperties.getInstance().getRegisterType());
        List<ServiceMeta> discoveries = registryService.discoveries(serviceName);

        final ServiceMeta curServiceMeta = allocateNode(makeConsistentHashRing(discoveries), params[0].hashCode());
        return ServiceMetaRes.build(curServiceMeta,discoveries);
    }
    
    // allocateNode()根据哈希值得到它在环上的真实位置
    private ServiceMeta allocateNode(TreeMap<Integer, ServiceMeta> ring, int hashCode) {
        // 获取最近的哈希环上节点位置
        Map.Entry<Integer, ServiceMeta> entry = ring.ceilingEntry(hashCode);
        if (entry == null) {
            // 如果没有找到则使用最小的节点
            entry = ring.firstEntry();
        }
        return entry.getValue();
    }

    // makeConsistentHashRing()会根据传入的ServiceMeta链表，返回哈希环
    // 哈希环ring的长相：
    // {ip1:port1的哈希值1-ServiceMeta1, ip1:port1的哈希值2-ServiceMeta1, ... , ip1:port1的哈希值10-ServiceMeta1,
    // 另一个地址的哈希值1-服务元数据 ~~~~ 另一个地址的哈希值10-服务元数据}
    // 即：多个哈希值映射到一个服务上
    private TreeMap<Integer, ServiceMeta> makeConsistentHashRing(List<ServiceMeta> servers) {
        TreeMap<Integer, ServiceMeta> ring = new TreeMap<>(); // 要返回的ring
        for (ServiceMeta instance : servers) {
            for (int i = 0; i < VIRTUAL_NODE_SIZE; i++) {
                // "127.0.0.1:8080$1" 
                ring.put((buildServiceInstanceKey(instance) + VIRTUAL_NODE_SPLIT + i).hashCode(), instance);
            }
        }
        return ring;
    }

    // 根据服务实例信息构建缓存键：根据服务元数据返回字符串型的ip:port eg."127.0.0.1:8080"
    private String buildServiceInstanceKey(ServiceMeta serviceMeta) {
        return String.join(":", serviceMeta.getServiceAddr(), String.valueOf(serviceMeta.getServicePort()));
    }
}
