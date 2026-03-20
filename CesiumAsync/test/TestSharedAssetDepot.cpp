#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/Result.h>
#include <CesiumUtility/SharedAsset.h>

#include <doctest/doctest.h>

#include <cstdint>
#include <memory>
#include <string>

using namespace CesiumAsync;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

namespace {

class TestAsset : public SharedAsset<TestAsset> {
public:
  TestAsset() = default;
  TestAsset(const std::string& value) : someValue(value) {}

  std::string someValue;

  int64_t getSizeBytes() const { return int64_t(this->someValue.size()); }
};

struct JustAsyncSystemContext {
  AsyncSystem asyncSystem;
};

std::optional<Future<ResultPointer<TestAsset>>> maybeFuture{};

IntrusivePointer<
    SharedAssetDepot<TestAsset, std::string, JustAsyncSystemContext>>
createDepot() {
  return new SharedAssetDepot<TestAsset, std::string, JustAsyncSystemContext>(
      [](const JustAsyncSystemContext& context, const std::string& assetKey) {
        if (maybeFuture) {
          Future<ResultPointer<TestAsset>> localFuture =
              std::move(*maybeFuture);
          maybeFuture.reset();
          return localFuture;
        } else {
          IntrusivePointer<TestAsset> p = new TestAsset();
          p->someValue = assetKey;
          return context.asyncSystem.createResolvedFuture(
              ResultPointer<TestAsset>(p));
        }
      });
}

} // namespace

