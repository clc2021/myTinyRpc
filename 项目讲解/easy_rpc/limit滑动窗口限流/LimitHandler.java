package com.rpc.easy_rpc_govern.limit;

import com.rpc.easy_rpc_govern.limit.entity.LimitingRule;

public interface LimitHandler {
    public boolean limitHandle(LimitingRule rule);
}
