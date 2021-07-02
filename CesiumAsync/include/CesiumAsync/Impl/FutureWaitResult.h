#pragma once

#include <exception>
#include <variant>

namespace CesiumAsync {
namespace Impl {

template <typename T> struct FutureWaitResult {
  typedef std::variant<T, std::exception> type;

  static std::variant<T, std::exception> getFromTask(async::task<T>& task) {
    return task.get();
  }
};

template <> struct FutureWaitResult<void> {
  typedef std::variant<std::monostate, std::exception> type;

  static std::variant<std::monostate, std::exception>
  getFromTask(async::task<void>& task) {
    task.get();
    return std::monostate();
  }
};

template <typename T>
using FutureWaitResult_t = typename FutureWaitResult<T>::type;

} // namespace Impl
} // namespace CesiumAsync