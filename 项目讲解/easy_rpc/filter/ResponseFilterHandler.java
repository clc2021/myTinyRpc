package com.rpc.easy_rpc_govern.filter;

import com.rpc.domain.pojo.ProviderResponse;

public interface ResponseFilterHandler {
    public void filterHandler(ProviderResponse result);

}
