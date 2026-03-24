/// @file TestAsyncSystemOrkester.cpp
/// Tests for the orkester-backed CesiumAsync implementation.
///
/// Uses only CesiumAsync's public API.
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumAsync/Promise.h>
#include <CesiumAsync/SharedFuture.h>

#include <doctest/doctest.h>

#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

namespace {

class MockTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  void startTask(std::function<void()> f) override {
    // Run on a detached thread (simplest background execution)
    std::thread(std::move(f)).detach();
  }
};

} // namespace

TEST_CASE("orkester: create_resolved_future") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.createResolvedFuture(42);
  CHECK(future.isReady());
  int result = std::move(future).wait();
  CHECK(result == 42);
}

TEST_CASE("orkester: create_resolved_void_future") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.createResolvedFuture();
  CHECK(future.isReady());
  std::move(future).wait();
}

TEST_CASE("orkester: run_in_worker_thread") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.runInWorkerThread([]() -> int { return 100 + 23; });

  int result = std::move(future).wait();
  CHECK(result == 123);
}

TEST_CASE("orkester: then_in_worker_thread chained") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future =
      system.createResolvedFuture(10)
          .thenInWorkerThread([](int value) -> int { return value * 2; })
          .thenInWorkerThread([](int value) -> std::string {
            return "Result: " + std::to_string(value);
          });

  std::string result = std::move(future).wait();
  CHECK(result == "Result: 20");
}

TEST_CASE("orkester: then_immediately") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.createResolvedFuture(5).thenImmediately(
      [](int value) -> int { return value + 1; });

  int result = std::move(future).wait();
  CHECK(result == 6);
}

TEST_CASE("orkester: promise_resolve") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto promise = system.createPromise<std::string>();
  auto future = promise.getFuture();

  // Resolve from another thread
  std::thread([p = std::move(promise)]() mutable {
    p.resolve(std::string("hello from promise"));
  }).detach();

  std::string result = std::move(future).wait();
  CHECK(result == "hello from promise");
}

TEST_CASE("orkester: shared_future") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto sharedFuture = system.createResolvedFuture(42).share();
  CHECK(sharedFuture.isReady());

  // Clone
  auto clone = sharedFuture;
  CHECK(clone.isReady());

  // Wait on both
  CHECK(sharedFuture.wait() == 42);
  CHECK(clone.wait() == 42);
}

TEST_CASE("orkester: create_future") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.createFuture<int>([](CesiumAsync::Promise<int> promise) {
    std::thread([p = std::move(promise)]() mutable {
      p.resolve(999);
    }).detach();
  });

  int result = std::move(future).wait();
  CHECK(result == 999);
}

TEST_CASE("orkester: dispatch_main_thread no-op") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  // This should not crash even with no pending tasks
  system.dispatchMainThreadTasks();
  bool dispatched = system.dispatchOneMainThreadTask();
  CHECK(!dispatched); // nothing queued
}

TEST_CASE("orkester: async_system_copy") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  // Copy construct
  CesiumAsync::AsyncSystem copy = system;

  // Both should work independently
  auto f1 = system.createResolvedFuture(10);
  auto f2 = copy.createResolvedFuture(20);

  CHECK(std::move(f1).wait() == 10);
  CHECK(std::move(f2).wait() == 20);

  // Copy assign
  CesiumAsync::AsyncSystem copy2(pProcessor);
  copy2 = system;

  auto f3 = copy2.createResolvedFuture(30);
  CHECK(std::move(f3).wait() == 30);
}

TEST_CASE("orkester: enter_main_thread") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.runInMainThread([]() -> int { return 42; });

  {
    auto scope = system.enterMainThread();
    system.dispatchMainThreadTasks();
  }

  int result = std::move(future).wait();
  CHECK(result == 42);
}

TEST_CASE("orkester: then_in_main_thread") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.createResolvedFuture(7).thenInMainThread(
      [](int value) -> int { return value * 3; });

  {
    auto scope = system.enterMainThread();
    for (int i = 0; i < 10; ++i) {
      system.dispatchMainThreadTasks();
      if (future.isReady())
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  int result = std::move(future).wait();
  CHECK(result == 21);
}

TEST_CASE("orkester: all_futures") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  std::vector<CesiumAsync::Future<int>> futures;
  futures.push_back(system.createResolvedFuture(1));
  futures.push_back(system.createResolvedFuture(2));
  futures.push_back(system.createResolvedFuture(3));

  auto allFuture = system.all(std::move(futures));
  auto results = std::move(allFuture).wait();

  CHECK(results.size() == 3);
  CHECK(results[0] == 1);
  CHECK(results[1] == 2);
  CHECK(results[2] == 3);
}

TEST_CASE("orkester: all_empty") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  std::vector<CesiumAsync::Future<int>> empty;
  auto allFuture = system.all(std::move(empty));
  auto results = std::move(allFuture).wait();
  CHECK(results.empty());
}

TEST_CASE("orkester: catch_immediately") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.createResolvedFuture(0)
                    .thenImmediately([](int) -> int {
                      throw std::runtime_error("test error");
                    })
                    .catchImmediately(
                        [](const std::exception& /*e*/) -> int { return -1; });

  int result = std::move(future).wait();
  CHECK(result == -1);
}

TEST_CASE("orkester: catch_in_main_thread") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.createResolvedFuture(0)
                    .thenImmediately([](int) -> int {
                      throw std::runtime_error("main thread error");
                    })
                    .catchInMainThread(
                        [](const std::exception& /*e*/) -> int { return -99; });

  {
    auto scope = system.enterMainThread();
    for (int i = 0; i < 10; ++i) {
      system.dispatchMainThreadTasks();
      if (future.isReady())
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }

  int result = std::move(future).wait();
  CHECK(result == -99);
}

TEST_CASE("orkester: then_pass_through") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto future = system.createResolvedFuture(42).thenPassThrough(
      std::string("extra"),
      100);

  auto [str, num, value] = std::move(future).wait();
  CHECK(str == "extra");
  CHECK(num == 100);
  CHECK(value == 42);
}

TEST_CASE("orkester: create_thread_pool") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto pool = system.createThreadPool(4);

  auto future =
      system.createResolvedFuture(5).thenInThreadPool(pool, [](int v) -> int {
        return v + 10;
      });

  int result = std::move(future).wait();
  CHECK(result == 15);
}

TEST_CASE("orkester: run_in_thread_pool") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem system(pProcessor);

  auto pool = system.createThreadPool(2);

  auto future = system.runInThreadPool(pool, []() -> int { return 77; });

  int result = std::move(future).wait();
  CHECK(result == 77);
}

TEST_CASE("orkester: equality_operators") {
  auto pProcessor = std::make_shared<MockTaskProcessor>();
  CesiumAsync::AsyncSystem s1(pProcessor);
  CesiumAsync::AsyncSystem s2(pProcessor);

  CHECK(s1 != s2);
  CHECK(!(s1 == s2));
}
