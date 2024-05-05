#ifndef SAMPLE_ENTITY_H
#define SAMPLE_ENTITY_H
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <utility>
class SampleEntity { // 样本窗口载体类
// 主要记录的是pass通过的和block拦截的
private:
    std::unordered_map<void*, int> passMap;
    std::unordered_map<void*, int> blockMap;
    std::atomic<int> pass; // 整个样本窗口内的API的通过数量
    std::atomic<int> block; // 整个样本窗口内的API的拒绝数量
    std::mutex mtx; // 保护

public:
    SampleEntity() {
        pass.store(0);
        block.store(0);
    }

    SampleEntity(const SampleEntity& other) { // 拷贝构造
        passMap = other.passMap;
        blockMap = other.blockMap;
        pass.store(other.pass.load());
        block.store(other.block.load());
    }

    SampleEntity& operator=(const SampleEntity& other) { // 拷贝赋值
        if (this != &other) {
            passMap = other.passMap;
            blockMap = other.blockMap;
            pass.store(other.pass.load());
            block.store(other.block.load());
        }
        return *this;
    }

    void init();
    void addPass();
    void addPass(void* obj);
    void addBlock();
    void addBlock(void* obj);
    int getPassCount(); 
    int getBlockCount();
    int getPassCountByKey(void* obj);
    int getBlockCountByKey(void* obj);
}; 

#endif