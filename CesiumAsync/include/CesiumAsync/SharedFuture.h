#pragma once

/// @file SharedFuture.h
/// Drop-in replacement for CesiumAsync::SharedFuture<T> backed by orkester.
///
/// Cloneable. Multiple continuations. Values stay in C++ via shared_ptr.

#include <CesiumAsync/Impl/OrkesterImpl.h>
#include <CesiumAsync/ThreadPool.h>

#include <tuple>

namespace CesiumAsync {

// ─── SharedFuture<T> ────────────────────────────────────────────────────────

/// Drop-in replacement for CesiumAsync::SharedFuture<T>, backed by orkester.
/// Cloneable. Values stay in C++ via shared_ptr<ValueSlot<T>>.
template <typename T>
class CESIUMASYNC_API SharedFuture final {
public:
    SharedFuture(const SharedFuture& rhs)
        : _system(rhs._system), _slot(rhs._slot),
          _signal(rhs._signal ? orkester_shared_future_clone(rhs._signal) : nullptr) {}

    SharedFuture& operator=(const SharedFuture& rhs) {
        if (this != &rhs) {
            if (_signal) orkester_shared_future_drop(_signal);
            _system = rhs._system;
            _slot = rhs._slot;
            _signal = rhs._signal ? orkester_shared_future_clone(rhs._signal) : nullptr;
        }
        return *this;
    }

    SharedFuture(SharedFuture&& rhs) noexcept
        : _system(rhs._system), _slot(std::move(rhs._slot)), _signal(rhs._signal) {
        rhs._signal = nullptr;
    }

    SharedFuture& operator=(SharedFuture&& rhs) noexcept {
        if (this != &rhs) {
            if (_signal) orkester_shared_future_drop(_signal);
            _system = rhs._system;
            _slot = std::move(rhs._slot);
            _signal = rhs._signal;
            rhs._signal = nullptr;
        }
        return *this;
    }

    ~SharedFuture() noexcept {
        if (_signal) orkester_shared_future_drop(_signal);
    }

    bool isReady() const {
        return _signal && orkester_shared_future_is_ready(_signal);
    }

    /// Block until resolved. Can be called multiple times (copies value).
    T wait() const {
        const char* errPtr = nullptr;
        size_t errLen = 0;
        bool ok = orkester_shared_future_wait(_signal, &errPtr, &errLen);
        if (!ok && errPtr) {
            std::string msg(errPtr, errLen);
            orkester_string_drop(errPtr, errLen);
            throw std::runtime_error(msg);
        }
        return _slot->peek(); // copy, don't consume
    }

    T waitInMainThread() {
        return wait(); // TODO: proper main-thread dispatch variant
    }

    /// Chain: worker thread.
    template <typename Func>
    CesiumImpl::ContinuationFutureType_t<Func, T>
    thenInWorkerThread(Func&& f) {
        return chain<Func>(orkester_context_t_ORKESTER_WORKER, std::forward<Func>(f));
    }

    /// Chain: main thread.
    template <typename Func>
    CesiumImpl::ContinuationFutureType_t<Func, T>
    thenInMainThread(Func&& f) {
        return chain<Func>(orkester_context_t_ORKESTER_MAIN, std::forward<Func>(f));
    }

    /// Chain: immediately.
    template <typename Func>
    CesiumImpl::ContinuationFutureType_t<Func, T>
    thenImmediately(Func&& f) {
        return chain<Func>(orkester_context_t_ORKESTER_IMMEDIATE, std::forward<Func>(f));
    }

    /// Chain in thread pool.
    template <typename Func>
    CesiumImpl::ContinuationFutureType_t<Func, T>
    thenInThreadPool(const ThreadPool& pool, Func&& f) {
        return chainInPool<Func>(pool, std::forward<Func>(f));
    }

    template <typename Func>
    Future<T> catchInMainThread(Func&& f) {
        return doCatch(orkester_context_t_ORKESTER_MAIN, std::forward<Func>(f));
    }

