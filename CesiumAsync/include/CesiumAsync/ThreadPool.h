#pragma once

#include <CesiumAsync/Library.h>

#include <orkester.h>

#include <cstdint>

namespace CesiumAsync {

/// Thread pool backed by orkester. Wraps an opaque `orkester_thread_pool_t`.
class CESIUMASYNC_API ThreadPool final {
public:
  ThreadPool(const ThreadPool& rhs)
      : _pool(rhs._pool ? orkester_thread_pool_clone(rhs._pool) : nullptr) {}

  explicit ThreadPool(int32_t numberOfThreads)
      : _pool(
            orkester_thread_pool_create(static_cast<size_t>(numberOfThreads))) {
  }

  ThreadPool& operator=(const ThreadPool& rhs) {
    if (this != &rhs) {
      if (_pool)
        orkester_thread_pool_drop(_pool);
      _pool = rhs._pool ? orkester_thread_pool_clone(rhs._pool) : nullptr;
    }
    return *this;
  }

  ThreadPool(ThreadPool&& rhs) noexcept : _pool(rhs._pool) {
    rhs._pool = nullptr;
  }

  ThreadPool& operator=(ThreadPool&& rhs) noexcept {
    if (this != &rhs) {
      if (_pool)
        orkester_thread_pool_drop(_pool);
      _pool = rhs._pool;
      rhs._pool = nullptr;
    }
    return *this;
  }

  ~ThreadPool() noexcept {
    if (_pool)
      orkester_thread_pool_drop(_pool);
  }

private:
  friend class AsyncSystem;
  template <typename T> friend class Future;
  template <typename T> friend class SharedFuture;

  orkester_thread_pool_t _pool = nullptr;
};

} // namespace CesiumAsync
