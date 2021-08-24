#pragma once

namespace CesiumAsync {

namespace Impl {
template <typename Func, typename T>
auto futureFunctionToTaskFunction(Func&& f);
}

/**
 * @brief A reference to the result produced by a {@link SharedFuture<T>}.
 *
 * A {@link SharedFutureResult<T>} only exists for a {@link SharedFuture<T>}
 * that is already resolved or rejected, and provides a means to access its
 * value without the full {@link SharedFuture<T>} machinery.
 *
 * Shared future results are reference counted. They remain accessible as long
 * as a {@link SharedFuture<T>} or {@link SharedFutureResult<T>} exists
 * referencing them.
 *
 * {@link SharedFuture<T>::wait} and {@link SharedFutureResult<T>::get} are
 * equivalent.
 *
 * @tparam T The type of value.
 */
template <typename T> class SharedFutureResult final {
public:
  /**
   * @brief Gets the shared future's result value if the future resolved, or
   * throws an exception if it rejected.
   *
   * This method is equivalent to {@link SharedFuture<T>::wait}, but it is
   * guaranteed not to block because the Future is guaranteed to already be
   * resolved or rejected.
   *
   * @return The resolved value.
   */
  const T& get() const { return this->_task.get(); }

private:
  SharedFutureResult(const async::shared_task<T>& task) : _task(task) {
    assert(task.ready());
  }

  async::shared_task<T> _task;

  template <typename Func, typename T2>
  friend auto Impl::futureFunctionToTaskFunction(Func&& f);
};

} // namespace CesiumAsync
