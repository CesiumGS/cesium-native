#pragma once

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>

#include <memory>

namespace CesiumNativeTests {
class FileAccessor : public CesiumAsync::IAssetAccessor {
public:
  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
  get(const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& url,
      const std::vector<THeader>&) override;

  CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> request(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::string& verb,
      const std::string& url,
      const std::vector<THeader>& headers,
      const std::span<const std::byte>&) override;

  void tick() noexcept override {}
};
} // namespace CesiumNativeTests
