/// @file TestAsyncSystemOrkester.cpp
/// Test harness for the orkester-backed CesiumAsync implementation.
///
/// Uses only CesiumAsync's public API.
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumAsync/Promise.h>
#include <CesiumAsync/SharedFuture.h>

#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

// ─── Mock ITaskProcessor ──────────────────────────────────────────────────────

class MockTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
    void startTask(std::function<void()> f) override {
        // Run on a detached thread (simplest background execution)
        std::thread(std::move(f)).detach();
    }
};

// ─── Tests ────────────────────────────────────────────────────────────────────

void test_create_resolved_future() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto future = system.createResolvedFuture(42);
    assert(future.isReady());
    int result = std::move(future).wait();
    assert(result == 42);
    std::cout << "  PASS: create_resolved_future" << std::endl;
}

void test_create_resolved_void_future() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto future = system.createResolvedFuture();
    assert(future.isReady());
    std::move(future).wait();
    std::cout << "  PASS: create_resolved_void_future" << std::endl;
}

void test_run_in_worker_thread() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto future = system.runInWorkerThread([]() -> int {
        return 100 + 23;
    });

    int result = std::move(future).wait();
    assert(result == 123);
    std::cout << "  PASS: run_in_worker_thread" << std::endl;
}

void test_then_in_worker_thread() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto future = system.createResolvedFuture(10)
        .thenInWorkerThread([](int value) -> int {
            return value * 2;
        })
        .thenInWorkerThread([](int value) -> std::string {
            return "Result: " + std::to_string(value);
        });

    std::string result = std::move(future).wait();
    assert(result == "Result: 20");
    std::cout << "  PASS: then_in_worker_thread (chained)" << std::endl;
}

void test_then_immediately() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto future = system.createResolvedFuture(5)
        .thenImmediately([](int value) -> int {
            return value + 1;
        });

    int result = std::move(future).wait();
    assert(result == 6);
    std::cout << "  PASS: then_immediately" << std::endl;
}

void test_promise_resolve() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto promise = system.createPromise<std::string>();
    auto future = promise.getFuture();

    // Resolve from another thread
    std::thread([p = std::move(promise)]() mutable {
        p.resolve(std::string("hello from promise"));
    }).detach();

    std::string result = std::move(future).wait();
    assert(result == "hello from promise");
    std::cout << "  PASS: promise_resolve" << std::endl;
}

void test_shared_future() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto sharedFuture = system.createResolvedFuture(42).share();
    assert(sharedFuture.isReady());

    // Clone
    auto clone = sharedFuture;
    assert(clone.isReady());

    // Wait on both — values stay in C++ (no double-free possible)
    assert(sharedFuture.wait() == 42);
    assert(clone.wait() == 42);

    std::cout << "  PASS: shared_future" << std::endl;
}

void test_create_future() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto future = system.createFuture<int>(
        [](CesiumAsync::Promise<int> promise) {
            std::thread([p = std::move(promise)]() mutable {
                p.resolve(999);
            }).detach();
        });

    int result = std::move(future).wait();
    assert(result == 999);
    std::cout << "  PASS: create_future" << std::endl;
}

void test_dispatch_main_thread() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    // This should not crash even with no pending tasks
    system.dispatchMainThreadTasks();
    bool dispatched = system.dispatchOneMainThreadTask();
    assert(!dispatched); // nothing queued
    std::cout << "  PASS: dispatch_main_thread (no-op)" << std::endl;
}

void test_async_system_copy() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    // Copy construct
    CesiumAsync::AsyncSystem copy = system;

    // Both should work independently
    auto f1 = system.createResolvedFuture(10);
    auto f2 = copy.createResolvedFuture(20);

    assert(std::move(f1).wait() == 10);
    assert(std::move(f2).wait() == 20);

    // Copy assign
    CesiumAsync::AsyncSystem copy2(pProcessor);
    copy2 = system;

    auto f3 = copy2.createResolvedFuture(30);
    assert(std::move(f3).wait() == 30);

    std::cout << "  PASS: async_system_copy" << std::endl;
}

void test_enter_main_thread() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    // Schedule work on the main thread
    auto future = system.runInMainThread([]() -> int {
        return 42;
    });

    // Enter main-thread scope so we can dispatch
    {
        auto scope = system.enterMainThread();

        // Dispatch tasks while in scope
        system.dispatchMainThreadTasks();
    }
    // Scope dropped — thread is no longer "main"

    int result = std::move(future).wait();
    assert(result == 42);
    std::cout << "  PASS: enter_main_thread" << std::endl;
}

