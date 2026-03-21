#pragma once

#include <CesiumAsync/Future.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumAsync/Library.h>
#include <CesiumAsync/Promise.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumAsync/ThreadPool.h>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace CesiumAsync {

class ITaskProcessor;
class ThreadPool;

class CESIUMASYNC_API AsyncSystem final {
public:
    explicit AsyncSystem(
        const std::shared_ptr<ITaskProcessor>& pTaskProcessor) noexcept {
        auto* ctx = new std::shared_ptr<ITaskProcessor>(pTaskProcessor);

        _system = orkester_async_create(
            // dispatch: schedule work(work_data) on a background thread
            [](void* context, orkester_callback_fn_t work, void* work_data) {
                auto* p = static_cast<std::shared_ptr<ITaskProcessor>*>(context);
                (*p)->startTask([work, work_data]() { work(work_data); });
            },
            static_cast<void*>(ctx),
            // destroy: clean up the shared_ptr
            [](void* context) {
                delete static_cast<std::shared_ptr<ITaskProcessor>*>(context);
            }
        );
    }

    ~AsyncSystem() noexcept {
        if (_system) orkester_async_destroy(_system);
    }

    // Copyable (cheap Arc clone on Rust side)
    AsyncSystem(const AsyncSystem& other) noexcept
        : _system(other._system ? orkester_async_clone(other._system) : nullptr) {}

    AsyncSystem& operator=(const AsyncSystem& other) noexcept {
        if (this != &other) {
            if (_system) orkester_async_destroy(_system);
            _system = other._system ? orkester_async_clone(other._system) : nullptr;
        }
        return *this;
    }

    AsyncSystem(AsyncSystem&& other) noexcept : _system(other._system) {
        other._system = nullptr;
    }
    AsyncSystem& operator=(AsyncSystem&& other) noexcept {
        if (this != &other) {
            if (_system) orkester_async_destroy(_system);
            _system = other._system;
            other._system = nullptr;
        }
        return *this;
    }

    /// Create a Future via callback that receives a Promise.
    template <typename T, typename Func>
    Future<T> createFuture(Func&& f) const {
        auto slot = std::make_shared<CesiumImpl::ValueSlot<T>>();

        orkester_promise_t promiseHandle = nullptr;
        orkester_future_t futureHandle = nullptr;
        orkester_promise_create(_system, &promiseHandle, &futureHandle);

        f(Promise<T>(_system, slot, promiseHandle, nullptr));
        return Future<T>(_system, slot, futureHandle);
    }

    /// Create a Promise<T> directly.
    template <typename T>
    Promise<T> createPromise() const {
        auto slot = std::make_shared<CesiumImpl::ValueSlot<T>>();

        orkester_promise_t promiseHandle = nullptr;
        orkester_future_t futureHandle = nullptr;
        orkester_promise_create(_system, &promiseHandle, &futureHandle);

        return Promise<T>(_system, slot, promiseHandle, futureHandle);
    }

    /// Create a pre-resolved Future<T>.
    template <typename T>
    Future<T> createResolvedFuture(T&& value) const {
        auto slot = std::make_shared<CesiumImpl::ValueSlot<T>>();
        slot->store(std::forward<T>(value));

        auto signal = orkester_future_create_resolved(_system);
        return Future<T>(_system, slot, signal);
    }

    /// Create a pre-resolved Future<void>.
    Future<void> createResolvedFuture() const {
        auto slot = std::make_shared<CesiumImpl::ValueSlot<void>>();
        auto signal = orkester_future_create_resolved(_system);
        return Future<void>(_system, slot, signal);
    }

