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

IntrusivePointer<SharedAssetDepot<TestAsset, std::string>> createDepot() {
  return new SharedAssetDepot<TestAsset, std::string>(
      [](const AsyncSystem& asyncSystem,
         const std::shared_ptr<IAssetAccessor>& /* pAssetAccessor */,
         const std::string& assetKey) {
        IntrusivePointer<TestAsset> p = new TestAsset();
        p->someValue = assetKey;
        return asyncSystem.createResolvedFuture(ResultPointer<TestAsset>(p));
      });
}

} // namespace

TEST_CASE("SharedAssetDepot") {
  std::shared_ptr<SimpleTaskProcessor> pTaskProcessor =
      std::make_shared<SimpleTaskProcessor>();
  AsyncSystem asyncSystem(pTaskProcessor);

  SUBCASE("getOrCreate can create assets") {
    auto pDepot = createDepot();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(asyncSystem, nullptr, "one").waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);
  }

  SUBCASE("getOrCreate returns the same asset when called a second time with "
          "the same key") {
    auto pDepot = createDepot();

    auto futureOne = pDepot->getOrCreate(asyncSystem, nullptr, "one");
    auto futureTwo = pDepot->getOrCreate(asyncSystem, nullptr, "one");

    ResultPointer<TestAsset> assetOne = futureOne.waitInMainThread();
    ResultPointer<TestAsset> assetTwo = futureTwo.waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);
    CHECK(assetOne.pValue == assetTwo.pValue);
  }

  SUBCASE("unreferenced assets become inactive") {
    auto pDepot = createDepot();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(asyncSystem, nullptr, "one").waitInMainThread();

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
        pDepot->getOrCreate(asyncSystem, nullptr, "one").waitInMainThread();

    CHECK(pDepot->getAssetCount() == 1);
    CHECK(pDepot->getActiveAssetCount() == 1);
    CHECK(pDepot->getInactiveAssetCount() == 0);

    assetOne.pValue.reset();

    CHECK(pDepot->getAssetCount() == 1);
    CHECK(pDepot->getActiveAssetCount() == 0);
    CHECK(pDepot->getInactiveAssetCount() == 1);

    ResultPointer<TestAsset> assetTwo =
        pDepot->getOrCreate(asyncSystem, nullptr, "one").waitInMainThread();

    CHECK(pDepot->getAssetCount() == 1);
    CHECK(pDepot->getActiveAssetCount() == 1);
    CHECK(pDepot->getInactiveAssetCount() == 0);
  }

  SUBCASE("inactive assets are deleted when size threshold is exceeded") {
    auto pDepot = createDepot();

    pDepot->inactiveAssetSizeLimitBytes =
        int64_t(std::string("one").size() + 1);

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(asyncSystem, nullptr, "one").waitInMainThread();
    ResultPointer<TestAsset> assetTwo =
        pDepot->getOrCreate(asyncSystem, nullptr, "two").waitInMainThread();

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
    SharedAssetDepot<TestAsset, std::string>* pDepotRaw = pDepot.get();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(asyncSystem, nullptr, "one").waitInMainThread();
    ResultPointer<TestAsset> assetTwo =
        pDepot->getOrCreate(asyncSystem, nullptr, "two!!").waitInMainThread();

    pDepot.reset();

    assetTwo.pValue.reset();

    REQUIRE(assetOne.pValue->getDepot() == pDepotRaw);
    CHECK(
        pDepotRaw->getInactiveAssetTotalSizeBytes() ==
        int64_t(std::string("two!!").size()));

    assetOne.pValue.reset();
  }
}
