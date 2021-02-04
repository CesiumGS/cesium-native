#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <spdlog/fwd.h>
#include <string>

namespace CesiumUtility {

    struct PerformanceToken {
        std::chrono::steady_clock::time_point start;
    };

    class PerformanceCheckpoint {
    public:
        PerformanceCheckpoint(const std::string& measurementName);

        PerformanceToken start();
        void stop(PerformanceToken token, const std::shared_ptr<spdlog::logger>& pLogger);

    private:
        std::string _measurementName;
        std::atomic<int64_t> _max;
        std::atomic<int64_t> _total;
        std::atomic<int64_t> _count;

        std::chrono::steady_clock::time_point _start;
    };

}
