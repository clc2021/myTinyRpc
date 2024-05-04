package com.rpc.easy_rpc_govern.limit.limitStrategy.slidingWindowStrategy;

import lombok.Data;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

@Data
public class SampleEntity {
//    键为limitKey
    private ConcurrentHashMap<Object,Integer> passMap;
//    键为limitKey
    private ConcurrentHashMap<Object,Integer> blockMap;
//    整个样本窗口内的API的通过数量
    private AtomicInteger pass;
//    整个样本窗口内的API的拒绝数量
    private AtomicInteger block;

    public SampleEntity() {
        passMap = new ConcurrentHashMap<>();
        blockMap = new ConcurrentHashMap<>();
        pass = new AtomicInteger(0);
        block = new AtomicInteger(0);
    }

    public void init(){
        passMap = new ConcurrentHashMap<>();
        blockMap = new ConcurrentHashMap<>();
        pass.set(0);
        block.set(0);
    }

    public void addPass(Object obj){
        passMap.put(obj, passMap.getOrDefault(obj,0)+1);
        addPass();
    }

    public void addBlock(Object obj){
        blockMap.put(obj, blockMap.getOrDefault(obj,0)+1);
        addBlock();
    }

    public void addPass(){
        pass.incrementAndGet();
    }

    public void addBlock(){
        block.incrementAndGet();
    }

    public int getPassCountByKey(Object obj){
        return passMap.getOrDefault(obj,0);
    }

    public int getBlockCountByKey(Object obj){
        return passMap.getOrDefault(obj,0);
    }
    
    public int getPassCount(){
        return pass.get();
    }
    
    public int getBlockCount(){
        return block.get();

    }




}