    /// Run a function in a worker thread.
    template <typename Func>
    auto runInWorkerThread(Func&& f) const {
        using RawResult = std::invoke_result_t<Func>;
        using U = typename CesiumImpl::RemoveFuture<RawResult>::type;
        constexpr bool needsUnwrap = CesiumImpl::IsFuture<RawResult>::value;

        if constexpr (needsUnwrap) {
            // Delegate to chain which auto-unwraps via orchestration protocol.
            return createResolvedFuture().thenInWorkerThread(std::forward<Func>(f));
        } else {
            auto outputSlot = std::make_shared<CesiumImpl::ValueSlot<U>>();

            struct WorkerAdapter {
                std::decay_t<Func> func;
                std::shared_ptr<CesiumImpl::ValueSlot<U>> slot;

                static void invoke(void* ctx) {
                    auto* self = static_cast<WorkerAdapter*>(ctx);
                    try {
                        if constexpr (std::is_void_v<U>) {
                            self->func();
                        } else {
                            self->slot->store(self->func());
                        }
                    } catch (...) {
                        self->slot->storeError(std::current_exception());
                    }
                    delete self;
                }
            };

            auto* adapter = new WorkerAdapter{std::forward<Func>(f), outputSlot};
            auto signal = orkester_async_run_in_worker(_system, &WorkerAdapter::invoke, adapter);
            return Future<U>(_system, outputSlot, signal);
        }
    }

    /// Run a function in the main thread.
    template <typename Func>
    auto runInMainThread(Func&& f) const {
        using RawResult = std::invoke_result_t<Func>;
        using U = typename CesiumImpl::RemoveFuture<RawResult>::type;
        constexpr bool needsUnwrap = CesiumImpl::IsFuture<RawResult>::value;

        if constexpr (needsUnwrap) {
            // Delegate to chain which auto-unwraps via orchestration protocol.
            return createResolvedFuture().thenInMainThread(std::forward<Func>(f));
        } else {
            auto outputSlot = std::make_shared<CesiumImpl::ValueSlot<U>>();

            struct MainAdapter {
                std::decay_t<Func> func;
                std::shared_ptr<CesiumImpl::ValueSlot<U>> slot;

                static void invoke(void* ctx) {
                    auto* self = static_cast<MainAdapter*>(ctx);
                    try {
                        if constexpr (std::is_void_v<U>) {
                            self->func();
                        } else {
                            self->slot->store(self->func());
                        }
                    } catch (...) {
                        self->slot->storeError(std::current_exception());
                    }
                    delete self;
                }
            };

            auto* adapter = new MainAdapter{std::forward<Func>(f), outputSlot};
            auto signal = orkester_async_run_in_main(_system, &MainAdapter::invoke, adapter);
            return Future<U>(_system, outputSlot, signal);
        }
    }

    /// Run a function in a thread pool.
    template <typename Func>
    auto runInThreadPool(const ThreadPool& pool, Func&& f) const {
        using RawResult = std::invoke_result_t<Func>;
        using U = typename CesiumImpl::RemoveFuture<RawResult>::type;
        constexpr bool needsUnwrap = CesiumImpl::IsFuture<RawResult>::value;

        if constexpr (needsUnwrap) {
            return createResolvedFuture().thenInThreadPool(pool, std::forward<Func>(f));
        } else {
            auto outputSlot = std::make_shared<CesiumImpl::ValueSlot<U>>();

            struct PoolAdapter {
                std::decay_t<Func> func;
                std::shared_ptr<CesiumImpl::ValueSlot<U>> slot;

                static void invoke(void* ctx) {
                    auto* self = static_cast<PoolAdapter*>(ctx);
                    try {
                        if constexpr (std::is_void_v<U>) {
                            self->func();
                        } else {
                            self->slot->store(self->func());
                        }
                    } catch (...) {
                        self->slot->storeError(std::current_exception());
                    }
                    delete self;
                }
            };

            auto* adapter = new PoolAdapter{std::forward<Func>(f), outputSlot};
            auto signal = orkester_async_run_in_pool(_system, pool._pool, &PoolAdapter::invoke, adapter);
            return Future<U>(_system, outputSlot, signal);
        }
    }

