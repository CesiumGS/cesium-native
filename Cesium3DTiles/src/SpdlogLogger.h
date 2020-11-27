#pragma once

#include <memory>
#include <mutex>

#include "spdlog/spdlog.h"
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/null_mutex.h>

#include "Cesium3DTiles/ILogger.h"

namespace {

	/**
	 * @brief Translate the given spdlog level into an appropriate Cesium log level.
	 */
	Cesium3DTiles::ILogger::Level translate(spdlog::level::level_enum spdlogLevel) {
		switch (spdlogLevel) {
		case spdlog::level::level_enum::trace:
			return Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_TRACE;
		case spdlog::level::level_enum::debug:
			return Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_DEBUG;
		case spdlog::level::level_enum::info:
			return Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_INFO;
		case spdlog::level::level_enum::warn:
			return Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_WARN;
		case spdlog::level::level_enum::err:
			return Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_ERROR;
		case spdlog::level::level_enum::critical:
			return Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_CRITICAL;
		case spdlog::level::level_enum::off:
		case spdlog::level::level_enum::n_levels:
			break;
		}
		// Should never happen:
		return Cesium3DTiles::ILogger::Level::CESIUM_LOG_LEVEL_WARN;
	}
}

/**
 * @brief Internal implementation of a spdlog sink that forwards the messages
 * to an {@link ILogger}.
 */
template<typename Mutex>
class spdlog_logger_sink : public spdlog::sinks::base_sink<Mutex>
{
public:
	spdlog_logger_sink(std::shared_ptr<Cesium3DTiles::ILogger> logger)
		: _logger(logger) 
	{
	}

protected:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		spdlog::memory_buf_t formatted;
		spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
		std::string formattedString = fmt::to_string(formatted);

		Cesium3DTiles::ILogger::Level cesiumLevel = translate(msg.level);
		this->_logger->log(cesiumLevel, formattedString);
	}

	void flush_() override {
		// Nothing to do here
	}

private:
	std::shared_ptr<Cesium3DTiles::ILogger> _logger;
};

using spdlog_logger_sink_mt = spdlog_logger_sink<std::mutex>;
using spdlog_logger_sink_st = spdlog_logger_sink<spdlog::details::null_mutex>;
