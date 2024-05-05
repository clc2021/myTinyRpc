#include "../include/limit/SampleEntity.h"

void SampleEntity::init() {
    std::lock_guard<std::mutex> lock(mtx); // 上锁
    passMap.clear();
    blockMap.clear();
    pass.store(0);
    block.store(0);
}

void SampleEntity::addPass() {
    pass.fetch_add(1);
}

void SampleEntity::addPass(void* obj) {
    std::lock_guard<std::mutex> lock(mtx); // 上锁
    passMap[obj] = passMap[obj] + 1;
    addPass();
}

void SampleEntity::addBlock() { 
    block.fetch_add(1);
}

void SampleEntity::addBlock(void* obj) { 
    std::lock_guard<std::mutex> lock(mtx);
    blockMap[obj] = blockMap[obj] + 1;
    addBlock();
}

int SampleEntity::getPassCount() {
    return pass.load();
}

int SampleEntity::getBlockCount() {
    return block.load();
}

int SampleEntity::getPassCountByKey(void* obj) {
    std::lock_guard<std::mutex> lock(mtx); // 上锁
    auto it = passMap.find(obj);
    if (it != passMap.end())
        return it->second;
    else 
        return 0;
}

int SampleEntity::getBlockCountByKey(void* obj) {
    std::lock_guard<std::mutex> lock(mtx); // 上锁
    auto it = blockMap.find(obj);
    if (it != blockMap.end())
        return it->second;
    else
        return 0;
}