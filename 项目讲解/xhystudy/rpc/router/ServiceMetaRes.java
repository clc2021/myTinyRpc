package org.xhystudy.rpc.router;

import org.xhystudy.rpc.common.ServiceMeta;

import java.util.ArrayList; // 动态数组
import java.util.Collection; // 集合

// !!!! 区分 ServiceMeta 和 ServiceMetaRes !!!!!!!
public class ServiceMetaRes { // 这是ServiceMetaRes 也就是一个结果集合
    // 当前服务节点：是一个服务节点类的对象ServiceMetaRes
    private ServiceMeta curServiceMeta;
    // 剩余服务节点：是一个服务节点类的集合set<ServiceMetaRes>
    private Collection<ServiceMeta> otherServiceMeta;

    public Collection<ServiceMeta> getOtherServiceMeta() { // 获取剩余服务节点的方法
        return otherServiceMeta;
    }

    public ServiceMeta getCurServiceMeta() { // 获取当前服务节点的方法
        return curServiceMeta;
    }

    // build方法：传入当前服务节点，其他服务节点集合
    // 其实这个build()我觉得就很像ServiceMeataRes的构造函数
    public static ServiceMetaRes build(ServiceMeta curServiceMeta, Collection<ServiceMeta> otherServiceMeta){
        final ServiceMetaRes serviceMetaRes = new ServiceMetaRes(); // 新建ServiceMetaRes
        serviceMetaRes.curServiceMeta = curServiceMeta;
        // 如果只有一个服务
        if(otherServiceMeta.size() == 1){
            otherServiceMeta = new ArrayList<>();
        }else{
            otherServiceMeta.remove(curServiceMeta);
        }
        serviceMetaRes.otherServiceMeta = otherServiceMeta;
        return serviceMetaRes;
    }

}
