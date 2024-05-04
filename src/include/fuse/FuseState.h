#ifndef FUSE_STATE_H
#define FUSE_STATE_H

enum FUSE_STATE_CODE{
    FULL_OPEN, // 全开模式，所有请求拦截，直至下一个时间片刷新
    HALF_OPEN, // 半开模式，有一定概率拦截，且不成功越多，拦截越多
    CLOSE // 关闭熔断器，不拦截
};

#endif // 