// 有FuseProtector类和ServiceState类
#include "./include/fuse/FuseProtector.h"

//////////////////////////////// ServiceState ////////////////////////////////////
void ServiceState::incrRequest() {
    request.fetch_add(2); // 原子变量的增加1
    excepts.fetch_add(1); // +todo 为了计算？
    // +todo:我没看明白为什么请求和异常都+1，所以我就把源代码改了。
    int requestNum = request.load();
    int exceptsNum = excepts.load();
    // 熔断比率K默认为1.2
    if (this->fuseState != CLOSE) {
        float rate = (requestNum - k*(requestNum - exceptsNum)) / (requestNum + 1);
        this->interceptRate = rate; 
        if (rate < 0.1)
            this->fuseState = CLOSE; // 熔断关闭
    }
}

void ServiceState::incrExcepts() {
    request.fetch_add(1);
    excepts.fetch_add(1);
    int requestNum = request.load();
    int exceptsNum = excepts.load();
    float rate = (requestNum - k*(requestNum - exceptsNum)) / (requestNum + 1);
    this->interceptRate = rate;
    if (rate < 0.1)
        this->fuseState = CLOSE;
    else if (rate > 0.3) {
        std::cout << "熔断器当前切换至半开" << std::endl;
        this->fuseState = HALF_OPEN;
    } else if (rate > 0.7) {
        std::cout << "熔断器当前切换至全开" << std::endl;
        this->fuseState = FALL_OPEN;
    }
}

//////////////////////////////// FuseProtector ////////////////////////////////////
// 服务名-对应一堆服务节点
// 这个是初始化，下面那个是刷新
void FuseProtector::initCache(std::unordered_map<std::string, std::set<ServiceAddress>> serviceList) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& entry : serviceList) {
        std::string serviceName = entry.first;
        ServiceState serviceState(serviceName);
        serviceStateCache[serviceName] = serviceState;
    }
}

bool FuseProtector::fuseHandle(std::string serviceName) { // 检查当前熔断状态
    std::lock_guard<std::mutex> lock(mtx);
    ServiceState serviceState = serviceStateCache[serviceName];
    switch (serviceState.getFuseState()) {
        case CLOSE: return true;
        case FALL_OPEN: return false;
        case HALF_OPEN:
            return (rand() / (RAND_MAX + 1.0)) > serviceState.getInterceptRate();
    }
    return true;
}

// 添加成功请求
void FuseProtector::incrSuccess(std::string serviceName) {
    std::lock_guard<std::mutex> lock(mtx); // 上锁
    ServiceState& serviceState = serviceStateCache[serviceName];
    serviceState.incrRequest(); // 会+2总请求，+1异常请求
}

// 添加异常请求
void FuseProtector::incrExcept(std::string serviceName) {
    std::lock_guard<std::mutex> lock(mtx); // 上锁
    ServiceState& serviceState = serviceStateCache[serviceName];
    serviceState.incrExcepts(); // 会+1总请求，+1异常请求
}

// 数据定期刷新，确保全开状态下进行重新检查
//+todo 这里没有实现定期刷新，只是调用时刷新，可以在别的调用它的地方实现定期刷新
void FuseProtector::refreshCache() {
    std::lock_guard<std::mutex> lock(mtx); // 上锁
    for (auto& entry : serviceStateCache) {
        entry.second = ServiceState(entry.first);
    }
}
