#ifndef SLIDING_WINDOW_H
#define SLIDING_WINDOW_H
#include <vector>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include "SampleWindow.h"
#include <utility>
#include <iostream>

class SlidingWindow {
private:
    const long windowIntervalInMs; // 1000
    const int sampleWindowAmount; // 8
    const long sampleWindowIntervalInMs; // 125
    std::vector<SampleWindow*> sampleWindowVector;
    std::mutex updateMtx; // 更新锁
    // std::mutex vecMtx; // 样本数组锁 to_do 不确定要不要用

    int getCurSampleWindowIdx(long time);
    long getCurSampleWindowStartTime(long time);
    bool isSampleWindowDeprecated(long time, SampleWindow* sampleWindow);
    std::vector<SampleWindow*> getValidSampleWindow(long time); 
    SampleWindow* getCurSampleWindow();

public:
    SlidingWindow(): windowIntervalInMs(1000), sampleWindowAmount(8), sampleWindowIntervalInMs(125) {
        std::cout << "调用SlidingWindow() 开头" << std::endl;
        for (int i = 0; i < sampleWindowAmount; i++) {
            sampleWindowVector.emplace_back(nullptr);
        }
        std::cout << "调用SlidingWindow() 结尾" << std::endl; 
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