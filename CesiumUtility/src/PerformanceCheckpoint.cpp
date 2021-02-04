#include "CesiumUtility/PerformanceCheckpoint.h"
#include <spdlog/spdlog.h>

using namespace CesiumUtility;

PerformanceCheckpoint::PerformanceCheckpoint(const std::string& measurementName) :
    _measurementName(measurementName),
    _max(0),
    _total(0),
    _count(0),
    _start()
{
}

PerformanceToken PerformanceCheckpoint::start() {
    return { std::chrono::high_resolution_clock::now() };
}

void PerformanceCheckpoint::stop(PerformanceToken token, const std::shared_ptr<spdlog::logger>& pLogger) {
    auto end = std::chrono::high_resolution_clock::now();
    int64_t t = (end - token.start).count();
    this->_total += t;
    ++this->_count;

    int64_t max = this->_max;
    while (t > max) {
        this->_max.compare_exchange_strong(max, t);
    }

    const double nanosecondsPerMilliseconds = 1000000.0;
    SPDLOG_LOGGER_WARN(pLogger, "{}: time {}ms max {}ms average {}ms", this->_measurementName, t / nanosecondsPerMilliseconds, max / nanosecondsPerMilliseconds, (this->_total / this->_count) / nanosecondsPerMilliseconds);
}
