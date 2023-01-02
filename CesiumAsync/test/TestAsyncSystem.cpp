#include "CesiumAsync/AsyncSystem.h"

#include <catch2/catch.hpp>

#include <chrono>
#include <memory>
#include <thread>

using namespace CesiumAsync;

namespace {

class MockTaskProcessor : public ITaskProcessor {
public:
  std::atomic<int32_t> tasksStarted = 0;

  virtual void startTask(std::function<void()> f) {
    ++tasksStarted;
    std::thread(f).detach();
  }
};

} // namespace

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

    CHECK(!executed);
    bool taskDispatched = asyncSystem.dispatchOneMainThreadTask();
    CHECK(taskDispatched);
    CHECK(executed);
    CHECK(pTaskProcessor->tasksStarted == 0);
  }

  SECTION("main thread continuations are run when instructed") {
    bool executed = false;

    auto future = asyncSystem.createResolvedFuture().thenInMainThread(
        [&executed]() { executed = true; });

    CHECK(!executed);
    bool taskDispatched = asyncSystem.dispatchOneMainThreadTask();
    CHECK(taskDispatched);
    CHECK(executed);
    CHECK(pTaskProcessor->tasksStarted == 0);
  }

  SECTION("worker continuations following a worker run immediately") {
    bool executed1 = false;
    bool executed2 = false;

    Promise<void> promise = asyncSystem.createPromise<void>();
    Future<void> trigger = promise.getFuture();

    auto future = std::move(trigger)
                      .thenInWorkerThread([&executed1]() { executed1 = true; })
                      .thenInWorkerThread([&executed2]() { executed2 = true; });

    // Now that both continuations are attached, set the chain in motion.
    promise.resolve();
    future.wait();

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

    CHECK(!executed1);
    CHECK(!executed2);
    bool taskDispatched = asyncSystem.dispatchOneMainThreadTask();
    CHECK(taskDispatched);
    CHECK(executed1);
    CHECK(executed2);
    CHECK(pTaskProcessor->tasksStarted == 0);
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
          auto future = asyncSystem.createResolvedFuture().thenInWorkerThread(
              [&executed]() { executed = true; });

          // The above continuation should be complete by the time the
          // `thenInWorkerThread` returns.
          CHECK(executed);

          return future;
        })
        .wait();

    CHECK(pTaskProcessor->tasksStarted == 1);
    CHECK(executed);
  }

  SECTION("can pass move-only objects between continuations") {
    auto future =
        asyncSystem
            .runInWorkerThread([]() { return std::make_unique<int>(42); })
            .thenInWorkerThread(
                [](std::unique_ptr<int>&& pResult) { return *pResult; });
    CHECK(future.wait() == 42);
  }

  SECTION("an exception thrown in a continuation rejects the future") {
    auto future = asyncSystem.runInWorkerThread(
        []() { throw std::runtime_error("test"); });
    CHECK_THROWS_WITH(future.wait(), "test");
  }

  SECTION("an exception thrown in createFuture rejects the future") {
    auto future = asyncSystem.createFuture<int>(
        [](const auto& /*promise*/) { throw std::runtime_error("test"); });
    CHECK_THROWS_WITH(future.wait(), "test");
  }

  SECTION("createFuture promise may resolve immediately") {
    auto future = asyncSystem.createFuture<int>(
        [](const auto& promise) { promise.resolve(42); });
    CHECK(future.wait() == 42);
  }

  SECTION("createFuture promise may resolve later") {
    auto future = asyncSystem.createFuture<int>([](const auto& promise) {
      std::thread([promise]() {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        promise.resolve(42);
      }).detach();
    });
    CHECK(future.wait() == 42);
  }

  SECTION("rejected promise invokes catch instead of then") {
    auto future = asyncSystem
                      .createFuture<int>([](const auto& promise) {
                        promise.reject(std::runtime_error("test"));
                      })
                      .thenInMainThread([](int /*x*/) {
                        // This should not be invoked.
                        CHECK(false);
                        return 1;
                      })
                      .catchInMainThread([](std::exception&& e) {
                        CHECK(std::string(e.what()) == "test");
                        return 2;
                      });

    asyncSystem.dispatchOneMainThreadTask();
    CHECK(future.wait() == 2);
  }

  SECTION("catch may chain to another futre") {
    auto future = asyncSystem
                      .createFuture<int>([](const auto& promise) {
                        promise.reject(std::runtime_error("test"));
                      })
                      .catchInMainThread(
                          [asyncSystem](std::exception&& e) -> Future<int> {
                            CHECK(std::string(e.what()) == "test");
                            return asyncSystem.createResolvedFuture(2);
                          });

    asyncSystem.dispatchOneMainThreadTask();
    CHECK(future.wait() == 2);
  }

  SECTION("then after returning catch is invoked") {
    auto future = asyncSystem
                      .createFuture<int>([](const auto& promise) {
                        promise.reject(std::runtime_error("test"));
                      })
                      .catchInMainThread([](std::exception&& e) {
                        CHECK(std::string(e.what()) == "test");
                        return 2;
                      })
                      .thenInMainThread([](int x) {
                        CHECK(x == 2);
                        return 3;
                      });

    asyncSystem.dispatchOneMainThreadTask();
    CHECK(future.wait() == 3);
  }

  SECTION("then after throwing catch is not invoked") {
    auto future = asyncSystem
                      .createFuture<int>([](const auto& promise) {
                        promise.reject(std::runtime_error("test"));
                      })
                      .catchInMainThread([](std::exception&& e) -> int {
                        CHECK(std::string(e.what()) == "test");
                        throw std::runtime_error("second");
                      })
                      .thenInMainThread([](int /*x*/) {
                        // Should not be called
                        CHECK(false);
                        return 3;
                      });

    asyncSystem.dispatchOneMainThreadTask();
    CHECK_THROWS_WITH(future.wait(), "second");
  }

  SECTION("Future returned by all resolves when all given Futures resolve") {
    auto one = asyncSystem.createPromise<int>();
    auto two = asyncSystem.createPromise<int>();
    auto three = asyncSystem.createPromise<int>();

    std::vector<Future<int>> futures;
    futures.emplace_back(one.getFuture());
    futures.emplace_back(two.getFuture());
    futures.emplace_back(three.getFuture());

    auto all = asyncSystem.all(std::move(futures));

    bool resolved = false;
    auto last =
        std::move(all).thenImmediately([&resolved](std::vector<int>&& result) {
          CHECK(result.size() == 3);
          CHECK(result[0] == 1);
          CHECK(result[1] == 2);
          CHECK(result[2] == 3);
          resolved = true;
        });

    three.resolve(3);
    one.resolve(1);
    two.resolve(2);

    last.wait();
    CHECK(resolved);
  }

  SECTION("Future returned by 'all' rejects when any Future rejects") {
    auto one = asyncSystem.createPromise<int>();
    auto two = asyncSystem.createPromise<int>();
    auto three = asyncSystem.createPromise<int>();

    std::vector<Future<int>> futures;
    futures.emplace_back(one.getFuture());
    futures.emplace_back(two.getFuture());
    futures.emplace_back(three.getFuture());

    auto all = asyncSystem.all(std::move(futures));

    bool rejected = false;

    auto last = std::move(all)
                    .thenImmediately([](std::vector<int>&& /*result*/) {
                      // Should not happen.
                      CHECK(false);
                    })
                    .catchImmediately([&rejected](std::exception&& e) {
                      CHECK(std::string(e.what()) == "2");
                      rejected = true;
                    });

    three.resolve(3);
    one.resolve(1);
    two.reject(std::runtime_error("2"));

    last.wait();
    CHECK(rejected);
  }

  SECTION("When multiple futures in an 'all' reject, the data from the first "
          "rejection in the list is used") {
    auto one = asyncSystem.createPromise<int>();
    auto two = asyncSystem.createPromise<int>();
    auto three = asyncSystem.createPromise<int>();

    std::vector<Future<int>> futures;
    futures.emplace_back(one.getFuture());
    futures.emplace_back(two.getFuture());
    futures.emplace_back(three.getFuture());

    auto all = asyncSystem.all(std::move(futures));

    bool rejected = false;

    auto last = std::move(all)
                    .thenImmediately([](std::vector<int>&& /*result*/) {
                      // Should not happen.
                      CHECK(false);
                    })
                    .catchImmediately([&rejected](std::exception&& e) {
                      CHECK(std::string(e.what()) == "1");
                      CHECK(!rejected);
                      rejected = true;
                    });

    three.reject(std::runtime_error("3"));
    one.reject(std::runtime_error("1"));
    two.reject(std::runtime_error("2"));

    last.wait();
    CHECK(rejected);
  }

  SECTION("conversion to SharedFuture") {
    auto promise = asyncSystem.createPromise<int>();
    auto sharedFuture = promise.getFuture().share();

    bool executed1 = false;
    auto one = sharedFuture
                   .thenInWorkerThread([&executed1](int value) {
                     CHECK(value == 1);
                     CHECK(!executed1);
                     return 2;
                   })
                   .thenInWorkerThread([&executed1](int value) {
                     CHECK(value == 2);
                     CHECK(!executed1);
                     executed1 = true;
                     return 10;
                   });

    bool executed2 = false;
    auto two = sharedFuture
                   .thenInWorkerThread([&executed2](int value) {
                     CHECK(value == 1);
                     CHECK(!executed2);
                     return 2;
                   })
                   .thenInWorkerThread([&executed2](int value) {
                     CHECK(value == 2);
                     CHECK(!executed2);
                     executed2 = true;
                     return 11;
                   });

    promise.resolve(1);

    int value1 = one.wait();
    int value2 = two.wait();

    CHECK(executed1);
    CHECK(executed2);
    CHECK(value1 == 10);
    CHECK(value2 == 11);
  }

  SECTION("can join two chains originating with a shared future") {
    auto promise = asyncSystem.createPromise<int>();
    auto sharedFuture = promise.getFuture().share();

    bool executed1 = false;
    auto one = sharedFuture
                   .thenInWorkerThread([&executed1](int value) {
                     CHECK(value == 1);
                     CHECK(!executed1);
                     return 2;
                   })
                   .thenInWorkerThread([&executed1](int value) {
                     CHECK(value == 2);
                     CHECK(!executed1);
                     executed1 = true;
                     return 10;
                   });

    bool executed2 = false;
    auto two = sharedFuture
                   .thenInWorkerThread([&executed2](int value) {
                     CHECK(value == 1);
                     CHECK(!executed2);
                     return 2;
                   })
                   .thenInWorkerThread([&executed2](int value) {
                     CHECK(value == 2);
                     CHECK(!executed2);
                     executed2 = true;
                     return 11;
                   });

    std::vector<Future<int>> futures;
    futures.emplace_back(std::move(one));
    futures.emplace_back(std::move(two));
    auto joined = asyncSystem.all(std::move(futures));

    promise.resolve(1);

    std::vector<int> result = joined.wait();
    CHECK(executed1);
    CHECK(executed2);
    CHECK(result.size() == 2);
    CHECK(result[0] == 10);
    CHECK(result[1] == 11);
  }

  SECTION("can join two shared futures") {
    auto promise = asyncSystem.createPromise<int>();
    auto sharedFuture = promise.getFuture().share();

    bool executed1 = false;
    auto one = sharedFuture
                   .thenInWorkerThread([&executed1](int value) {
                     CHECK(value == 1);
                     CHECK(!executed1);
                     return 2;
                   })
                   .thenInWorkerThread([&executed1](int value) {
                     CHECK(value == 2);
                     CHECK(!executed1);
                     executed1 = true;
                     return 10;
                   });

    bool executed2 = false;
    auto two = sharedFuture
                   .thenInWorkerThread([&executed2](int value) {
                     CHECK(value == 1);
                     CHECK(!executed2);
                     return 2;
                   })
                   .thenInWorkerThread([&executed2](int value) {
                     CHECK(value == 2);
                     CHECK(!executed2);
                     executed2 = true;
                     return 11;
                   });

    std::vector<SharedFuture<int>> futures;
    futures.emplace_back(std::move(one).share());
    futures.emplace_back(std::move(two).share());
    auto joined = asyncSystem.all(std::move(futures));

    promise.resolve(1);

    std::vector<int> result = joined.wait();
    CHECK(executed1);
    CHECK(executed2);
    CHECK(result.size() == 2);
    CHECK(result[0] == 10);
    CHECK(result[1] == 11);
  }

  SECTION("can catch from shared future") {
    auto promise = asyncSystem.createPromise<int>();
    auto sharedFuture = promise.getFuture().share();

    bool executed1 = false;
    auto one = sharedFuture.catchImmediately([&executed1](std::exception&& e) {
      executed1 = true;
      CHECK(std::string(e.what()) == "reject!!");
      return 2;
    });

    promise.reject(std::runtime_error("reject!!"));

    int value1 = one.wait();

    CHECK(executed1);
    CHECK(value1 == 2);
  }

  SECTION("Future reports when it is ready") {
    auto promise = asyncSystem.createPromise<int>();
    auto future = promise.getFuture();

    CHECK(!future.isReady());
    promise.resolve(4);
    CHECK(future.isReady());
  }

  SECTION("SharedFuture reports when it is ready") {
    auto promise = asyncSystem.createPromise<int>();
    auto future = promise.getFuture().share();

    CHECK(!future.isReady());
    promise.resolve(4);
    CHECK(future.isReady());
  }

  SECTION("SharedFuture may resolve to void") {
    auto promise = asyncSystem.createPromise<void>();
    auto future = promise.getFuture().share();

    CHECK(!future.isReady());
    promise.resolve();
    CHECK(future.isReady());
    future.wait();
  }
}
