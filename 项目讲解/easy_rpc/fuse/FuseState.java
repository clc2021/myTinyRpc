package com.rpc.easy_rpc_govern.fuse;

public enum FuseState {
    /**
     * 全开模式，所有请求拦截，直至下一个时间片刷新
     */
    FALL_OPEN(),
    /**
     * 半开模式，有一定概率拦截，且不成功越多，拦截越多
     */
    HALF_OPEN(),
    /**
     * 关闭熔断器，不拦截
     */
    CLOSE()
}
