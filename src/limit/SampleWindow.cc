#include "../include/limit/SampleWindow.h"
#include <memory>
long SampleWindow::getStartTimeInMs() {
    return startTimeInMs;
}

SampleEntity SampleWindow::getSampleEntity() {
    return sampleEntity;
}

SampleWindow SampleWindow::reset(long startTime) {  
    startTimeInMs = startTime; // 重置
    sampleEntity.init();
    return *this;
}