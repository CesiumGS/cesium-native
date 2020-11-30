#include "catch2/catch.hpp"
#include "Cesium3DTiles/AsyncSystem.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/ITaskProcessor.h"
#include <future>

using namespace Cesium3DTiles;

TEST_CASE("AsyncSystem") {
    class TestAssetAccessor : public IAssetAccessor {
    public:
        virtual ~TestAssetAccessor() = default;

        virtual std::unique_ptr<IAssetRequest> requestAsset(
            const std::string& /*url*/,
            const std::vector<THeader>& /*headers*/
        ) {
            return nullptr;
        }

        virtual void tick() noexcept {

        }
    };

    class TestTaskProcessor : public ITaskProcessor {
    public:
        virtual ~TestTaskProcessor() = default;

        virtual void startTask(std::function<void()> f) {
            auto x = std::async(f);
        }

    };

    Cesium3DTiles::AsyncSystem system(
        std::make_shared<TestAssetAccessor>(),
        std::make_shared<TestTaskProcessor>()
    );

    system.runMainThreadTasks();
}
