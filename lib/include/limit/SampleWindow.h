#ifndef SAMPLE_WINDOW_H
#define SAMPLE_WINDOW_H
#include "SampleEntity.h"
#include <memory>
#include <utility> // std::move
#include <mutex>
//class SampleWindow: public std::enable_shared_from_this<SampleWindow> { // 样本窗口类
class SampleWindow {
private:
    long startTimeInMs; // 窗口开始时间 
    const long intervalTime; // 窗口时间长度
    SampleEntity sampleEntity; // 窗口数据

public:
    SampleWindow(long st, long ite, SampleEntity sam): startTimeInMs(st), intervalTime(ite), sampleEntity(sam) {}
    SampleWindow(const SampleWindow& sw):// 常成员变量只能通过构造函数列表初始化一次
        startTimeInMs(sw.startTimeInMs),
        intervalTime(sw.intervalTime),
        sampleEntity(sw.sampleEntity) {}
    
    SampleWindow& operator=(const SampleWindow& other) {
        if (this != &other) {
            startTimeInMs = other.startTimeInMs;
            sampleEntity = other.sampleEntity;
        }
        return *this;
    }
    
    long getStartTimeInMs();
    SampleEntity getSampleEntity();
    SampleWindow* reset(long startTime);
};
#endif