    template <typename Func>
    Future<T> catchImmediately(Func&& f) {
        return doCatch(orkester_context_t_ORKESTER_IMMEDIATE, std::forward<Func>(f));
    }

    template <typename... TPassThrough>
    Future<std::tuple<std::remove_cvref_t<TPassThrough>..., T>>
    thenPassThrough(TPassThrough&&... values) {
        return thenImmediately(
            [... captured = std::forward<TPassThrough>(values)](T result) mutable {
                return std::make_tuple(std::move(captured)..., std::move(result));
            });
    }

private:
    friend class AsyncSystem;
    template <typename R> friend class Future;
    template <typename R> friend class SharedFuture;
    template <typename R> friend class Promise;
    template <typename V> friend orkester_future_t CesiumImpl::stealSignal(SharedFuture<V>&);
    template <typename V> friend std::shared_ptr<CesiumImpl::ValueSlot<V>> CesiumImpl::getSlot(SharedFuture<V>&);

    SharedFuture(const orkester_async_t* system,
                 std::shared_ptr<CesiumImpl::ValueSlot<T>> slot,
                 orkester_shared_future_t signal)
        : _system(system), _slot(std::move(slot)), _signal(signal) {}

    /// Generic continuation chaining using context enum + simple callbacks.
    template <typename Func>
    auto chain(
        orkester_context_t context,
        Func&& f)
    {
        using RawResult = typename CesiumImpl::ContinuationReturnType<Func, T>::type;
        using U = typename CesiumImpl::RemoveFuture<RawResult>::type;
        constexpr bool needsUnwrap = CesiumImpl::IsFuture<RawResult>::value;

        auto nextSlot = std::make_shared<CesiumImpl::ValueSlot<U>>();

        orkester_promise_t outPromise = nullptr;
        orkester_future_t outFuture = nullptr;
        orkester_promise_create(_system, &outPromise, &outFuture);

        struct Adapter {
            std::shared_ptr<CesiumImpl::ValueSlot<T>> inputSlot;
            std::shared_ptr<CesiumImpl::ValueSlot<U>> outputSlot;
            std::decay_t<Func> func;
            orkester_promise_t promise;
            const orkester_async_t* system;

            static void invoke(void* ctx) {
                auto* self = static_cast<Adapter*>(ctx);
                self->run();
            }

            void run() {
                try {
                    if (inputSlot->hasError()) {
                        outputSlot->storeError(CesiumImpl::extractError(*inputSlot));
                        orkester_promise_resolve(promise);
                        delete this;
                        return;
                    }

                    if constexpr (std::is_void_v<T>) {
                        if constexpr (needsUnwrap) {
                            auto innerFuture = func();
                            unwrap(std::move(innerFuture));
                            return;
                        } else if constexpr (std::is_void_v<U>) {
                            func();
                        } else {
                            outputSlot->store(func());
                        }
                    } else {
                        if constexpr (needsUnwrap) {
                            auto innerFuture = func(inputSlot->peek());
                            unwrap(std::move(innerFuture));
                            return;
                        } else if constexpr (std::is_void_v<U>) {
                            func(inputSlot->peek());
                        } else {
                            outputSlot->store(func(inputSlot->peek()));
                        }
                    }
                } catch (...) {
                    outputSlot->storeError(std::current_exception());
                }
                orkester_promise_resolve(promise);
                delete this;
            }

            void unwrap(Future<U>&& innerFuture) {
                auto innerSlot = CesiumImpl::getSlot(innerFuture);
                auto innerSignal = CesiumImpl::stealSignal(innerFuture);

                struct Forwarder {
                    std::shared_ptr<CesiumImpl::ValueSlot<U>> innerSlot;
                    std::shared_ptr<CesiumImpl::ValueSlot<U>> outputSlot;
                    orkester_promise_t promise;

                    static void invoke(void* ctx) {
                        auto* fwd = static_cast<Forwarder*>(ctx);
                        try {
                            if (fwd->innerSlot->hasError()) {
                                fwd->outputSlot->storeError(CesiumImpl::extractError(*fwd->innerSlot));
                            } else if constexpr (!std::is_void_v<U>) {
                                fwd->outputSlot->store(fwd->innerSlot->take());
                            }
                        } catch (...) {
                            fwd->outputSlot->storeError(std::current_exception());
                        }
                        orkester_promise_resolve(fwd->promise);
                        delete fwd;
                    }
                };

                auto* fwd = new Forwarder{innerSlot, outputSlot, promise};
                orkester_future_then(innerSignal, orkester_context_t_ORKESTER_IMMEDIATE, &Forwarder::invoke, fwd);
                delete this;
            }

            void unwrap(SharedFuture<U>&& innerShared) {
                auto slot = CesiumImpl::getSlot(innerShared);
                orkester_future_t sig = CesiumImpl::stealSignal(innerShared);
                Future<U> unique(system, std::move(slot), sig);
                unwrap(std::move(unique));
            }
        };

        auto* adapter = new Adapter{_slot, nextSlot, std::forward<Func>(f), outPromise, _system};
        orkester_shared_future_then(_signal, context, &Adapter::invoke, adapter);
        // SharedFuture does NOT null _signal (it can be reused)
        return Future<U>(_system, nextSlot, outFuture);
    }

