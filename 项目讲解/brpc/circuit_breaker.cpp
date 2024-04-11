#include "brpc/circuit_breaker.h"

#include <cmath>
#include <gflags/gflags.h>

#include "brpc/errno.pb.h"
#include "butil/time.h"

namespace brpc {

// 定义断路器短窗口大小，默认为1500
DEFINE_int32(circuit_breaker_short_window_size, 1500,
    "Short window sample size.");
// 定义断路器长窗口大小，默认为3000
DEFINE_int32(circuit_breaker_long_window_size, 3000,
    "Long window sample size.");
// 定义断路器短窗口最大错误率，范围为0-99，默认为10
DEFINE_int32(circuit_breaker_short_window_error_percent, 10,
    "The maximum error rate allowed by the short window, ranging from 0-99.");
// 定义断路器长窗口最大错误率，范围为0-99，默认为5
DEFINE_int32(circuit_breaker_long_window_error_percent, 5,
    "The maximum error rate allowed by the long window, ranging from 0-99.");
// 定义断路器最小错误成本，当错误成本的指数移动平均小于此值时，将其设为零，默认为500
DEFINE_int32(circuit_breaker_min_error_cost_us, 500,
    "The minimum error_cost, when the ema of error cost is less than this "
    "value, it will be set to zero.");
// 定义断路器最大失败延迟倍数，默认为2
DEFINE_int32(circuit_breaker_max_failed_latency_mutiple, 2,
    "The maximum multiple of the latency of the failed request relative to "
    "the average latency of the success requests.");
// 定义断路器最小隔离持续时间，单位为毫秒，默认为100
DEFINE_int32(circuit_breaker_min_isolation_duration_ms, 100,
    "Minimum isolation duration in milliseconds");
// 定义断路器最大隔离持续时间，单位为毫秒，默认为30000
DEFINE_int32(circuit_breaker_max_isolation_duration_ms, 30000,
    "Maximum isolation duration in milliseconds");
// 定义断路器ε值，默认为0.02
DEFINE_double(circuit_breaker_epsilon_value, 0.02,
    "ema_alpha = 1 - std::pow(epsilon, 1.0 / window_size)");

namespace {
// EPSILON 用于在计算 EMA 时生成平滑系数。
// EPSILON 越大，平滑系数就越大，这意味着早期数据的比例更大。
// smooth = pow(EPSILON, 1 / window_size)
// 例如，当 window_size = 100 时，
// EPSILON = 0.1，smooth = 0.9772
// EPSILON = 0.3，smooth = 0.9880
// 当 window_size = 1000 时，
// EPSILON = 0.1，smooth = 0.9977
// EPSILON = 0.3，smooth = 0.9987
#define EPSILON (FLAGS_circuit_breaker_epsilon_value)

}  // namespace

// 断路器 EmaErrorRecorder 类的构造函数
CircuitBreaker::EmaErrorRecorder::EmaErrorRecorder(int window_size,
                                                   int max_error_percent)
    : _window_size(window_size)
    , _max_error_percent(max_error_percent)
    , _smooth(std::pow(EPSILON, 1.0/window_size))
    , _sample_count_when_initializing(0)
    , _error_count_when_initializing(0)
    , _ema_error_cost(0)
    , _ema_latency(0) {
}

// 对当前的 RPC 进行采样
bool CircuitBreaker::EmaErrorRecorder::OnCallEnd(int error_code,
                                                 int64_t latency) {
    int64_t ema_latency = 0;
    bool healthy = false;
    if (error_code == 0) {
        ema_latency = UpdateLatency(latency);
        healthy = UpdateErrorCost(0, ema_latency);
    } else {
        ema_latency = _ema_latency.load(butil::memory_order_relaxed);
        healthy = UpdateErrorCost(latency, ema_latency);
    }

    // 当窗口正在初始化时，使用错误率来确定是否需要隔离。
    if (_sample_count_when_initializing.load(butil::memory_order_relaxed) < _window_size &&
        _sample_count_when_initializing.fetch_add(1, butil::memory_order_relaxed) < _window_size) {
        if (error_code != 0) {
            const int32_t error_count =
                _error_count_when_initializing.fetch_add(1, butil::memory_order_relaxed);
            return error_count < _window_size * _max_error_percent / 100;
        }
        // 因为一旦 OnCallEnd 返回 false，节点很快就会被隔离，
        // 所以当 error_code=0 时，我们不再检查错误计数。
        return true;
    }

    return healthy;
}

// 重置记录器
void CircuitBreaker::EmaErrorRecorder::Reset() {
    if (_sample_count_when_initializing.load(butil::memory_order_relaxed) < _window_size) {
        _sample_count_when_initializing.store(0, butil::memory_order_relaxed);
        _error_count_when_initializing.store(0, butil::memory_order_relaxed);
        _ema_latency.store(0, butil::memory_order_relaxed);
    }
    _ema_error_cost.store(0, butil::memory_order_relaxed);
}

// 更新延迟
int64_t CircuitBreaker::EmaErrorRecorder::UpdateLatency(int64_t latency) {
    int64_t ema_latency = _ema_latency.load(butil::memory_order_relaxed);
    do {
        int64_t next_ema_latency = 0;
        if (0 == ema_latency) {
            next_ema_latency = latency;
        } else {
            next_ema_latency = ema_latency * _smooth + latency * (1 - _smooth);
        }
        if (_ema_latency.compare_exchange_weak(ema_latency, next_ema_latency)) {
            return next_ema_latency;
        }
    } while(true);
}

// 更新错误成本
bool CircuitBreaker::EmaErrorRecorder::UpdateErrorCost(int64_t error_cost,
                                                       int64_t ema_latency) {
    const int max_mutiple = FLAGS_circuit_breaker_max_failed_latency_mutiple;
    if (ema_latency != 0) {
        error_cost = std::min(ema_latency * max_mutiple, error_cost);
    }
    // 错误响应
    if (error_cost != 0) {
        int64_t ema_error_cost =
            _ema_error_cost.fetch_add(error_cost, butil::memory_order_relaxed);
        ema_error_cost += error_cost;
        const int64_t max_error_cost =
            ema_latency * _window_size * (_max_error_percent / 100.0) * (1.0 + EPSILON);
        return ema_error_cost <= max_error_cost;
    }

    // 普通响应
    int64_t ema_error_cost = _ema_error_cost.load(butil::memory_order_relaxed);
    do {
        if (ema_error_cost == 0) {
            break;
        } else if (ema_error_cost < FLAGS_circuit_breaker_min_error_cost_us) {
            if (_ema_error_cost.compare_exchange_weak(
                ema_error_cost, 0, butil::memory_order_relaxed)) {
                break;
            }
        } else {
            int64_t next_ema_error_cost = ema_error_cost * _smooth;
            if (_ema_error_cost.compare_exchange_weak(
                ema_error_cost, next_ema_error_cost)) {
                break;
            }
        }
    } while (true);
    return true;
}

// 断路器的构造函数
CircuitBreaker::CircuitBreaker()
    : _long_window(FLAGS_circuit_breaker_long_window_size,
                   FLAGS_circuit_breaker_long_window_error_percent)
    , _short_window(FLAGS_circuit_breaker_short_window_size,
                    FLAGS_circuit_breaker_short_window_error_percent)
    , _last_reset_time_ms(0)
    , _isolation_duration_ms(FLAGS_circuit_breaker_min_isolation_duration_ms)
    , _isolated_times(0)
    , _broken(false) {
}

// 对 RPC 调用结束的处理
bool CircuitBreaker::OnCallEnd(int error_code, int64_t latency) {
    // 如果服务器达到最大并发数，当新请求到达时将直接返回 ELIMIT。
    // 这通常意味着整个下游集群过载。
    // 如果在此时隔离节点，可能会增加下游的压力。
    // 另一方面，由于 ELIMIT 对应的延迟通常非常小，我们不能将其视为成功的请求。
    // 这里我们简单地忽略返回 ELIMIT 的请求。
    if (error_code == ELIMIT) {
        return true;
    }
    if (_broken.load(butil::memory_order_relaxed)) {
        return false;
    }
    if (_long_window.OnCallEnd(error_code, latency) &&
        _short_window.OnCallEnd(error_code, latency)) {
        return true;
    }
    MarkAsBroken();
    return false;
}

// 重置断路器
void CircuitBreaker::Reset() {
    _long_window.Reset();
    _short_window.Reset();
    _last_reset_time_ms = butil::cpuwide_time_ms();
    _broken.store(false, butil::memory_order_release);
}

// 标记节点为断开
void CircuitBreaker::MarkAsBroken() {
    if (!_broken.exchange(true, butil::memory_order_acquire)) {
        _isolated_times.fetch_add(1, butil::memory_order_relaxed);
        UpdateIsolationDuration();
    }
}

// 更新隔离持续时间
void CircuitBreaker::UpdateIsolationDuration() {
    int64_t now_time_ms = butil::cpuwide_time_ms();
    int isolation_duration_ms = _isolation_duration_ms.load(butil::memory_order_relaxed);
    const int max_isolation_duration_ms =
        FLAGS_circuit_breaker_max_isolation_duration_ms;
    const int min_isolation_duration_ms =
        FLAGS_circuit_breaker_min_isolation_duration_ms;
    if (now_time_ms - _last_reset_time_ms < max_isolation_duration_ms) {
        isolation_duration_ms =
            std::min(isolation_duration_ms * 2, max_isolation_duration_ms);
    } else {
        isolation_duration_ms = min_isolation_duration_ms;
    }
    _isolation_duration_ms.store(isolation_duration_ms, butil::memory_order_relaxed);
}

}  // namespace brpc
