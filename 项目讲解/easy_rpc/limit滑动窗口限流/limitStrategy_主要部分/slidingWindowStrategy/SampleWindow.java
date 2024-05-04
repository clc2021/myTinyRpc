package com.rpc.easy_rpc_govern.limit.limitStrategy.slidingWindowStrategy;

import lombok.AllArgsConstructor;
import lombok.Data;

@Data
@AllArgsConstructor
public class SampleWindow {

    private long startTimeInMs;

    private final long intervalTime;

//    实际保持当前窗口的数据
    private SampleEntity sampleEntity;

    public SampleWindow reset(long startTime){
        startTimeInMs = startTime;
        sampleEntity.init();
        return this;
    }

}