    /// Continuation chaining via thread pool.
    template <typename Func>
    auto chainInPool(const ThreadPool& pool, Func&& f)
    {
        using RawResult = typename CesiumImpl::ContinuationReturnType<Func, T>::type;
        using U = typename CesiumImpl::RemoveFuture<RawResult>::type;
        constexpr bool needsUnwrap = CesiumImpl::IsFuture<RawResult>::value;

        auto nextSlot = std::make_shared<CesiumImpl::ValueSlot<U>>();

        orkester_promise_t outPromise = nullptr;
        orkester_future_t outFuture = nullptr;
        orkester_promise_create(_system, &outPromise, &outFuture);

        struct Adapter {
            std::shared_ptr<CesiumImpl::ValueSlot<T>> inputSlot;
            std::shared_ptr<CesiumImpl::ValueSlot<U>> outputSlot;
            std::decay_t<Func> func;
            orkester_promise_t promise;
            const orkester_async_t* system;

            static void invoke(void* ctx) {
                auto* self = static_cast<Adapter*>(ctx);
                self->run();
            }

            void run() {
                try {
                    if (inputSlot->hasError()) {
                        outputSlot->storeError(CesiumImpl::extractError(*inputSlot));
                        orkester_promise_resolve(promise);
                        delete this;
                        return;
                    }

                    if constexpr (std::is_void_v<T>) {
                        if constexpr (needsUnwrap) {
                            auto innerFuture = func();
                            unwrap(std::move(innerFuture));
                            return;
                        } else if constexpr (std::is_void_v<U>) {
                            func();
                        } else {
                            outputSlot->store(func());
                        }
                    } else {
                        if constexpr (needsUnwrap) {
                            auto innerFuture = func(inputSlot->peek());
                            unwrap(std::move(innerFuture));
                            return;
                        } else if constexpr (std::is_void_v<U>) {
                            func(inputSlot->peek());
                        } else {
                            outputSlot->store(func(inputSlot->peek()));
                        }
                    }
                } catch (...) {
                    outputSlot->storeError(std::current_exception());
                }
                orkester_promise_resolve(promise);
                delete this;
            }

            void unwrap(Future<U>&& innerFuture) {
                auto innerSlot = CesiumImpl::getSlot(innerFuture);
                auto innerSignal = CesiumImpl::stealSignal(innerFuture);

                struct Forwarder {
                    std::shared_ptr<CesiumImpl::ValueSlot<U>> innerSlot;
                    std::shared_ptr<CesiumImpl::ValueSlot<U>> outputSlot;
                    orkester_promise_t promise;

                    static void invoke(void* ctx) {
                        auto* fwd = static_cast<Forwarder*>(ctx);
                        try {
                            if (fwd->innerSlot->hasError()) {
                                fwd->outputSlot->storeError(CesiumImpl::extractError(*fwd->innerSlot));
                            } else if constexpr (!std::is_void_v<U>) {
                                fwd->outputSlot->store(fwd->innerSlot->take());
                            }
                        } catch (...) {
                            fwd->outputSlot->storeError(std::current_exception());
                        }
                        orkester_promise_resolve(fwd->promise);
                        delete fwd;
                    }
                };

                auto* fwd = new Forwarder{innerSlot, outputSlot, promise};
                orkester_future_then(innerSignal, orkester_context_t_ORKESTER_IMMEDIATE, &Forwarder::invoke, fwd);
                delete this;
            }

            void unwrap(SharedFuture<U>&& innerShared) {
                auto slot = CesiumImpl::getSlot(innerShared);
                orkester_future_t sig = CesiumImpl::stealSignal(innerShared);
                Future<U> unique(system, std::move(slot), sig);
                unwrap(std::move(unique));
            }
        };

        auto* adapter = new Adapter{_slot, nextSlot, std::forward<Func>(f), outPromise, _system};
        orkester_shared_future_then_in_pool(_signal, pool._pool, &Adapter::invoke, adapter);
        return Future<U>(_system, nextSlot, outFuture);
    }

