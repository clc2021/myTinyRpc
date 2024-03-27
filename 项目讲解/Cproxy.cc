// 1.获取负载均衡策略
// 这里的loadBalancerType是RpcInvokerProxy类的私有变量
LoadBalancer* loadBalancer = LoadBalancerFactory::get(loadBalancerType);
// 2.根据策略获取对应服务
// 这里返回的是当前节点+剩余服务节点，是为了容错的一个包装。
ServiceMetaRes serviceMetaRes = loadBalancer->select(params, serviceName);
ServiceMeta curServiceMeta = serviceMetaRes.getCurServiceMeta();
const std::vector<ServiceMeta>& otherServiceMeta = serviceMetaRes.getOtherServiceMeta();
long count = 1;
long retryCount = this->retryCount;
RpcResponse rpcResponse = nullptr;
// 重试机制
while (count <= retryCount) {
    // 处理返回数据
    RpcFuture<RpcResponse>* future = new RpcFuture<RpcResponse>(new DefaultPromise<DefaultEventLoop>(), timeout);
    // XXXHolder
    RpcRequestHolder::REQUEST_MAP.put(requestId, future);
    try {
        // 发送消息
        rpcConsumer.sendRequest(protocol, curServiceMeta);
        // 等待响应数据返回
        rpcResponse = future->getPromise()->get(future->getTimeout(), TimeUnit::MILLISECONDS);
        // 如果有异常并且没有其他服务
        if (rpcResponse.getException() != nullptr && otherServiceMeta.size() == 0) {
            throw rpcResponse.getException();
        }
        if (rpcResponse.getException() != nullptr) {
            throw rpcResponse.getException();
        }
        log.info("rpc 调用成功, serviceName: {}", serviceName);
        try {
            FilterConfig::getClientAfterFilterChain()->doFilter(filterData);
        }
        catch (std::exception& e) {
            throw e;
        }
        return rpcResponse.getData();
    }
    catch (std::exception& e) {
        std::string errorMsg = e.what();
        // todo 这里的容错机制可拓展,留作业自行更改
        switch (faultTolerantType) {
            // 快速失败
            case FailFast:
                log.warn("rpc 调用失败,触发 FailFast 策略,异常信息: {}", errorMsg);
                return rpcResponse.getException();
            // 故障转移
            case Failover:
                log.warn("rpc 调用失败,第{}次重试,异常信息:{}", count, errorMsg);
                count++;
                if (!ObjectUtils::isEmpty(otherServiceMeta)) {
                    const ServiceMeta& next = otherServiceMeta.begin();
                    curServiceMeta = next;
                    otherServiceMeta.erase(next);
                }
                else {
                    const std::string msg = String::format("rpc 调用失败,无服务可用 serviceName: {%s}, 异常信息: {%s}", serviceName, errorMsg);
                    log.warn(msg);
                    throw std::runtime_error(msg);
                }
                break;
            // 忽视这次错误
            case Failsafe:
                return nullptr;
        }
    }
}
throw std::runtime_error("rpc 调用失败，超过最大重试次数");
