#pragma once

#include "Impl/AsyncSystemSchedulers.h"
#include "Impl/cesium-async++.h"

#include <exception>
#include <memory>
#include <utility>

namespace CesiumAsync {

template <typename T> class Future;

/**
 * @brief A promise that can be resolved or rejected by an asynchronous task.
 *
 * @tparam T The type of the object that the promise will be resolved with.
 */
template <typename T> class Promise {
public:
  /**
   * @brief Will be called when the task completed successfully.
   *
   * @param value The value that was computed by the asynchronous task.
   */
  void resolve(T&& value) const { this->_pEvent->set(std::move(value)); }

  /**
   * @brief Will be called when the task completed successfully.
   *
   * @param value The value that was computed by the asynchronous task.
   */
  void resolve(const T& value) const { this->_pEvent->set(value); }

  /**
   * @brief Will be called when the task failed.
   *
   * @param error The error that caused the task to fail.
   */
  template <typename TException> void reject(TException error) const {
    this->_pEvent->set_exception(std::make_exception_ptr(std::move(error)));
  }

  /**
   * @brief Will be called when the task failed.
   *
   * @param error The error, captured with `std::current_exception`, that
   * caused the task to fail.
   */
  void reject(const std::exception_ptr& error) const {
    this->_pEvent->set_exception(error);
  }

  /**
   * @brief Gets the Future that resolves or rejects when this Promise is
   * resolved or rejected.
   *
   * This method may only be called once.
   *
   * @return The future.
   */
  Future<T> getFuture() const {
    return Future<T>(this->_pSchedulers, this->_pEvent->get_task());
  }

private:
  Promise(
      const std::shared_ptr<CesiumImpl::AsyncSystemSchedulers>& pSchedulers,
      const std::shared_ptr<async::event_task<T>>& pEvent) noexcept
      : _pSchedulers(pSchedulers), _pEvent(pEvent) {}

  std::shared_ptr<CesiumImpl::AsyncSystemSchedulers> _pSchedulers;
  std::shared_ptr<async::event_task<T>> _pEvent;

  friend class AsyncSystem;
};

/**
 * @brief Specialization for promises that resolve to no value.
 */
template <> class Promise<void> {
public:
  /**
   * @brief Will be called when the task completed successfully.
   */
  void resolve() const { this->_pEvent->set(); }

  /**
   * @brief Will be called when the task failed.
   *
   * @param error The error that caused the task to fail.
   */
  template <typename TException> void reject(TException error) const {
    this->_pEvent->set_exception(std::make_exception_ptr(std::move(error)));
  }

  /**
   * @brief Will be called when the task failed.
   *
   * @param error The error, captured with `std::current_exception`, that
   * caused the task to fail.
   */
  void reject(const std::exception_ptr& error) const {
    this->_pEvent->set_exception(error);
  }
  /**
   * @copydoc Promise::getFuture
   */
  Future<void> getFuture() const {
    return Future<void>(this->_pSchedulers, this->_pEvent->get_task());
  }

private:
  Promise(
      const std::shared_ptr<CesiumImpl::AsyncSystemSchedulers>& pSchedulers,
      const std::shared_ptr<async::event_task<void>>& pEvent) noexcept
      : _pSchedulers(pSchedulers), _pEvent(pEvent) {}

  std::shared_ptr<CesiumImpl::AsyncSystemSchedulers> _pSchedulers;
  std::shared_ptr<async::event_task<void>> _pEvent;

  friend class AsyncSystem;
};

} // namespace CesiumAsync
