#pragma once

#include <Cesium3DTilesReader/Library.h>
#include <Cesium3DTilesReader/SubtreeReader.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>

namespace Cesium3DTilesReader {

/**
 * @brief Reads 3D Tiles subtrees from a binary or JSON subtree file.
 *
 * While {@link SubtreeReader} can parse a {@link Subtree} from a binary buffer
 * as well, `SubtreeFileReader` additionally supports:
 *
 * 1. Loading binary subtree files.
 * 2. Loading external buffers asynchronously.
 * 3. Decoding buffers from data URIs.
 *
 * The subtree file need not be an actual file on disk.
 */
class CESIUM3DTILESREADER_API SubtreeFileReader {
public:
  /**
   * @brief Constructs a new instance.
   */
  SubtreeFileReader();

  /**
   * @brief Gets the options controlling how the JSON is read.
   */
  CesiumJsonReader::JsonReaderOptions& getOptions();

  /**
   * @brief Gets the options controlling how the JSON is read.
   */
  const CesiumJsonReader::JsonReaderOptions& getOptions() const;

  /**
   * @brief Asynchronously loads a subtree from a URL.
   *
   * Please note that the `SubtreeFileReader` instance must remain valid until
   * the returned future resolves or rejects. Destroying it earlier will result
   * in undefined behavior. One easy way to achieve this is to construct the
   * reader with `std::make_shared` and capture the `std::shared_ptr` in the
   * continuation lambda.
   *
   * @param asyncSystem The AsyncSystem used to do asynchronous work.
   * @param pAssetAccessor The accessor used to retrieve the URL and any other
   * required resources.
   * @param url The URL from which to get the subtree file.
   * @param headers Headers to include in the request for the initial subtree
   * file and any additional resources that are required.
   * @return A future that resolves to the result of loading the subtree.
   */
  CesiumAsync::Future<CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree>>
  load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& headers = {})
      const noexcept;

  CesiumAsync::Future<CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree>>
  load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::shared_ptr<CesiumAsync::IAssetRequest>& pRequest)
      const noexcept;

  CesiumAsync::Future<CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree>>
  load(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const gsl::span<const std::byte>& data) const noexcept;

private:
  CesiumAsync::Future<CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree>>
  loadBinary(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const gsl::span<const std::byte>& data) const noexcept;
  CesiumAsync::Future<CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree>>
  loadJson(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      const gsl::span<const std::byte>& data) const noexcept;
  CesiumAsync::Future<CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree>>
  postprocess(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<CesiumAsync::IAssetAccessor>& pAssetAccessor,
      const std::string& url,
      const std::vector<CesiumAsync::IAssetAccessor::THeader>& requestHeaders,
      CesiumJsonReader::ReadJsonResult<Cesium3DTiles::Subtree>&& loaded)
      const noexcept;

  SubtreeReader _reader;
};

} // namespace Cesium3DTilesReader
