package com.rpc.easy_rpc_govern.limit.limitStrategy.slidingWindowStrategy;

import com.rpc.easy_rpc_govern.limit.config.LimitConfig;
import com.rpc.easy_rpc_govern.limit.limitStrategy.LimitStrategy;
import lombok.extern.slf4j.Slf4j;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReferenceArray;
import java.util.concurrent.locks.ReentrantLock;

@Slf4j
public class SlidingWindow implements LimitStrategy {
//    滑动窗口时间 1000
    private final long windowIntervalInMs;
//    每个滑动窗口的样本窗口数量 8
    private final int sampleWindowAmount;
//    每个样本窗口的时间 125
    private final long sampleWindowIntervalInMs;
//    样本窗口数组
    private final AtomicReferenceArray<SampleWindow> sampleWindowAtomicReferenceArray;

    private final ReentrantLock updateLock = new ReentrantLock();

    public SlidingWindow(LimitConfig limitConfig){
        this.windowIntervalInMs = limitConfig.getWindowIntervalInMs();
        this.sampleWindowAmount = limitConfig.getSampleWindowAmount();
        this.sampleWindowIntervalInMs = windowIntervalInMs / sampleWindowAmount;
        this.sampleWindowAtomicReferenceArray = new AtomicReferenceArray<>(sampleWindowAmount);
    }

//  根据当前时间获取样本窗口地址
    private int getCurSampleWindowIndex(Long time){

        long temp = time / sampleWindowIntervalInMs;
        return (int)(temp % sampleWindowAmount);
    }

//    获取样本窗口的开始时间
    private Long getCurSampleWindowStartTime(Long time){ // 传入实时
        return time - (time % windowIntervalInMs);
    }

//    判断当前样本窗口是否还在滑动窗口时间内
    private boolean isSampleWindowDeprecated(long time,SampleWindow sampleWindow){
        if(sampleWindow == null){
            return true;
        }
        return ((time - sampleWindow.getStartTimeInMs()) > windowIntervalInMs);
    }
//  获取有效窗口
    private List<SampleWindow> getValidSampleWindow(long time){
        List<SampleWindow> res = new ArrayList<>();
        int length = sampleWindowAtomicReferenceArray.length();
        for (int i = 0; i < length; i++) {
            SampleWindow sampleWindow = sampleWindowAtomicReferenceArray.get(i);
            if(!isSampleWindowDeprecated(time,sampleWindow)){
                res.add(sampleWindow);
            }
        }
        return res;
    }

    /**
     * 获取当前的样本窗口
     * 1：计算当前系统时间对应的样本窗口的开始时间
     * 2：通过该系统时间找到样本窗口的下标
     * 3：先查看样本窗口如果是为null，则进行创建一个样本窗口，然后返窗口
     * 4：如果数组中的样本窗口有值，则查看该样本窗口的起始时间是否与计算的开始时间一致，如果一致则直接返回该样本窗口
     * 5：如果不一致，则该窗口以前创建过，是一个过去的样本窗口，则进行初始化为当前窗口
     * @return
     */
    private SampleWindow getCurSampleWindow(){
        Long curSystemTime = System.currentTimeMillis();
        int curSampleWindowIndex = getCurSampleWindowIndex(curSystemTime); // 固定时间
        long curSampleWindowStartTime = getCurSampleWindowStartTime(curSystemTime);

        while (true){
            // 若为null则初始化一个样本窗口
            if(sampleWindowAtomicReferenceArray.get(curSampleWindowIndex) == null){
                SampleWindow newSampleWindow = new SampleWindow(curSampleWindowStartTime,sampleWindowIntervalInMs,new SampleEntity());
                if (sampleWindowAtomicReferenceArray.compareAndSet(curSampleWindowIndex,null,newSampleWindow)) {
                    return newSampleWindow;
                }else {
                    // CAS如果失败则礼让线程，重试
                    Thread.yield();
                }
            }else {
                SampleWindow oldSampleWindow = sampleWindowAtomicReferenceArray.get(curSampleWindowIndex);
                // 当前为同一个样本窗，直接返回
                if(oldSampleWindow.getStartTimeInMs() == curSampleWindowStartTime){
                    return oldSampleWindow;
                }else if(oldSampleWindow.getStartTimeInMs() < curSampleWindowStartTime){
                     if(updateLock.tryLock()){
                         try {
                             return oldSampleWindow.reset(curSampleWindowStartTime);
                         }finally {
                             updateLock.unlock();
                         }
                     }else {
                         Thread.yield();
                     }
                }
//                一般不会发生以下情况（重置系统时间导致时间倒流）
                else if(oldSampleWindow.getStartTimeInMs() > curSampleWindowStartTime){
                    return new SampleWindow(curSampleWindowStartTime,sampleWindowIntervalInMs,new SampleEntity());
                }
            }
        }
    }

    @Override
    public int getQPS() {
        long curTime = System.currentTimeMillis();
        int passCount = getPassCount(curTime);
        return passCount;
    }

    @Override
    public int getQPS(Object obj) {
        long curTime = System.currentTimeMillis();
        int passCount = getPassCount(curTime,obj);
        return passCount;
    }

    @Override
    public int getPassCount(long time){
        getCurSampleWindow();
        int passAmount = 0;
        List<SampleWindow> validSampleWindows = getValidSampleWindow(time);
        for (SampleWindow validSampleWindow : validSampleWindows) {
            passAmount+=validSampleWindow.getSampleEntity().getPassCount();
        }
        return passAmount;
    }

    @Override
    public int getPassCount(long time, Object obj) {
        getCurSampleWindow();
        int passAmount = 0;
        List<SampleWindow> validSampleWindows = getValidSampleWindow(time);
        for (SampleWindow validSampleWindow : validSampleWindows) {
            passAmount+=validSampleWindow.getSampleEntity().getPassCountByKey(obj);
        }
        return passAmount;
    }

    @Override
    public void incrPassCount() {
        SampleWindow curSampleWindow = getCurSampleWindow();
        curSampleWindow.getSampleEntity().addPass();
    }

    @Override
    public void incrPassCount(Object obj) {
        SampleWindow curSampleWindow = getCurSampleWindow();
        curSampleWindow.getSampleEntity().addPass(obj);
    }

    @Override
    public void incrBlockCount() {
        SampleWindow curSampleWindow = getCurSampleWindow();
        curSampleWindow.getSampleEntity().addBlock();
    }


    @Override
    public void incrBlockCount(Object obj) {
        SampleWindow curSampleWindow = getCurSampleWindow();
        curSampleWindow.getSampleEntity().addBlock(obj);
    }
}
