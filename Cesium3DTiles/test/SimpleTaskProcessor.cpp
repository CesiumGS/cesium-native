
#include "SimpleTaskProcessor.h"

#include "Cesium3DTiles/spdlog-cesium.h"

#include "Cesium3DTilesTestUtils.h"

#include <chrono>
#include <thread>

namespace Cesium3DTilesTests
{
	SimpleTaskProcessor::SimpleTaskProcessor() : 
		_sleepDurationMs(50),
		_blocking(false) {
		// Nothing else to do here
	}

	SimpleTaskProcessor::SimpleTaskProcessor(int32_t sleepDurationMs, bool blocking) : 
		_sleepDurationMs(sleepDurationMs),
		_blocking(blocking) {
		// Nothing else to do here
	}

	void SimpleTaskProcessor::startTask(std::function<void()> f) {
		SPDLOG_TRACE("Called SimpleTaskProcessor::startTask");

		std::thread thread([this, f] {
			SPDLOG_TRACE("SimpleTaskProcessor thread running");
			try {
				sleepMsLogged(_sleepDurationMs);
				f();
			} catch (const std::exception& e) {
				SPDLOG_ERROR("SimpleTaskProcessor: Exception in background thread: {0}", e.what());
			}
			SPDLOG_TRACE("SimpleTaskProcessor thread running DONE");
		});
		thread.detach();
		if (_blocking) {
			thread.join();
		}
	}
}