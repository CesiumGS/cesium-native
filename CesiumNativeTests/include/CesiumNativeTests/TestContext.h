#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>

#include <map>

namespace CesiumNativeTests {
/**
 * @brief A context wrapper for asynchronous elements commonly used across unit
 * tests.
 */
template <typename TTaskProcessor> struct TestContext {
  using RequestMap = std::map<std::string, std::shared_ptr<SimpleAssetRequest>>;

  TestContext(RequestMap&& requests = {})
      : pAssetAccessor(
            std::make_shared<SimpleAssetAccessor>(std::move(requests))),
        asyncSystem(std::make_shared<TTaskProcessor>()) {}

  std::shared_ptr<SimpleAssetAccessor> pAssetAccessor;
  CesiumAsync::AsyncSystem asyncSystem;
};
} // namespace CesiumNativeTests
