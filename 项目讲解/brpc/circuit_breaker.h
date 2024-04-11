#ifndef BRPC_CIRCUIT_BREAKER_H
#define BRPC_CIRCUIT_BREAKER_H

#include "butil/atomicops.h"

namespace brpc {

// 断路器类
class CircuitBreaker {
public:
    CircuitBreaker();  // 构造函数

    ~CircuitBreaker() {}  // 析构函数

    // 对当前的 RPC 进行采样。如果节点需要被隔离，则返回 false，否则返回 true。
    // error_code：此次调用的错误码，0 表示成功。
    // latency：此次调用的时间消耗。
    // 注意：一旦 OnCallEnd() 确定某个节点需要被隔离，它将始终返回 false，直到调用 Reset()。
    // 通常 Reset() 会在健康检查线程中调用。
    bool OnCallEnd(int error_code, int64_t latency);

    // 重置断路器并清除历史数据。将擦除历史数据并重新开始采样。
    // 在调用此方法之前，您需要确保没有其他人正在访问断路器。
    void Reset();

    // 将 Socket 标记为损坏。当您想提前隔离节点时调用此方法。
    // 当此方法连续调用多次时，只有第一次调用会生效。
    void MarkAsBroken();

    // 标记为隔离的次数
    int isolated_times() const {
        return _isolated_times.load(butil::memory_order_relaxed);
    }

    // Socket 失败时应隔离的持续时间（毫秒）。
    // Socket 错误频率越高，持续时间越长。
    int isolation_duration_ms() const {
        return _isolation_duration_ms.load(butil::memory_order_relaxed);
    }

private:
    // 更新隔离持续时间
    void UpdateIsolationDuration();

    // 指数移动平均错误记录器类
    class EmaErrorRecorder {
    public:
        // 构造函数
        EmaErrorRecorder(int windows_size,  int max_error_percent);

        // 对当前的 RPC 进行采样
        bool OnCallEnd(int error_code, int64_t latency);

        // 重置记录器
        void Reset();

    private:
        // 更新延迟
        int64_t UpdateLatency(int64_t latency);

        // 更新错误成本
        bool UpdateErrorCost(int64_t latency, int64_t ema_latency);

        const int _window_size;  // 窗口大小
        const int _max_error_percent;  // 最大错误百分比
        const double _smooth;  // 平滑参数

        butil::atomic<int32_t> _sample_count_when_initializing;  // 初始化时的样本计数
        butil::atomic<int32_t> _error_count_when_initializing;  // 初始化时的错误计数
        butil::atomic<int64_t> _ema_error_cost;  // 指数移动平均错误成本
        butil::atomic<int64_t> _ema_latency;  // 指数移动平均延迟
    };

    EmaErrorRecorder _long_window;  // 长窗口记录器
    EmaErrorRecorder _short_window;  // 短窗口记录器
    int64_t _last_reset_time_ms;  // 上次重置时间（毫秒）
    butil::atomic<int> _isolation_duration_ms;  // 隔离持续时间
    butil::atomic<int> _isolated_times;  // 隔离次数
    butil::atomic<bool> _broken;  // 是否断开
};

}  // namespace brpc

#endif // BRPC_CIRCUIT_BREAKER_H_
