#pragma once

/// @file Future.h
/// Drop-in replacement for CesiumAsync::Future<T> backed by orkester.
///
/// Architecture: Rust manages scheduling via Future<()> (completion signals).
/// C++ manages typed values via shared_ptr<ValueSlot<T>>.
/// No values cross the FFI boundary.

#include <CesiumAsync/Impl/OrkesterImpl.h>
#include <CesiumAsync/ThreadPool.h>

#include <string>
#include <tuple>

namespace CesiumAsync {

// ─── Future<T> ──────────────────────────────────────────────────────────────

/// Drop-in replacement for CesiumAsync::Future<T>, backed by orkester.
///
/// Holds a shared_ptr<ValueSlot<T>> (C++ side, typed) and a orkester_future_t
/// (Rust side, scheduling). Values never cross the FFI boundary.
template <typename T>
class CESIUMASYNC_API Future final {
public:
    Future(const Future&) = delete;
    Future& operator=(const Future&) = delete;

    Future(Future&& rhs) noexcept
        : _system(rhs._system), _slot(std::move(rhs._slot)), _signal(rhs._signal) {
        rhs._signal = nullptr;
    }

    Future& operator=(Future&& rhs) noexcept {
        if (this != &rhs) {
            if (_signal) orkester_future_drop(_signal);
            _system = rhs._system;
            _slot = std::move(rhs._slot);
            _signal = rhs._signal;
            rhs._signal = nullptr;
        }
        return *this;
    }

    ~Future() noexcept {
        if (_signal) orkester_future_drop(_signal);
    }

    bool isReady() const {
        return _signal && orkester_future_is_ready(_signal);
    }

    /// Block until resolved. Consumes the future.
    T wait() {
        waitOnSignal();
        return _slot->take();
    }

    /// Block in main thread.
    T waitInMainThread() {
        const char* errPtr = nullptr;
        size_t errLen = 0;
        bool ok = orkester_future_wait_in_main(_signal, &errPtr, &errLen);
        _signal = nullptr;
        if (!ok) {
            _slot->storeError(std::make_exception_ptr(
                std::runtime_error(makeErrorMsg(errPtr, errLen))));
        }
        return _slot->take();
    }

    /// Chain: worker thread.
    template <typename Func>
    CesiumImpl::ContinuationFutureType_t<Func, T>
    thenInWorkerThread(Func&& f) && {
        return chain<Func>(orkester_context_t_ORKESTER_WORKER, std::forward<Func>(f));
    }

    /// Chain: main thread.
    template <typename Func>
    CesiumImpl::ContinuationFutureType_t<Func, T>
    thenInMainThread(Func&& f) && {
        return chain<Func>(orkester_context_t_ORKESTER_MAIN, std::forward<Func>(f));
    }

    /// Chain: immediately (inline).
    template <typename Func>
    CesiumImpl::ContinuationFutureType_t<Func, T>
    thenImmediately(Func&& f) && {
        return chain<Func>(orkester_context_t_ORKESTER_IMMEDIATE, std::forward<Func>(f));
    }

    /// Chain in a thread pool.
    template <typename Func>
    CesiumImpl::ContinuationFutureType_t<Func, T>
    thenInThreadPool(const ThreadPool& pool, Func&& f) && {
        return chainInPool<Func>(pool, std::forward<Func>(f));
    }

    /// Catch errors in the main thread.
    template <typename Func>
    Future<T> catchInMainThread(Func&& f) && {
        return doCatch(orkester_context_t_ORKESTER_MAIN, std::forward<Func>(f));
    }

    /// Catch errors immediately.
    template <typename Func>
    Future<T> catchImmediately(Func&& f) && {
        return doCatch(orkester_context_t_ORKESTER_IMMEDIATE, std::forward<Func>(f));
    }

    /// Pass through additional values alongside the result.
    template <typename... TPassThrough>
    Future<std::tuple<std::remove_cvref_t<TPassThrough>..., T>>
    thenPassThrough(TPassThrough&&... values) && {
        return std::move(*this).thenImmediately(
            [... captured = std::forward<TPassThrough>(values)](T&& result) mutable {
                return std::make_tuple(std::move(captured)..., std::move(result));
            });
    }