TEST_CASE("SharedAssetDepot") {
  std::shared_ptr<SimpleTaskProcessor> pTaskProcessor =
      std::make_shared<SimpleTaskProcessor>();
  AsyncSystem asyncSystem(pTaskProcessor);
  JustAsyncSystemContext context{asyncSystem};

  SUBCASE("getOrCreate can create assets") {
    auto pDepot = createDepot();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(context, "one").waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);
  }

  SUBCASE("getOrCreate returns the same asset when called a second time with "
          "the same key") {
    auto pDepot = createDepot();

    auto futureOne = pDepot->getOrCreate(context, "one");
    auto futureTwo = pDepot->getOrCreate(context, "one");

    ResultPointer<TestAsset> assetOne = futureOne.waitInMainThread();
    ResultPointer<TestAsset> assetTwo = futureTwo.waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);
    CHECK(assetOne.pValue == assetTwo.pValue);
  }

  SUBCASE("getOrCreate returns a future for the same asset when called "
          "multiple times while loading") {
    auto pDepot = createDepot();

    Promise<ResultPointer<TestAsset>> promise =
        context.asyncSystem.createPromise<ResultPointer<TestAsset>>();
    maybeFuture = promise.getFuture();

    auto futureOne = pDepot->getOrCreate(context, "one");
    auto futureTwo = pDepot->getOrCreate(context, "one");

    promise.resolve(ResultPointer<TestAsset>(new TestAsset("one")));

    ResultPointer<TestAsset> assetOne = futureOne.waitInMainThread();
    ResultPointer<TestAsset> assetTwo = futureTwo.waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);
    REQUIRE(assetTwo.pValue != nullptr);
    CHECK(assetOne.pValue == assetTwo.pValue);
  }

  SUBCASE("loads that fail with an exception can be retried") {
    auto pDepot = createDepot();

    Promise<ResultPointer<TestAsset>> promise =
        context.asyncSystem.createPromise<ResultPointer<TestAsset>>();
    maybeFuture = promise.getFuture();

    auto futureOne = pDepot->getOrCreate(context, "one");

    // Reject the load with an exception.
    promise.reject(std::runtime_error("Simulated load failure"));

    ResultPointer<TestAsset> assetOne = futureOne.waitInMainThread();

    REQUIRE(assetOne.pValue == nullptr);
    CHECK(assetOne.errors.hasErrors());

    // Now try again, this time succeeding.
    promise = context.asyncSystem.createPromise<ResultPointer<TestAsset>>();
    maybeFuture = promise.getFuture();
    auto futureTwo = pDepot->getOrCreate(context, "one");

    promise.resolve(ResultPointer<TestAsset>(new TestAsset("one")));

    ResultPointer<TestAsset> assetTwo = futureTwo.waitInMainThread();

    REQUIRE(assetTwo.pValue != nullptr);
    CHECK(assetTwo.pValue->someValue == "one");
  }

  SUBCASE("loads that fail immediately with an exception can also be retried") {
    auto pDepot = createDepot();

    Promise<ResultPointer<TestAsset>> promise =
        context.asyncSystem.createPromise<ResultPointer<TestAsset>>();
    maybeFuture = promise.getFuture();

    // Reject the load with an exception before requesting the asset, so that
    // the load will fail immediately in the thread that calls getOrCreate.
    promise.reject(std::runtime_error("Simulated load failure"));

    auto futureOne = pDepot->getOrCreate(context, "one");

    ResultPointer<TestAsset> assetOne = futureOne.waitInMainThread();

    REQUIRE(assetOne.pValue == nullptr);
    CHECK(assetOne.errors.hasErrors());

    // Now try again, this time succeeding.
    promise = context.asyncSystem.createPromise<ResultPointer<TestAsset>>();
    maybeFuture = promise.getFuture();
    auto futureTwo = pDepot->getOrCreate(context, "one");

    promise.resolve(ResultPointer<TestAsset>(new TestAsset("one")));

    ResultPointer<TestAsset> assetTwo = futureTwo.waitInMainThread();

    REQUIRE(assetTwo.pValue != nullptr);
    CHECK(assetTwo.pValue->someValue == "one");
  }

  SUBCASE("loads with a non-exception failure cache the failure") {
    auto pDepot = createDepot();

    Promise<ResultPointer<TestAsset>> promise =
        context.asyncSystem.createPromise<ResultPointer<TestAsset>>();
    maybeFuture = promise.getFuture();

    auto futureOne = pDepot->getOrCreate(context, "one");

    // Resolve the load with a non-exception failure.
    promise.resolve(
        ResultPointer<TestAsset>(ErrorList::error("Simulated load failure")));

    ResultPointer<TestAsset> assetOne = futureOne.waitInMainThread();

    REQUIRE(assetOne.pValue == nullptr);
    CHECK(assetOne.errors.hasErrors());

    // Now try again; it should return the same failure without attempting
    // to load again.
    auto futureTwo = pDepot->getOrCreate(context, "one");

    ResultPointer<TestAsset> assetTwo = futureTwo.waitInMainThread();

    REQUIRE(assetTwo.pValue == nullptr);
    CHECK(assetTwo.errors.hasErrors());
  }

  SUBCASE("unreferenced assets become inactive") {
    auto pDepot = createDepot();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(context, "one").waitInMainThread();

    CHECK(pDepot->getAssetCount() == 1);
    CHECK(pDepot->getActiveAssetCount() == 1);
    CHECK(pDepot->getInactiveAssetCount() == 0);

    assetOne.pValue.reset();

    CHECK(pDepot->getAssetCount() == 1);
    CHECK(pDepot->getActiveAssetCount() == 0);
    CHECK(pDepot->getInactiveAssetCount() == 1);
  }

  SUBCASE("re-referenced assets become active again") {
    auto pDepot = createDepot();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(context, "one").waitInMainThread();

    CHECK(pDepot->getAssetCount() == 1);
    CHECK(pDepot->getActiveAssetCount() == 1);
    CHECK(pDepot->getInactiveAssetCount() == 0);

    assetOne.pValue.reset();

    CHECK(pDepot->getAssetCount() == 1);
    CHECK(pDepot->getActiveAssetCount() == 0);
    CHECK(pDepot->getInactiveAssetCount() == 1);

    ResultPointer<TestAsset> assetTwo =
        pDepot->getOrCreate(context, "one").waitInMainThread();

    CHECK(pDepot->getAssetCount() == 1);
    CHECK(pDepot->getActiveAssetCount() == 1);
    CHECK(pDepot->getInactiveAssetCount() == 0);
  }

  SUBCASE("inactive assets are deleted when size threshold is exceeded") {
    auto pDepot = createDepot();

    pDepot->inactiveAssetSizeLimitBytes =
        int64_t(std::string("one").size() + 1);

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(context, "one").waitInMainThread();
    ResultPointer<TestAsset> assetTwo =
        pDepot->getOrCreate(context, "two").waitInMainThread();

    assetOne.pValue.reset();

    CHECK(pDepot->getAssetCount() == 2);
    CHECK(pDepot->getActiveAssetCount() == 1);
    CHECK(pDepot->getInactiveAssetCount() == 1);

    assetTwo.pValue.reset();

    CHECK(pDepot->getAssetCount() == 1);
    CHECK(pDepot->getActiveAssetCount() == 0);
    CHECK(pDepot->getInactiveAssetCount() == 1);
  }

  SUBCASE("is kept alive until all of its assets are unreferenced") {
    auto pDepot = createDepot();
    SharedAssetDepot<TestAsset, std::string, JustAsyncSystemContext>*
        pDepotRaw = pDepot.get();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(context, "one").waitInMainThread();
    ResultPointer<TestAsset> assetTwo =
        pDepot->getOrCreate(context, "two!!").waitInMainThread();

    pDepot.reset();

    assetTwo.pValue.reset();

    REQUIRE(assetOne.pValue->getDepot() == pDepotRaw);
    CHECK(
        pDepotRaw->getInactiveAssetTotalSizeBytes() ==
        int64_t(std::string("two!!").size()));

    assetOne.pValue.reset();
  }

  SUBCASE("recreates invalidated asset") {
    auto pDepot = createDepot();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(context, "one").waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);

    pDepot->invalidate("one");

    ResultPointer<TestAsset> assetOne2 =
        pDepot->getOrCreate(context, "one").waitInMainThread();

    REQUIRE(assetOne.pValue != assetOne2.pValue);
    CHECK(assetOne.pValue->someValue == "one");
    CHECK(assetOne2.pValue->someValue == "one");
  }

  SUBCASE("is kept alive for as long as invalidated assets are alive") {
    auto pDepot = createDepot();
    SharedAssetDepot<TestAsset, std::string, JustAsyncSystemContext>*
        pDepotRaw = pDepot.get();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(context, "one").waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);

    pDepot->invalidate("one");

    pDepot.reset();

    REQUIRE(assetOne.pValue->getDepot() == pDepotRaw);
    CHECK(pDepotRaw->getInactiveAssetTotalSizeBytes() == 0);

    assetOne.pValue.reset();
  }

  SUBCASE("invalidated assets don't count against inactive asset size") {
    auto pDepot = createDepot();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(context, "one").waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);
    assetOne.pValue.reset();

    CHECK(pDepot->getInactiveAssetTotalSizeBytes() > 0);
    pDepot->invalidate("one");
    CHECK(pDepot->getInactiveAssetTotalSizeBytes() == 0);
  }

  SUBCASE("can invalidate an asset that was never valid") {
    auto pDepot = createDepot();
    pDepot->invalidate("one");
  }

  SUBCASE("can invalidate the same asset twice") {
    auto pDepot = createDepot();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(context, "one").waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);

    pDepot->invalidate("one");
    pDepot->invalidate("one");
  }
}
