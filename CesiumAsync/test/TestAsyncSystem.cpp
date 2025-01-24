#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Promise.h>
#include <CesiumAsync/SharedFuture.h>
#include <CesiumAsync/ThreadPool.h>

#include <doctest/doctest.h>

#include <atomic>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

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

  SUBCASE("runs worker tasks with the task processor") {
    bool executed = false;

    asyncSystem.runInWorkerThread([&executed]() { executed = true; }).wait();

    CHECK(pTaskProcessor->tasksStarted == 1);
    CHECK(executed);
  }

  SUBCASE("worker continuations are run via the task processor") {
    bool executed = false;

    asyncSystem.createResolvedFuture()
        .thenInWorkerThread([&executed]() { executed = true; })
        .wait();

    CHECK(pTaskProcessor->tasksStarted == 1);
    CHECK(executed);
  }

  SUBCASE("runs main thread tasks when instructed") {
    bool executed = false;

    auto future =
        asyncSystem.runInMainThread([&executed]() { executed = true; });

    CHECK(!executed);
    bool taskDispatched = asyncSystem.dispatchOneMainThreadTask();
    CHECK(taskDispatched);
    CHECK(executed);
    CHECK(pTaskProcessor->tasksStarted == 0);
  }

  SUBCASE("main thread continuations are run when instructed") {
    bool executed = false;

    auto future = asyncSystem.createResolvedFuture().thenInMainThread(
        [&executed]() { executed = true; });

    CHECK(!executed);
    bool taskDispatched = asyncSystem.dispatchOneMainThreadTask();
    CHECK(taskDispatched);
    CHECK(executed);
    CHECK(pTaskProcessor->tasksStarted == 0);
  }

  SUBCASE("worker continuations following a worker run immediately") {
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

  SUBCASE("main thread continuations following a main thread task run "
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

  SUBCASE("worker continuations following a thread pool thread run as a "
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

  SUBCASE("a worker continuation that returns an already resolved future "
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

  SUBCASE("can pass move-only objects between continuations") {
    auto future =
        asyncSystem
            .runInWorkerThread([]() { return std::make_unique<int>(42); })
            .thenInWorkerThread(
                [](std::unique_ptr<int>&& pResult) { return *pResult; });
    CHECK(future.wait() == 42);
  }

  SUBCASE("an exception thrown in a continuation rejects the future") {
    auto future = asyncSystem.runInWorkerThread(
        []() { throw std::runtime_error("test"); });
    CHECK_THROWS_WITH(future.wait(), "test");
  }

  SUBCASE("an exception thrown in createFuture rejects the future") {
    auto future = asyncSystem.createFuture<int>(
        [](const auto& /*promise*/) { throw std::runtime_error("test"); });
    CHECK_THROWS_WITH(future.wait(), "test");
  }

  SUBCASE("createFuture promise may resolve immediately") {
    auto future = asyncSystem.createFuture<int>(
        [](const auto& promise) { promise.resolve(42); });
    CHECK(future.wait() == 42);
  }

  SUBCASE("createFuture promise may resolve later") {
    auto future = asyncSystem.createFuture<int>([](const auto& promise) {
      std::thread([promise]() {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(10ms);
        promise.resolve(42);
      }).detach();
    });
    CHECK(future.wait() == 42);
  }

  SUBCASE("rejected promise invokes catch instead of then") {
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

  SUBCASE("catch may chain to another futre") {
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

  SUBCASE("then after returning catch is invoked") {
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

  SUBCASE("then after throwing catch is not invoked") {
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

  SUBCASE("Future returned by all resolves when all given Futures resolve") {
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

  SUBCASE("Can use `all` with void-returning Futures") {
    auto one = asyncSystem.createPromise<void>();
    auto two = asyncSystem.createPromise<void>();
    auto three = asyncSystem.createPromise<void>();

    std::vector<Future<void>> futures;
    futures.emplace_back(one.getFuture());
    futures.emplace_back(two.getFuture());
    futures.emplace_back(three.getFuture());

    Future<void> all = asyncSystem.all(std::move(futures));

    bool resolved = false;
    Future<void> last =
        std::move(all).thenImmediately([&resolved]() { resolved = true; });

    three.resolve();
    one.resolve();
    two.resolve();

    last.wait();
    CHECK(resolved);
  }

  SUBCASE("Future returned by 'all' rejects when any Future rejects") {
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

  SUBCASE("When multiple futures in an 'all' reject, the data from the first "
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

  SUBCASE("conversion to SharedFuture") {
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

  SUBCASE("can join two chains originating with a shared future") {
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

  SUBCASE("can join two shared futures") {
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

  SUBCASE("can join two shared futures returning void") {
    auto promise = asyncSystem.createPromise<void>();
    auto sharedFuture = promise.getFuture().share();

    bool executed1 = false;
    Future<void> one =
        sharedFuture.thenInWorkerThread([&executed1]() { CHECK(!executed1); })
            .thenInWorkerThread([&executed1]() {
              CHECK(!executed1);
              executed1 = true;
            });

    bool executed2 = false;
    Future<void> two =
        sharedFuture.thenInWorkerThread([&executed2]() { CHECK(!executed2); })
            .thenInWorkerThread([&executed2]() {
              CHECK(!executed2);
              executed2 = true;
            });

    std::vector<SharedFuture<void>> futures;
    futures.emplace_back(std::move(one).share());
    futures.emplace_back(std::move(two).share());
    Future<void> joined = asyncSystem.all(std::move(futures));

    promise.resolve();

    joined.wait();
    CHECK(executed1);
    CHECK(executed2);
  }

  SUBCASE("can catch from shared future") {
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

  SUBCASE("Future reports when it is ready") {
    auto promise = asyncSystem.createPromise<int>();
    auto future = promise.getFuture();

    CHECK(!future.isReady());
    promise.resolve(4);
    CHECK(future.isReady());
  }

  SUBCASE("SharedFuture reports when it is ready") {
    auto promise = asyncSystem.createPromise<int>();
    auto future = promise.getFuture().share();

    CHECK(!future.isReady());
    promise.resolve(4);
    CHECK(future.isReady());
  }

  SUBCASE("SharedFuture may resolve to void") {
    auto promise = asyncSystem.createPromise<void>();
    auto future = promise.getFuture().share();

    CHECK(!future.isReady());
    promise.resolve();
    CHECK(future.isReady());
    future.wait();
  }

  SUBCASE("thenPassThrough") {
    bool checksCompleted = false;

    asyncSystem.createResolvedFuture(3.1)
        .thenPassThrough(std::string("foo"), 4)
        .thenImmediately([&](std::tuple<std::string, int, double>&& tuple) {
          auto& [s, i, d] = tuple;
          CHECK(s == "foo");
          CHECK(i == 4);
          CHECK(d == 3.1);
          checksCompleted = true;
        });

    CHECK(checksCompleted);
  }

  SUBCASE("thenPassThrough on a SharedFuture") {
    bool checksCompleted = false;

    asyncSystem.createResolvedFuture(3.1)
        .share()
        .thenPassThrough(std::string("foo"), 4)
        .thenImmediately([&](std::tuple<std::string, int, double>&& tuple) {
          auto& [s, i, d] = tuple;
          CHECK(s == "foo");
          CHECK(i == 4);
          CHECK(d == 3.1);
          checksCompleted = true;
        });

    CHECK(checksCompleted);
  }

  SUBCASE("waitInMainThread") {
    SUBCASE("Future returning a value") {
      bool called = false;
      Future<int> future =
          asyncSystem.createResolvedFuture().thenInMainThread([&called]() {
            called = true;
            return 4;
          });
      int value = std::move(future).waitInMainThread();
      CHECK(called);
      CHECK(value == 4);
    }

    SUBCASE("Future returning void") {
      bool called = false;
      Future<void> future = asyncSystem.createResolvedFuture().thenInMainThread(
          [&called]() { called = true; });
      std::move(future).waitInMainThread();
      CHECK(called);
    }

    SUBCASE("SharedFuture returning a value") {
      bool called = false;
      SharedFuture<int> future = asyncSystem.createResolvedFuture()
                                     .thenInMainThread([&called]() {
                                       called = true;
                                       return 4;
                                     })
                                     .share();
      int value = future.waitInMainThread();
      CHECK(called);
      CHECK(value == 4);
    }

    SUBCASE("SharedFuture returning void") {
      bool called = false;
      SharedFuture<void> future =
          asyncSystem.createResolvedFuture()
              .thenInMainThread([&called]() { called = true; })
              .share();
      future.waitInMainThread();
      CHECK(called);
    }

    SUBCASE("Future resolving while main thread is waiting") {
      bool called1 = false;
      bool called2 = false;
      Future<void> future =
          asyncSystem.createResolvedFuture()
              .thenInWorkerThread([&called1]() {
                using namespace std::chrono_literals;
                // should be long enough for the main thread to start waiting on
                // the conditional, without slowing the test down too much.
                std::this_thread::sleep_for(20ms);
                called1 = true;
              })
              .thenInMainThread([&called2]() { called2 = true; });
      future.waitInMainThread();
      CHECK(called1);
      CHECK(called2);
    }

    SUBCASE("Future resolving from a worker while main thread is waiting") {
      bool called1 = false;
      bool called2 = false;
      bool called3 = false;
      Future<void> future =
          asyncSystem.createResolvedFuture()
              .thenInWorkerThread([&called1]() {
                using namespace std::chrono_literals;
                // should be long enough for the main thread to start waiting on
                // the conditional, without slowing the test down too much.
                std::this_thread::sleep_for(20ms);
                called1 = true;
              })
              .thenInMainThread([&called2]() { called2 = true; })
              .thenInWorkerThread([&called3]() {
                using namespace std::chrono_literals;
                // Sufficient time for the main thread to drop back into waiting
                // on the conditional again after it was awakened by the
                // scheduling of the main thread continuation above. It should
                // awaken again when this continuation completes.
                std::this_thread::sleep_for(20ms);
                called3 = true;
              });
      future.waitInMainThread();
      CHECK(called1);
      CHECK(called2);
      CHECK(called3);
    }

    SUBCASE("Future rejecting with throw") {
      bool called = false;
      auto future =
          asyncSystem.runInWorkerThread([]() { throw std::runtime_error(""); })
              .thenInMainThread([&called]() {
                called = true;
                return 4;
              });
      CHECK_THROWS(std::move(future).waitInMainThread());
      CHECK(!called);
    }

    SUBCASE("Future rejecting with Promise::reject") {
      bool called = false;
      auto promise = asyncSystem.createPromise<void>();
      promise.reject(std::runtime_error("Some exception"));
      Future<int> future = promise.getFuture().thenInMainThread([&called]() {
        called = true;
        return 4;
      });
      CHECK_THROWS(std::move(future).waitInMainThread());
      CHECK(!called);
    }

    SUBCASE("SharedFuture rejecting") {
      bool called = false;
      auto promise = asyncSystem.createPromise<void>();
      promise.reject(std::runtime_error("Some exception"));
      SharedFuture<int> future = promise.getFuture()
                                     .thenInMainThread([&called]() {
                                       called = true;
                                       return 4;
                                     })
                                     .share();
      CHECK_THROWS(future.waitInMainThread());
      CHECK(!called);
    }

    SUBCASE(
        "catchImmediately can return a value from a mutable lambda capture") {
      auto promise = asyncSystem.createPromise<std::string>();
      promise.reject(std::runtime_error("Some exception"));
      std::string myValue = "value from catch";
      Future<std::string> future =
          promise.getFuture()
              .catchImmediately([myValue = std::move(myValue)](
                                    std::exception&& exception) mutable {
                CHECK(std::string(exception.what()) == "Some exception");
                return myValue;
              })
              .thenImmediately(
                  [](std::string&& result) { return std::move(result); });
      std::string result = future.waitInMainThread();
      CHECK(result == "value from catch");
    }
  }
}