    /// Convert to SharedFuture.
    SharedFuture<T> share() && {
        auto sharedSignal = orkester_future_share(_signal);
        _signal = nullptr;
        return SharedFuture<T>(_system, _slot, sharedSignal);
    }

private:
    friend class AsyncSystem;
    template <typename R> friend class Future;
    template <typename R> friend class SharedFuture;
    template <typename R> friend class Promise;
    template <typename V> friend orkester_future_t CesiumImpl::stealSignal(Future<V>&);
    template <typename V> friend std::shared_ptr<CesiumImpl::ValueSlot<V>> CesiumImpl::getSlot(Future<V>&);

    Future(const orkester_async_t* system,
           std::shared_ptr<CesiumImpl::ValueSlot<T>> slot,
           orkester_future_t signal)
        : _system(system), _slot(std::move(slot)), _signal(signal) {}

    /// Wait on the Rust signal, handling errors.
    void waitOnSignal() {
        const char* errPtr = nullptr;
        size_t errLen = 0;
        bool ok = orkester_future_wait(_signal, &errPtr, &errLen);
        _signal = nullptr;
        if (!ok) {
            _slot->storeError(std::make_exception_ptr(
                std::runtime_error(makeErrorMsg(errPtr, errLen))));
        }
    }

    static std::string makeErrorMsg(const char* ptr, size_t len) {
        if (!ptr) return "unknown orkester error";
        std::string msg(ptr, len);
        orkester_string_drop(ptr, len);
        return msg;
    }

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
                            auto innerFuture = func(inputSlot->take());
                            unwrap(std::move(innerFuture));
                            return;
                        } else if constexpr (std::is_void_v<U>) {
                            func(inputSlot->take());
                        } else {
                            outputSlot->store(func(inputSlot->take()));
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
        orkester_future_then(_signal, context, &Adapter::invoke, adapter);
        _signal = nullptr;
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
                            auto innerFuture = func(inputSlot->take());
                            unwrap(std::move(innerFuture));
                            return;
                        } else if constexpr (std::is_void_v<U>) {
                            func(inputSlot->take());
                        } else {
                            outputSlot->store(func(inputSlot->take()));
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
        orkester_future_then_in_pool(_signal, pool._pool, &Adapter::invoke, adapter);
        _signal = nullptr;
        return Future<U>(_system, nextSlot, outFuture);
    }

    /// Generic catch chaining using context enum + simple callbacks.
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
                            (void)self->inputSlot->take(); // rethrows
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
                        self->outputSlot->store(self->inputSlot->take());
                    }
                } catch (...) {
                    self->outputSlot->storeError(std::current_exception());
                }
                orkester_promise_resolve(self->promise);
                delete self;
            }
        };

        auto* adapter = new CatchAdapter{_slot, nextSlot, std::forward<Func>(f), outPromise};
        orkester_future_then(_signal, context, &CatchAdapter::invoke, adapter);
        _signal = nullptr;
        return Future<T>(_system, nextSlot, outFuture);
    }

    const orkester_async_t* _system = nullptr;
    std::shared_ptr<CesiumImpl::ValueSlot<T>> _slot;
    orkester_future_t _signal = nullptr;
};

// ─── void specializations ───────────────────────────────────────────────────

template <>
inline void Future<void>::wait() {
    waitOnSignal();
    _slot->take(); // rethrows if error
}

template <>
inline void Future<void>::waitInMainThread() {
    const char* errPtr = nullptr;
    size_t errLen = 0;
    bool ok = orkester_future_wait_in_main(_signal, &errPtr, &errLen);
    _signal = nullptr;
    if (!ok) {
        _slot->storeError(std::make_exception_ptr(
            std::runtime_error(makeErrorMsg(errPtr, errLen))));
    }
    _slot->take();
}

// ─── Helper function definitions ────────────────────────────────────────────

namespace CesiumImpl {

template <typename V>
orkester_future_t stealSignal(Future<V>& f) {
    orkester_future_t sig = f._signal;
    f._signal = nullptr;
    return sig;
}

template <typename V>
std::shared_ptr<ValueSlot<V>> getSlot(Future<V>& f) {
    return f._slot;
}

} // namespace CesiumImpl

} // namespace CesiumAsync