void test_then_in_main_thread() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto future = system.createResolvedFuture(7)
        .thenInMainThread([](int value) -> int {
            return value * 3;
        });

    // Enter main-thread scope and dispatch
    {
        auto scope = system.enterMainThread();
        for (int i = 0; i < 10; ++i) {
            system.dispatchMainThreadTasks();
            if (future.isReady()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    int result = std::move(future).wait();
    assert(result == 21);
    std::cout << "  PASS: then_in_main_thread" << std::endl;
}

void test_all_futures() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    std::vector<CesiumAsync::Future<int>> futures;
    futures.push_back(system.createResolvedFuture(1));
    futures.push_back(system.createResolvedFuture(2));
    futures.push_back(system.createResolvedFuture(3));

    auto allFuture = system.all(std::move(futures));
    auto results = std::move(allFuture).wait();

    assert(results.size() == 3);
    assert(results[0] == 1);
    assert(results[1] == 2);
    assert(results[2] == 3);
    std::cout << "  PASS: all_futures" << std::endl;
}

void test_all_empty() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    std::vector<CesiumAsync::Future<int>> empty;
    auto allFuture = system.all(std::move(empty));
    auto results = std::move(allFuture).wait();
    assert(results.empty());
    std::cout << "  PASS: all_empty" << std::endl;
}

void test_catch_immediately() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    // Create a future that throws
    auto future = system.createResolvedFuture(0)
        .thenImmediately([](int) -> int {
            throw std::runtime_error("test error");
        })
        .catchImmediately([](const std::exception& /*e*/) -> int {
            return -1;
        });

    int result = std::move(future).wait();
    assert(result == -1);
    std::cout << "  PASS: catch_immediately" << std::endl;
}

void test_catch_in_main_thread() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto future = system.createResolvedFuture(0)
        .thenImmediately([](int) -> int {
            throw std::runtime_error("main thread error");
        })
        .catchInMainThread([](const std::exception& /*e*/) -> int {
            return -99;
        });

    // Enter main-thread scope and dispatch
    {
        auto scope = system.enterMainThread();
        for (int i = 0; i < 10; ++i) {
            system.dispatchMainThreadTasks();
            if (future.isReady()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    int result = std::move(future).wait();
    assert(result == -99);
    std::cout << "  PASS: catch_in_main_thread" << std::endl;
}

void test_then_pass_through() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto future = system.createResolvedFuture(42)
        .thenPassThrough(std::string("extra"), 100);

    auto [str, num, value] = std::move(future).wait();
    assert(str == "extra");
    assert(num == 100);
    assert(value == 42);
    std::cout << "  PASS: then_pass_through" << std::endl;
}

void test_create_thread_pool() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    // createThreadPool should compile and return something usable
    auto pool = system.createThreadPool(4);

    // thenInThreadPool should compile and delegate to worker
    auto future = system.createResolvedFuture(5)
        .thenInThreadPool(pool, [](int v) -> int { return v + 10; });

    int result = std::move(future).wait();
    assert(result == 15);
    std::cout << "  PASS: create_thread_pool" << std::endl;
}

void test_run_in_thread_pool() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem system(pProcessor);

    auto pool = system.createThreadPool(2);

    auto future = system.runInThreadPool(pool, []() -> int { return 77; });

    int result = std::move(future).wait();
    assert(result == 77);
    std::cout << "  PASS: run_in_thread_pool" << std::endl;
}

void test_equality_operators() {
    auto pProcessor = std::make_shared<MockTaskProcessor>();
    CesiumAsync::AsyncSystem s1(pProcessor);
    CesiumAsync::AsyncSystem s2(pProcessor);
    CesiumAsync::AsyncSystem s1_copy = s1;

    assert(s1 != s2);     // different Rust handles
    assert(!(s1 == s2));
    // Note: copies have different handles (cloned Arc), so they are not equal
    // This matches the semantics: different orkester system instances

    std::cout << "  PASS: equality_operators" << std::endl;
}

// ─── Main ─────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "CesiumAsync (orkester) Tests" << std::endl;
    std::cout << "========================================" << std::endl;

    test_create_resolved_future();
    test_create_resolved_void_future();
    test_run_in_worker_thread();
    test_then_in_worker_thread();
    test_then_immediately();
    test_promise_resolve();
    test_shared_future();
    test_create_future();
    test_dispatch_main_thread();
    test_async_system_copy();
    test_enter_main_thread();
    test_then_in_main_thread();
    test_all_futures();
    test_all_empty();
    test_catch_immediately();
    test_catch_in_main_thread();
    test_then_pass_through();
    test_create_thread_pool();
    test_run_in_thread_pool();
    test_equality_operators();

    std::cout << std::endl;
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
