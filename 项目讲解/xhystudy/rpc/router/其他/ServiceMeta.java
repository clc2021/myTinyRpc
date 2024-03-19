package org.xhystudy.rpc.common;

import java.io.Serializable; // 用于标识类的实例可以被序列化
import java.util.Objects; 
// Objects是Java中的一个实用类，提供了一系列静态方法用于对象的处理，比如equals()、hashCode()等。

// 服务元数据
public class ServiceMeta implements Serializable {
    private String serviceName;
    private String serviceVersion;
    private String serviceAddr;
    private int servicePort;
    /**
     * 关于redis注册中心的属性
     */
    private long endTime;
    private String UUID;
    /**
     * 故障转移需要移除不可用服务
     * @param o
     * @return
     */
    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        ServiceMeta that = (ServiceMeta) o;
        return servicePort == that.servicePort &&
                Objects.equals(serviceName, that.serviceName) &&
                Objects.equals(serviceVersion, that.serviceVersion) &&
                Objects.equals(serviceAddr, that.serviceAddr) &&
                Objects.equals(UUID, that.UUID);
    }

    public String getServiceName() {
        return serviceName;
    }

    public void setServiceName(String serviceName) {
        this.serviceName = serviceName;
    }

    public String getServiceVersion() {
        return serviceVersion;
    }

    public void setServiceVersion(String serviceVersion) {
        this.serviceVersion = serviceVersion;
    }

    public String getServiceAddr() {
        return serviceAddr;
    }

    public void setServiceAddr(String serviceAddr) {
        this.serviceAddr = serviceAddr;
    }

    public int getServicePort() {
        return servicePort;
    }

    public void setServicePort(int servicePort) {
        this.servicePort = servicePort;
    }

    public long getEndTime() {
        return endTime;
    }

    public void setEndTime(long endTime) {
        this.endTime = endTime;
    }

    public String getUUID() {
        return UUID;
    }

    public void setUUID(String UUID) {
        this.UUID = UUID;
    }

    // UUID（通用唯一标识符）是一个标准的128位数字（16个字节）的格式，用于唯一地标识信息。
    @Override
    public int hashCode() {
        return Objects.hash(serviceName, serviceVersion, serviceAddr, servicePort, UUID);
    }
}