    /// Wait for all futures to complete.
    template <typename T>
    Future<std::vector<T>> all(std::vector<Future<T>>&& futures) const {
        if (futures.empty()) {
            return createResolvedFuture(std::vector<T>{});
        }

        auto results = std::make_shared<std::vector<T>>();
        results->reserve(futures.size());

        // Chain each future sequentially: wait for its signal,
        // then take the result and accumulate.
        Future<void> chain = createResolvedFuture();
        for (auto& f : futures) {
            auto slot = f._slot;
            // Chain our void chain through this future's signal.
            // After our chain resolves, schedule a continuation on
            // this future's signal so we wait for both.
            chain = std::move(chain).thenImmediately(
                [system = _system, slot, signal = f._signal, results]() mutable {
                    // Create a future from this signal+slot so we
                    // can chain on it properly.
                    Future<T> inner(system, slot, signal);
                    return std::move(inner).thenImmediately(
                        [results](T&& value) {
                            results->push_back(std::move(value));
                        });
                });
            f._signal = nullptr;
        }

        return std::move(chain).thenImmediately(
            [results]() { return std::move(*results); });
    }

    /// Wait for all shared futures to complete.
    template <typename T>
    Future<std::vector<T>> all(std::vector<SharedFuture<T>>&& futures) const {
        if (futures.empty()) {
            return createResolvedFuture(std::vector<T>{});
        }

        auto results = std::make_shared<std::vector<T>>();
        results->reserve(futures.size());

        Future<void> chain = createResolvedFuture();
        for (auto& f : futures) {
            auto slot = f._slot;
            auto sharedSig = f._signal;
            chain = std::move(chain).thenImmediately(
                [system = _system, slot, sharedSig, results]() mutable {
                    struct Noop { static void invoke(void*) {} };
                    orkester_future_t sig = orkester_shared_future_then(
                        sharedSig, orkester_context_t_ORKESTER_IMMEDIATE, &Noop::invoke, nullptr);
                    Future<T> inner(system, slot, sig);
                    return std::move(inner).thenImmediately(
                        [results](T&& value) {
                            results->push_back(std::move(value));
                        });
                });
        }

        return std::move(chain).thenImmediately(
            [results]() { return std::move(*results); });
    }

    /// Dispatch queued main-thread tasks.
    void dispatchMainThreadTasks() const {
        orkester_async_dispatch(_system);
    }

    /// Dispatch one main-thread task.
    bool dispatchOneMainThreadTask() const {
        return orkester_async_dispatch_one(_system);
    }

    /// RAII scope that designates the current thread as the "main thread".
    /// While this scope is alive, continuations scheduled for the main thread
    /// will run inline on the current thread.
    class MainThreadScope {
    public:
        MainThreadScope(const MainThreadScope&) = delete;
        MainThreadScope& operator=(const MainThreadScope&) = delete;

        MainThreadScope(MainThreadScope&& rhs) noexcept : _scope(rhs._scope) {
            rhs._scope = nullptr;
        }
        MainThreadScope& operator=(MainThreadScope&& rhs) noexcept {
            if (this != &rhs) {
                if (_scope) orkester_main_thread_scope_drop(_scope);
                _scope = rhs._scope;
                rhs._scope = nullptr;
            }
            return *this;
        }

        ~MainThreadScope() noexcept {
            if (_scope) orkester_main_thread_scope_drop(_scope);
        }

    private:
        friend class AsyncSystem;
        explicit MainThreadScope(orkester_main_thread_scope_t scope) : _scope(scope) {}
        orkester_main_thread_scope_t _scope = nullptr;
    };

    /// Enter a scope in which the calling thread is treated as the main thread.
    MainThreadScope enterMainThread() const {
        return MainThreadScope(orkester_main_thread_scope_create(_system));
    }

    /// Create a thread pool with the given number of threads.
    ThreadPool createThreadPool(int32_t numberOfThreads) const {
        return ThreadPool(numberOfThreads);
    }

    bool operator==(const AsyncSystem& rhs) const noexcept {
        return _system == rhs._system;
    }

    bool operator!=(const AsyncSystem& rhs) const noexcept {
        return _system != rhs._system;
    }

private:
    orkester_async_t* _system = nullptr;
};

} // namespace CesiumAsync
