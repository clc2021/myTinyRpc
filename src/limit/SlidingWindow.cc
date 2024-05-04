#include "../include/limit/SlidingWindow.h"

int SlidingWindow::getCurSampleWindowIdx(long time) { // 根据当前时间获取样本窗口地址。传入实时。
    long temp = time / sampleWindowIntervalInMs; // 1300 / 125 = 9
    return (int)(temp % sampleWindowAmount); // 9 % 8 = 1
}

long SlidingWindow::getCurSampleWindowStartTime(long time) { // 获取样本开始时间。传入实时. to_do ???
    return time - (time % windowIntervalInMs); // 1300 - (1300 % 1000) = 1000
}

// 判断窗口是否弃用。传入实时。
bool SlidingWindow::isSampleWindowDeprecated(long time, SampleWindow* sampleWindow) { // 判断当前样本窗口是否还在滑动窗口时间内。实际上是看窗口是否弃用
    if (sampleWindow == nullptr)
        return true;
    //to_do这里好像就是窗口的开始时间而已。???
    return ((time - sampleWindow->getStartTimeInMs()) > windowIntervalInMs); 
}

std::vector<SampleWindow*> SlidingWindow::getValidSampleWindow(long time) { // 获取有效窗口
    std::lock_guard<std::mutex> lock(updateMtx); // 上锁
    std::vector<SampleWindow*> res;
    int length = sampleWindowVector.size();
    for (int i = 0; i < length; i++) {
        SampleWindow* sampleWindow = sampleWindowVector[i].load();
        if (!isSampleWindowDeprecated(time, sampleWindow))
            res.emplace_back(new SampleWindow(*sampleWindow));
    }

    return res;   
}

/*
获取当前的样本窗口 (核心)
1：获取系统时间
2:通过该系统时间找到样本窗口的下标
3：计算当前系统时间对应的样本窗口的开始时间
4：先查看样本窗口如果是为nullptr，则进行创建一个样本窗口，然后返窗口
5：如果数组中的样本窗口有值，则查看该样本窗口的起始时间是否与计算的开始时间一致，如果一致则直接返回该样本窗口
6：如果不一致，则该窗口以前创建过，是一个过去的样本窗口，则进行初始化为当前窗口
*/
SampleWindow* SlidingWindow::getCurSampleWindow() { // 获取当前窗口
    // 获取当前的系统时间
    long curSystemTime = std::chrono::system_clock::now().time_since_epoch().count();
    int curSampleWindowIdx = getCurSampleWindowIdx(curSystemTime);
    long curSampleWindowStartTime = getCurSampleWindowStartTime(curSystemTime);
    while (true) {
        // 若为nullptr，则初始化一个窗口
        if (!sampleWindowVector[curSampleWindowIdx].load()) {
            SampleWindow* newSampleWindow = new 
                SampleWindow(curSampleWindowStartTime, sampleWindowIntervalInMs, SampleEntity());
            if (std::atomic_exchange(&sampleWindowVector[curSampleWindowIdx], newSampleWindow) == nullptr)
                return newSampleWindow;
            else {
                delete newSampleWindow;
                std::this_thread::yield();
            }
        } else { // 不空检查
            auto oldSampleWindow = sampleWindowVector[curSampleWindowIdx].load(); // 旧的窗口
            if (oldSampleWindow->getStartTimeInMs() == curSampleWindowStartTime)
                return oldSampleWindow;
            else if (oldSampleWindow->getStartTimeInMs() < curSampleWindowStartTime) {
                std::lock_guard<std::mutex> lock(updateMtx); // 上锁：更新锁
                oldSampleWindow->reset(curSampleWindowStartTime); // 更新窗口
            } else if (oldSampleWindow->getStartTimeInMs() > curSampleWindowStartTime) {
                return new SampleWindow(curSampleWindowStartTime, sampleWindowIntervalInMs, SampleEntity());
            }
        } 
    }
}   

int SlidingWindow::getPassCount(long time) {
    SampleWindow* sw = getCurSampleWindow(); // 得到当前样本窗口
    int passAmount = 0;
    auto validSampleWindows = getValidSampleWindow(time);
    for (auto validSampleWindow : validSampleWindows)
        passAmount += validSampleWindow->getSampleEntity().getPassCount();

    return passAmount;
}

int SlidingWindow::getPassCount(long time, void* obj) {
    SampleWindow* sw = getCurSampleWindow(); // 得到当前样本窗口
    int passAmount = 0;
    auto validSampleWindows = getValidSampleWindow(time);
    for (auto validSampleWindow : validSampleWindows)
        passAmount += validSampleWindow->getSampleEntity().getPassCountByKey(obj);

    return passAmount;
}

void SlidingWindow::incrPassCount() {
    SamlpleWindow* sw = getCurSampleWindow();
    sw->getSampleEntity().addPass();
}

void SlidingWindow::incrPassCount(void* obj) {
    SampleWindow* sw = getCurSampleWindow();
    sw->getSampleEntity().addPass(obj);
}

void SlidingWindow::incrBlockCount() {
    SampleWindow* sw = getCurSampleWindow();
    sw->getSampleEntity().addBlock();
}

void SlidingWindow::incrBlockCount(void* obj) {
    SampleWindow* sw = getCurSampleWindow();
    sw->getSampleEntity().addBlock(obj);
}

int SlidingWindow::getQPS() {
    long curTime = std::chrono::system_clock::now().time_since_epoch().count();
    int passCount = getPassCount(curTime);
    return passCount;
}

int SlidingWindow::getQPS(void* obj) {
    long curTime = std::chrono::system_clock::now().time_since_epoch().count();
    int passCount = getPassCount(curTime, obj);
    return passCount;
}
