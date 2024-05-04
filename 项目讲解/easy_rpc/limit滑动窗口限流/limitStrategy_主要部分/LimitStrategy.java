package com.rpc.easy_rpc_govern.limit.limitStrategy;

public interface LimitStrategy {
    // 获取通过的数量
    public int getPassCount(long time);

    // 根据Key获取通过的数量
    public int getPassCount(long time,Object obj);

    // 获取当前QPS
    public int getQPS();

    // 获取指定key的QPS
    public int getQPS(Object obj);

    // 添加通过数量
    public void incrPassCount();

    // 添加通过数量
    public void incrPassCount(Object obj);

    // 添加拒绝数量
    public void incrBlockCount();

    // 添加拒绝数量
    public void incrBlockCount(Object obj);
}
