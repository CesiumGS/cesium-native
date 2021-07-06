#include "CesiumAsync/AsyncSystem.h"
#include <catch2/catch.hpp>
#include <chrono>
#include <memory>
#include <thread>

using namespace CesiumAsync;

class MockTaskProcessor : public ITaskProcessor {
public:
  std::atomic<int32_t> tasksStarted = 0;

  virtual void startTask(std::function<void()> f) {
    ++tasksStarted;
    std::thread(f).detach();
  }
};

TEST_CASE("AsyncSystem") {
  std::shared_ptr<MockTaskProcessor> pTaskProcessor =
      std::make_shared<MockTaskProcessor>();
  AsyncSystem asyncSystem(pTaskProcessor);

  SECTION("runs worker tasks with the task processor") {
    bool executed = false;

    asyncSystem.runInWorkerThread([&executed]() { executed = true; }).wait();

    CHECK(pTaskProcessor->tasksStarted == 1);
    CHECK(executed);
  }

  SECTION("worker continuations are run via the task processor") {
    bool executed = false;

    asyncSystem.createResolvedFuture()
        .thenInWorkerThread([&executed]() { executed = true; })
        .wait();

    CHECK(pTaskProcessor->tasksStarted == 1);
    CHECK(executed);
  }

  SECTION("runs main thread tasks when instructed") {
    bool executed = false;

    auto future =
        asyncSystem.runInMainThread([&executed]() { executed = true; });
    CHECK(asyncSystem.dispatchZeroOrOneMainThreadTask() == true);
    future.wait();

    CHECK(pTaskProcessor->tasksStarted == 0);
    CHECK(executed);
  }

  SECTION("main thread continuations are run when instructed") {
    bool executed = false;

    auto future = asyncSystem.createResolvedFuture().thenInMainThread(
        [&executed]() { executed = true; });
    CHECK(asyncSystem.dispatchZeroOrOneMainThreadTask() == true);
    future.wait();

    CHECK(pTaskProcessor->tasksStarted == 0);
    CHECK(executed);
  }

  SECTION("worker continuations following a worker run immediately") {
    bool executed1 = false;
    bool executed2 = false;

    asyncSystem.runInWorkerThread([&executed1]() { executed1 = true; })
        .thenInWorkerThread([&executed2]() { executed2 = true; })
        .wait();

    CHECK(pTaskProcessor->tasksStarted == 1);
    CHECK(executed1);
    CHECK(executed2);
  }

  SECTION("main thread continuations following a main thread task run "
          "immediately") {
    bool executed1 = false;
    bool executed2 = false;

    auto future =
        asyncSystem.runInMainThread([&executed1]() { executed1 = true; })
            .thenInMainThread([&executed2]() { executed2 = true; });

    CHECK(asyncSystem.dispatchZeroOrOneMainThreadTask() == true);
    future.wait();

    CHECK(pTaskProcessor->tasksStarted == 0);
    CHECK(executed1);
    CHECK(executed2);
  }

  SECTION("worker continuations following a thread pool thread run as a "
          "separate task") {
    ThreadPool pool(1);

    bool executed1 = false;
    bool executed2 = false;
    bool executed3 = false;

    asyncSystem.runInWorkerThread([&executed1]() { executed1 = true; })
        .thenInThreadPool(pool, [&executed2]() { executed2 = true; })
        .thenInWorkerThread([&executed3]() { executed3 = true; })
        .wait();

    CHECK(pTaskProcessor->tasksStarted == 2);
    CHECK(executed1);
    CHECK(executed2);
    CHECK(executed3);
  }

  SECTION("a worker continuation that returns an already resolved future "
          "immediately invokes an attached worker continuation") {
    bool executed = false;

    asyncSystem
        .runInWorkerThread([asyncSystem, &executed]() {
          return asyncSystem.createResolvedFuture().thenInWorkerThread(
              [&executed]() { executed = true; });
        })
        .wait();

    CHECK(pTaskProcessor->tasksStarted == 1);
  }
}
