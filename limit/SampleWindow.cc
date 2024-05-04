#include "SampleWindow.h"
#include <memory>
long SampleWindow::getStartTimeInMs() {
    return startTimeInMs;
}

SampleEntity SampleWindow::getSampleEntity() {
    return sampleEntity;
}

// std::shared_ptr<SampleWindow> reset(long startTime) {
SampleWindow SampleWindow::reset(long startTime) {
    startTimeInMs = startTime; // 重置
    sampleEntity.init();
    // return shared_from_this(); to_do要不要换成别的？
    return *this;
}