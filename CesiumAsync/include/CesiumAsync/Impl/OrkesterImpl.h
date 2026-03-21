#pragma once

/// @file Impl/OrkesterImpl.h
/// Shared implementation types for the orkester-backed
/// Future/SharedFuture/Promise. This header is included by both Future.h and
/// SharedFuture.h.

#include <CesiumAsync/Library.h>

#include <orkester.h>

// The original CesiumAsync Future.h transitively included spdlog via
// ThreadPool.h -> Impl/ImmediateScheduler.h. Downstream headers
// (SubtreeAvailability.h etc.) depend on this transitive include.
#include <spdlog/spdlog.h>

#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <variant>

namespace CesiumAsync {

template <typename T> class Future;
template <typename T> class SharedFuture;
template <typename T> class Promise;
class AsyncSystem;
class ThreadPool;

namespace CesiumImpl {

template <typename T> struct ValueSlot {
  std::variant<std::monostate, T, std::exception_ptr> state;

  void store(T&& value) { state.template emplace<T>(std::move(value)); }
  template <
      typename U = T,
      std::enable_if_t<std::is_copy_constructible_v<U>, int> = 0>
  void store(const T& value) {
    state.template emplace<T>(value);
  }
  void storeError(std::exception_ptr e) { state = e; }

  bool hasValue() const { return std::holds_alternative<T>(state); }
  bool hasError() const {
    return std::holds_alternative<std::exception_ptr>(state);
  }

  T take() {
    if (auto* e = std::get_if<std::exception_ptr>(&state)) {
      std::rethrow_exception(*e);
    }
    if (auto* v = std::get_if<T>(&state)) {
      T result = std::move(*v);
      state = std::monostate{};
      return result;
    }
    throw std::runtime_error("ValueSlot: no value");
  }

  T peek() const {
    if (auto* e = std::get_if<std::exception_ptr>(&state)) {
      std::rethrow_exception(*e);
    }
    if (auto* v = std::get_if<T>(&state)) {
      return *v;
    }
    throw std::runtime_error("ValueSlot: no value");
  }
};

template <> struct ValueSlot<void> {
  std::exception_ptr error;

  void storeError(std::exception_ptr e) { error = e; }
  bool hasError() const { return error != nullptr; }

  void take() {
    if (error)
      std::rethrow_exception(error);
  }

  void peek() const {
    if (error)
      std::rethrow_exception(error);
  }
};

template <typename T>
std::exception_ptr extractError(const ValueSlot<T>& slot) {
  if (auto* e = std::get_if<std::exception_ptr>(&slot.state))
    return *e;
  return {};
}
inline std::exception_ptr extractError(const ValueSlot<void>& slot) {
  return slot.error;
}

// ─── Continuation type computation ──────────────────────────────────────────

template <typename Func, typename T> struct ContinuationReturnType {
  using type = std::invoke_result_t<Func, T>;
};

template <typename Func> struct ContinuationReturnType<Func, void> {
  using type = std::invoke_result_t<Func>;
};

template <typename T> struct RemoveFuture {
  using type = T;
};
template <typename T> struct RemoveFuture<Future<T>> {
  using type = T;
};
template <typename T> struct RemoveFuture<SharedFuture<T>> {
  using type = T;
};

template <typename T> struct IsFuture : std::false_type {};
template <typename T> struct IsFuture<Future<T>> : std::true_type {};
template <typename T> struct IsFuture<SharedFuture<T>> : std::true_type {};

template <typename Func, typename T>
using ContinuationFutureType_t = Future<typename RemoveFuture<
    typename ContinuationReturnType<Func, T>::type>::type>;

// ─── Helper function declarations ───────────────────────────────────────────
// stealSignal / getSlot are defined in Future.h / SharedFuture.h after
// the respective class definitions.
template <typename V> orkester_future_t stealSignal(Future<V>& f);
template <typename V> std::shared_ptr<ValueSlot<V>> getSlot(Future<V>& f);
template <typename V> orkester_future_t stealSignal(SharedFuture<V>& f);
template <typename V> std::shared_ptr<ValueSlot<V>> getSlot(SharedFuture<V>& f);

} // namespace CesiumImpl
} // namespace CesiumAsync
