package com.rpc.easy_rpc_govern.filter;

import com.rpc.domain.pojo.ConsumeRequest;

/**
 * 调用拦截器
 */
public interface RequestFilterHandler {
    public void filterHandler(ConsumeRequest consumeRequest);
}