    /// Generic catch chaining using context enum.
    template <typename Func>
    Future<T> doCatch(
        orkester_context_t context,
        Func&& f)
    {
        auto nextSlot = std::make_shared<CesiumImpl::ValueSlot<T>>();

        orkester_promise_t outPromise = nullptr;
        orkester_future_t outFuture = nullptr;
        orkester_promise_create(_system, &outPromise, &outFuture);

        struct CatchAdapter {
            std::shared_ptr<CesiumImpl::ValueSlot<T>> inputSlot;
            std::shared_ptr<CesiumImpl::ValueSlot<T>> outputSlot;
            std::decay_t<Func> func;
            orkester_promise_t promise;

            static void invoke(void* ctx) {
                auto* self = static_cast<CatchAdapter*>(ctx);
                try {
                    if (self->inputSlot->hasError()) {
                        try {
                            (void)self->inputSlot->peek(); // rethrows
                        } catch (std::exception& e) {
                            if constexpr (std::is_void_v<T>) {
                                self->func(std::move(e));
                            } else {
                                self->outputSlot->store(self->func(std::move(e)));
                            }
                            orkester_promise_resolve(self->promise);
                            delete self;
                            return;
                        }
                    }
                    // No error — pass through
                    if constexpr (!std::is_void_v<T>) {
                        self->outputSlot->store(self->inputSlot->peek());
                    }
                } catch (...) {
                    self->outputSlot->storeError(std::current_exception());
                }
                orkester_promise_resolve(self->promise);
                delete self;
            }
        };

        auto* adapter = new CatchAdapter{_slot, nextSlot, std::forward<Func>(f), outPromise};
        orkester_shared_future_then(_signal, context, &CatchAdapter::invoke, adapter);
        return Future<T>(_system, nextSlot, outFuture);
    }

    const orkester_async_t* _system = nullptr;
    std::shared_ptr<CesiumImpl::ValueSlot<T>> _slot;
    orkester_shared_future_t _signal = nullptr;
};

// ─── void specialization ────────────────────────────────────────────────────

template <>
inline void SharedFuture<void>::wait() const {
    const char* errPtr = nullptr;
    size_t errLen = 0;
    bool ok = orkester_shared_future_wait(_signal, &errPtr, &errLen);
    if (!ok && errPtr) {
        std::string msg(errPtr, errLen);
        orkester_string_drop(errPtr, errLen);
        throw std::runtime_error(msg);
    }
    _slot->peek();
}

// ─── Helper function definitions for SharedFuture ───────────────────────────

namespace CesiumImpl {

template <typename V>
orkester_future_t stealSignal(SharedFuture<V>& f) {
    orkester_future_t unique = orkester_shared_future_into_unique(f._signal);
    f._signal = nullptr;
    return unique;
}

template <typename V>
std::shared_ptr<ValueSlot<V>> getSlot(SharedFuture<V>& f) {
    return f._slot;
}

} // namespace CesiumImpl

} // namespace CesiumAsync
