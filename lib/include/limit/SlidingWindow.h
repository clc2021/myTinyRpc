#ifndef SLIDING_WINDOW_H
#define SLIDING_WINDOW_H
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include "SampleWindow.h"

class SlidingWindow {
private:
    const long windowIntervalInMs; // 1000 滑动窗口时间 to_do：为什么是const？这里到底哪些涉及const？ 
    // 样本窗口是有：开始时间、时间长度和样本数据
    const int sampleWindowAmount; // 8 每个滑动窗口的样本窗口数量
    const long sampleWindowIntervalInMs; // 125 每个样本窗口时间
    // 声明为原子类型进行无锁编程
    std::vector<std::atomic<SampleWindow*>> sampleWindowVector; // 8个SampleWindow()　样本窗口数组
    std::mutex updateMtx;

    int getCurSampleWindowIdx(long time);
    long getCurSampleWindowStartTime(long time);
    bool isSampleWindowDeprecated(long time, SampleWindow* sampleWindow);
    std::vector<SampleWindow*> getValidSampleWindow(long time); 
    SampleWindow* getCurSampleWindow();

public:
    SlidingWindow(): windowIntervalInMs(1000), sampleWindowAmount(8), sampleWindowIntervalInMs(125) {
        sampleWindowVector.resize(sampleWindowAmount, nullptr);
    }

    int getPassCount(long time);
    int getPassCount(long time, void* obj);
    void incrPassCount();
    void incrPassCount(void* obj);
    void incrBlockCount();
    void incrBlockCount(void* obj);
    int getQPS();
    int getQPS(void* obj);
};

#endif