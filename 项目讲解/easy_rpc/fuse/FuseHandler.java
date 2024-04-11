package com.rpc.easy_rpc_govern.fuse;


public interface FuseHandler {
    public boolean fuseHandle(String serviceName);

    public void incrSuccess(String serviceName);

    public void incrExcept(String serviceName);

}
