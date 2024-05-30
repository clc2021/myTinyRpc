#include "../include/limit/SlidingWindow.h"

// 根据当前时间获取样本窗口地址。传入实时。
int SlidingWindow::getCurSampleWindowIdx(long time) { 
    long temp = time / sampleWindowIntervalInMs; 
    return (int)(temp % sampleWindowAmount); 
}

// 获取样本窗口的开始时间。一定是滑动窗口时间的整数倍。
long SlidingWindow::getCurSampleWindowStartTime(long time) { 
    return time - (time % windowIntervalInMs); 
}

// 判断窗口是否弃用。传入实时。
bool SlidingWindow::isSampleWindowDeprecated(long time, SampleWindow* sampleWindow) { 
    if (sampleWindow == nullptr)
        return true;
    return ((time - sampleWindow->getStartTimeInMs()) > windowIntervalInMs); 
}

// 获取有效窗口
std::vector<SampleWindow*> SlidingWindow::getValidSampleWindow(long time) { 
    std::lock_guard<std::mutex> lock(updateMtx); // 上锁
    std::vector<SampleWindow*> res;
    int length = sampleWindowVector.size();
    for (int i = 0; i < length; i++) {
        SampleWindow* sampleWindow = sampleWindowVector[i];
        if (!isSampleWindowDeprecated(time, sampleWindow))
            res.emplace_back(new SampleWindow(*sampleWindow));
    }

    return res;   
}

// 获取当前窗口
SampleWindow* SlidingWindow::getCurSampleWindow() { 
    std::cout << "在最重要的getCurSampleWindow()中: " << std::endl;
    long curSystemTime = std::chrono::system_clock::now().time_since_epoch().count(); // 当前系统时间
    int curSampleWindowIdx = getCurSampleWindowIdx(curSystemTime); // 获取[]也就是idx
    long curSampleWindowStartTime = getCurSampleWindowStartTime(curSystemTime);
    std::cout << "当前系统时间: " << curSystemTime << "\t" << "index: " << curSampleWindowIdx << "\t"
    << "样本窗口开始时间: " << curSampleWindowStartTime << std::endl;

    while (true) {
        // 若为nullptr，则初始化一个窗口
        SampleWindow* currentSampleWindow = nullptr;
        {
            std::lock_guard<std::mutex> lock(updateMtx); // 上锁
            currentSampleWindow = sampleWindowVector[curSampleWindowIdx];
        }

        if (!currentSampleWindow) {                          // 样本窗口开始时间        // 样本窗口持续时间        // 空的实体
            SampleWindow* newSampleWindow = new SampleWindow(curSampleWindowStartTime, sampleWindowIntervalInMs, SampleEntity());
            {
                std::lock_guard<std::mutex> lock(updateMtx); // 上锁
                if (!sampleWindowVector[curSampleWindowIdx]) {
                    sampleWindowVector[curSampleWindowIdx] = newSampleWindow;
                } else {
                    delete newSampleWindow;
                    std::this_thread::yield();
                    continue;
                }
            }
            return newSampleWindow;
        } 

        else { // 不空检查
            if (currentSampleWindow->getStartTimeInMs() == curSampleWindowStartTime)
                return currentSampleWindow;
            else if (currentSampleWindow->getStartTimeInMs() < curSampleWindowStartTime) {
                std::lock_guard<std::mutex> lock(updateMtx); // 上锁：更新锁
                return currentSampleWindow->reset(curSampleWindowStartTime); // 更新窗口
            } else {
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
    SampleWindow* sw = getCurSampleWindow();
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
