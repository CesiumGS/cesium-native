#include <CesiumAsync/SharedAsset.h>
#include <CesiumAsync/SharedAssetDepot.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>

#include <catch2/catch.hpp>

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

  SECTION("getOrCreate can create assets") {
    auto pDepot = createDepot();

    ResultPointer<TestAsset> assetOne =
        pDepot->getOrCreate(asyncSystem, nullptr, "one").waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);
  }

  SECTION("getOrCreate returns the same asset when called a second time with "
          "the same key") {
    auto pDepot = createDepot();

    auto futureOne = pDepot->getOrCreate(asyncSystem, nullptr, "one");
    auto futureTwo = pDepot->getOrCreate(asyncSystem, nullptr, "one");

    ResultPointer<TestAsset> assetOne = futureOne.waitInMainThread();
    ResultPointer<TestAsset> assetTwo = futureTwo.waitInMainThread();

    REQUIRE(assetOne.pValue != nullptr);
    CHECK(assetOne.pValue == assetTwo.pValue);
  }

  SECTION("unreferenced assets become inactive") {
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

  SECTION("re-referenced assets become active again") {
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

  SECTION("inactive assets are deleted when size threshold is exceeded") {
    auto pDepot = createDepot();

    pDepot->inactiveAssetSizeLimitBytes = std::string("one").size() + 1;

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
}
