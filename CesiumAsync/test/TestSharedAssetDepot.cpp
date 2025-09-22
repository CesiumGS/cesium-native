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
  std::string someValue;

  int64_t getSizeBytes() const { return int64_t(this->someValue.size()); }
};

struct JustAsyncSystemContext {
  AsyncSystem asyncSystem;
};

IntrusivePointer<
    SharedAssetDepot<TestAsset, std::string, JustAsyncSystemContext>>
createDepot() {
  return new SharedAssetDepot<TestAsset, std::string, JustAsyncSystemContext>(
      [](const JustAsyncSystemContext& context, const std::string& assetKey) {
        IntrusivePointer<TestAsset> p = new TestAsset();
        p->someValue = assetKey;
        return context.asyncSystem.createResolvedFuture(
            ResultPointer<TestAsset>(p));
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
