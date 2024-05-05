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

// to_do 这段逻辑存疑？
std::vector<SampleWindow*> SlidingWindow::getValidSampleWindow(long time) { // 获取有效窗口
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

SampleWindow* SlidingWindow::getCurSampleWindow() { // 获取当前窗口
    // 获取当前的系统时间
    long curSystemTime = std::chrono::system_clock::now().time_since_epoch().count();
    int curSampleWindowIdx = getCurSampleWindowIdx(curSystemTime);
    long curSampleWindowStartTime = getCurSampleWindowStartTime(curSystemTime);
    while (true) {
        // 若为nullptr，则初始化一个窗口
        SampleWindow* currentSampleWindow = nullptr;
        {
            std::lock_guard<std::mutex> lock(updateMtx); // 上锁
            currentSampleWindow = sampleWindowVector[curSampleWindowIdx];
        }
        if (!currentSampleWindow) {
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
        } else { // 不空检查
            if (currentSampleWindow->getStartTimeInMs() == curSampleWindowStartTime)
                return currentSampleWindow;
            else if (currentSampleWindow->getStartTimeInMs() < curSampleWindowStartTime) {
                std::lock_guard<std::mutex> lock(updateMtx); // 上锁：更新锁
                currentSampleWindow->reset(curSampleWindowStartTime); // 更新窗口
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
