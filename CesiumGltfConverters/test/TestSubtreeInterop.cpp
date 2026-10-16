#include <Cesium3DTiles/SubtreeInterop.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeometry/QuadtreeTileID.h>
#include <CesiumGltf/Extension3dTilesSubtree.h>
#include <CesiumGltf/Model.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>
#include <CesiumNativeTests/SimpleTaskProcessor.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>
#include <CesiumNativeTests/waitForFuture.h>
#include <CesiumTilesetJson/Subtree.h>
#include <CesiumUtility/JsonValue.h>

#include <doctest/doctest.h>
#include <libmorton/morton.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <spdlog/spdlog.h>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

using namespace CesiumTilesetJson;
using namespace Cesium3DTiles;
using namespace CesiumGeometry;
using namespace CesiumNativeTests;

TEST_CASE("Test SubtreeInterop") {
  SUBCASE("convertBuffer") {
    std::vector<std::byte> data{std::byte(255), std::byte(0)};

    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::Buffer buffer;
      buffer.name = "test";
      buffer.cesium.data = data;
      buffer.byteLength = int64_t(data.size());

      CesiumGltf::Buffer result =
          SubtreeInterop::convertBuffer(std::move(buffer));
      CHECK_EQ(result.name, "test");
      CHECK_EQ(result.cesium.data, data);
      CHECK_EQ(result.byteLength, int64_t(data.size()));
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::Buffer buffer;
      buffer.name = "test";
      buffer.cesium.data = data;
      buffer.byteLength = int64_t(buffer.cesium.data.size());

      CesiumUtility::Result<CesiumTilesetJson::Buffer> result =
          SubtreeInterop::convertBuffer(std::move(buffer));
      REQUIRE_UNARY(result.value);
      CHECK_EQ(result.value->name, "test");
      CHECK_EQ(result.value->cesium.data, data);
      CHECK_EQ(result.value->byteLength, int64_t(data.size()));
    }

    SUBCASE("does not error on glTF chunk when a uri is present") {
      CesiumGltf::Buffer buffer;
      buffer.chunk = 1;
      buffer.uri = "external.bin";
      buffer.byteLength = 10;

      CesiumUtility::Result<CesiumTilesetJson::Buffer> result =
          SubtreeInterop::convertBuffer(std::move(buffer));
      REQUIRE_UNARY(result.value);
      CHECK_EQ(result.value->uri, "external.bin");
    }

    SUBCASE("does not error on glTF chunk when data is present") {
      CesiumGltf::Buffer buffer;
      buffer.chunk = 1;
      buffer.cesium.data = {std::byte(255)};
      buffer.byteLength = 1;

      CesiumUtility::Result<CesiumTilesetJson::Buffer> result =
          SubtreeInterop::convertBuffer(std::move(buffer));
      REQUIRE_UNARY(result.value);
    }

    SUBCASE("errors on unresolved glTF chunk") {
      CesiumGltf::Buffer buffer;
      buffer.chunk = 1;

      CesiumUtility::Result<CesiumTilesetJson::Buffer> result =
          SubtreeInterop::convertBuffer(std::move(buffer));
      REQUIRE_UNARY_FALSE(result.value);
    }
  }

  SUBCASE("convertBufferView") {
    SUBCASE("from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::BufferView bufferView;
      bufferView.name = "test";
      bufferView.buffer = 0;
      bufferView.byteLength = 24;
      bufferView.byteOffset = 8;

      CesiumUtility::Result<CesiumGltf::BufferView> result =
          SubtreeInterop::convertBufferView(std::move(bufferView));
      REQUIRE_UNARY(result.value);
      CHECK_EQ(result.value->name, "test");
      CHECK_EQ(result.value->buffer, 0);
      CHECK_EQ(result.value->byteLength, 24);
      CHECK_EQ(result.value->byteOffset, 8);
    }

    SUBCASE("errors on large buffer index in 3D Tiles 1.1") {
      CesiumTilesetJson::BufferView bufferView;
      bufferView.buffer = int64_t(std::numeric_limits<int32_t>::max()) + 1;
      bufferView.byteLength = 24;

      CesiumUtility::Result<CesiumGltf::BufferView> result =
          SubtreeInterop::convertBufferView(std::move(bufferView));
      REQUIRE_UNARY_FALSE(result.value);
    }

    SUBCASE("from glTF to 3D Tiles 1.1") {
      CesiumGltf::BufferView bufferView;
      bufferView.name = "test";
      bufferView.buffer = 0;
      bufferView.byteLength = 24;
      bufferView.byteOffset = 8;

      CesiumUtility::Result<CesiumTilesetJson::BufferView> result =
          SubtreeInterop::convertBufferView(std::move(bufferView));
      REQUIRE_UNARY(result.value);
      CHECK_EQ(result.value->name, "test");
      CHECK_EQ(result.value->buffer, 0);
      CHECK_EQ(result.value->byteLength, 24);
      CHECK_EQ(result.value->byteOffset, 8);
    }

    SUBCASE("warns and preserves byteStride from glTF to 3D Tiles 1.1") {
      CesiumGltf::BufferView bufferView;
      bufferView.buffer = 0;
      bufferView.byteLength = 24;
      bufferView.byteStride = 4;

      CesiumUtility::Result<CesiumTilesetJson::BufferView> result =
          SubtreeInterop::convertBufferView(std::move(bufferView));
      REQUIRE_UNARY(result.value);
      CHECK_FALSE(result.errors.warnings.empty());

      auto it = result.value->unknownProperties.find("byteStride");
      REQUIRE(it != result.value->unknownProperties.end());
      CHECK_EQ(it->second.getSafeNumberOrDefault<int64_t>(-1), 4);
    }
  }

  SUBCASE("convertAvailability") {
    SUBCASE("bitstream from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::Availability availability;
      availability.bitstream = 0;
      availability.availableCount = 10;

      CesiumGltf::Availability result =
          SubtreeInterop::convertAvailability(std::move(availability));
      CHECK_EQ(result.bitstream, 0);
      CHECK_EQ(result.availableCount, 10);
      CHECK_UNARY_FALSE(result.constant);
    }

    SUBCASE("legacy bufferView property from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::Availability availability;
      availability.unknownProperties.emplace(
          "bufferView",
          CesiumUtility::JsonValue(static_cast<int64_t>(2)));

      CesiumGltf::Availability result =
          SubtreeInterop::convertAvailability(std::move(availability));
      CHECK_EQ(result.bitstream, 2);
      CHECK_UNARY_FALSE(result.constant);
    }

    SUBCASE("constant from 3D Tiles 1.1 to glTF") {
      CesiumTilesetJson::Availability availability;
      availability.constant =
          CesiumTilesetJson::Availability::Constant::AVAILABLE;

      CesiumGltf::Availability result =
          SubtreeInterop::convertAvailability(std::move(availability));
      CHECK_EQ(result.constant, CesiumGltf::Availability::Constant::AVAILABLE);
      CHECK_LT(result.bitstream, 0);
      CHECK_UNARY_FALSE(result.availableCount);
    }

    SUBCASE("bitstream from glTF to 3D Tiles 1.1") {
      CesiumGltf::Availability availability;
      availability.bitstream = 0;
      availability.availableCount = 10;

      CesiumTilesetJson::Availability result =
          SubtreeInterop::convertAvailability(std::move(availability));
      CHECK_EQ(result.bitstream, 0);
      CHECK_EQ(result.availableCount, 10);
      CHECK_UNARY_FALSE(result.constant);
    }

    SUBCASE("constant from glTF to 3D Tiles 1.1") {
      CesiumGltf::Availability availability;
      availability.constant = CesiumGltf::Availability::Constant::AVAILABLE;

      CesiumTilesetJson::Availability result =
          SubtreeInterop::convertAvailability(std::move(availability));
      CHECK_EQ(
          result.constant,
          CesiumTilesetJson::Availability::Constant::AVAILABLE);
      CHECK_UNARY_FALSE(result.bitstream);
      CHECK_UNARY_FALSE(result.availableCount);
    }
  }

  SUBCASE("convertToGltfModel") {
    SUBCASE("for no contentAvailability") {
      CesiumTilesetJson::Subtree subtree;
      subtree.tileAvailability.constant =
          CesiumTilesetJson::Availability::Constant::AVAILABLE;
      subtree.childSubtreeAvailability.constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

      CesiumUtility::Result<CesiumGltf::Model> result =
          SubtreeInterop::convertToGltfModel(std::move(subtree));
      REQUIRE_UNARY(result.value);

      auto* pSubtreeExtension =
          result.value->getExtension<CesiumGltf::Extension3dTilesSubtree>();
      REQUIRE(pSubtreeExtension);
      CHECK_EQ(
          pSubtreeExtension->tileAvailability.constant,
          CesiumGltf::Availability::Constant::AVAILABLE);
      CHECK_EQ(
          pSubtreeExtension->childSubtreeAvailability.constant,
          CesiumGltf::Availability::Constant::UNAVAILABLE);
      CHECK_UNARY_FALSE(pSubtreeExtension->contentAvailability);
    }

    SUBCASE("for single contentAvailability") {
      CesiumTilesetJson::Subtree subtree;
      subtree.tileAvailability.constant =
          CesiumTilesetJson::Availability::Constant::AVAILABLE;
      subtree.contentAvailability.emplace_back().constant =
          CesiumTilesetJson::Availability::Constant::AVAILABLE;
      subtree.childSubtreeAvailability.bitstream = 0;

      std::vector<std::byte> childBitstream{std::byte(255), std::byte(0)};
      {
        CesiumTilesetJson::Buffer& buffer = subtree.buffers.emplace_back();
        buffer.cesium.data = childBitstream;
        buffer.byteLength = int64_t(childBitstream.size());

        CesiumTilesetJson::BufferView& bufferView =
            subtree.bufferViews.emplace_back();
        bufferView.buffer = 0;
        bufferView.byteLength = buffer.byteLength;
      }

      CesiumUtility::Result<CesiumGltf::Model> result =
          SubtreeInterop::convertToGltfModel(std::move(subtree));
      REQUIRE_UNARY(result.value);

      CesiumGltf::Model& model = *result.value;

      auto* pSubtreeExtension =
          model.getExtension<CesiumGltf::Extension3dTilesSubtree>();
      REQUIRE(pSubtreeExtension);

      REQUIRE_UNARY(pSubtreeExtension->contentAvailability);
      CHECK_EQ(
          pSubtreeExtension->tileAvailability.constant,
          CesiumGltf::Availability::Constant::AVAILABLE);
      CHECK_EQ(
          pSubtreeExtension->contentAvailability->constant,
          CesiumGltf::Availability::Constant::AVAILABLE);

      CHECK_EQ(pSubtreeExtension->childSubtreeAvailability.bitstream, 0);

      REQUIRE_EQ(model.bufferViews.size(), 1);
      CHECK_EQ(model.bufferViews[0].buffer, 0);
      CHECK_EQ(model.bufferViews[0].byteLength, int64_t(childBitstream.size()));

      REQUIRE_EQ(model.buffers.size(), 1);
      CHECK_EQ(model.buffers[0].cesium.data, childBitstream);
      CHECK_EQ(model.buffers[0].byteLength, int64_t(childBitstream.size()));
    }

    SUBCASE("for multiple contentAvailability") {
      CesiumTilesetJson::Subtree subtree;
      subtree.tileAvailability.constant =
          CesiumTilesetJson::Availability::Constant::AVAILABLE;
      subtree.childSubtreeAvailability.constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

      {
        CesiumTilesetJson::Buffer& buffer = subtree.buffers.emplace_back();
        buffer.cesium.data = {std::byte(0), std::byte(1)};
        buffer.byteLength = int64_t(buffer.cesium.data.size());

        CesiumTilesetJson::BufferView& bufferView =
            subtree.bufferViews.emplace_back();
        bufferView.buffer = 0;
        bufferView.byteLength = buffer.byteLength;

        CesiumTilesetJson::Availability& availability =
            subtree.contentAvailability.emplace_back();
        availability.bitstream = 0;
        availability.availableCount = 1;
      }

      {
        CesiumTilesetJson::Buffer& buffer = subtree.buffers.emplace_back();
        buffer.cesium.data = {std::byte(1), std::byte(3)};
        buffer.byteLength = int64_t(buffer.cesium.data.size());

        CesiumTilesetJson::BufferView& bufferView =
            subtree.bufferViews.emplace_back();
        bufferView.buffer = 1;
        bufferView.byteLength = buffer.byteLength;

        CesiumTilesetJson::Availability& availability =
            subtree.contentAvailability.emplace_back();
        availability.bitstream = 1;
        availability.availableCount = 3;
      }

      std::vector<std::byte> expected{std::byte(1), std::byte(3)};

      CesiumUtility::Result<CesiumGltf::Model> result =
          SubtreeInterop::convertToGltfModel(std::move(subtree));
      REQUIRE_UNARY(result.value);

      CesiumGltf::Model& model = *result.value;

      auto* pSubtreeExtension =
          model.getExtension<CesiumGltf::Extension3dTilesSubtree>();
      REQUIRE(pSubtreeExtension);

      CHECK_EQ(
          pSubtreeExtension->tileAvailability.constant,
          CesiumGltf::Availability::Constant::AVAILABLE);
      CHECK_EQ(
          pSubtreeExtension->childSubtreeAvailability.constant,
          CesiumGltf::Availability::Constant::UNAVAILABLE);

      REQUIRE_UNARY(pSubtreeExtension->contentAvailability);
      CHECK_EQ(pSubtreeExtension->contentAvailability->bitstream, 2);
      CHECK_EQ(pSubtreeExtension->contentAvailability->availableCount, 3);

      REQUIRE_EQ(model.bufferViews.size(), 3);
      CHECK_EQ(model.bufferViews[2].buffer, 2);
      CHECK_EQ(model.bufferViews[2].byteLength, int64_t(expected.size()));

      REQUIRE_EQ(model.buffers.size(), 3);
      CHECK_EQ(model.buffers[2].cesium.data, expected);
      CHECK_EQ(model.buffers[2].byteLength, int64_t(expected.size()));
    }

    SUBCASE("merges to constant AVAILABLE when any contentAvailability is "
            "constant AVAILABLE") {
      CesiumTilesetJson::Subtree subtree;
      subtree.tileAvailability.constant =
          CesiumTilesetJson::Availability::Constant::AVAILABLE;
      subtree.childSubtreeAvailability.constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

      {
        CesiumTilesetJson::Buffer& buffer = subtree.buffers.emplace_back();
        buffer.cesium.data = {std::byte(0), std::byte(1)};
        buffer.byteLength = int64_t(buffer.cesium.data.size());

        CesiumTilesetJson::BufferView& bufferView =
            subtree.bufferViews.emplace_back();
        bufferView.buffer = 0;
        bufferView.byteLength = buffer.byteLength;

        CesiumTilesetJson::Availability& availability =
            subtree.contentAvailability.emplace_back();
        availability.bitstream = 0;
        availability.availableCount = 1;
      }

      subtree.contentAvailability.emplace_back().constant =
          CesiumTilesetJson::Availability::Constant::AVAILABLE;

      CesiumUtility::Result<CesiumGltf::Model> result =
          SubtreeInterop::convertToGltfModel(std::move(subtree));
      REQUIRE_UNARY(result.value);

      auto* pSubtreeExtension =
          result.value->getExtension<CesiumGltf::Extension3dTilesSubtree>();
      REQUIRE(pSubtreeExtension);
      REQUIRE_UNARY(pSubtreeExtension->contentAvailability);
      CHECK_EQ(
          pSubtreeExtension->contentAvailability->constant,
          CesiumGltf::Availability::Constant::AVAILABLE);
    }

    SUBCASE("merges to constant UNAVAILABLE when all contentAvailability "
            "entries are unavailable") {
      CesiumTilesetJson::Subtree subtree;
      subtree.tileAvailability.constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;
      subtree.childSubtreeAvailability.constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

      subtree.contentAvailability.emplace_back().constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;
      subtree.contentAvailability.emplace_back().constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

      CesiumUtility::Result<CesiumGltf::Model> result =
          SubtreeInterop::convertToGltfModel(std::move(subtree));
      REQUIRE_UNARY(result.value);

      auto* pSubtreeExtension =
          result.value->getExtension<CesiumGltf::Extension3dTilesSubtree>();
      REQUIRE(pSubtreeExtension);
      REQUIRE_UNARY(pSubtreeExtension->contentAvailability);
      CHECK_EQ(
          pSubtreeExtension->contentAvailability->constant,
          CesiumGltf::Availability::Constant::UNAVAILABLE);
      CHECK_LT(pSubtreeExtension->contentAvailability->bitstream, 0);
    }

    SUBCASE("errors on invalid bitstream index in contentAvailability") {
      CesiumTilesetJson::Subtree subtree;
      subtree.tileAvailability.constant =
          CesiumTilesetJson::Availability::Constant::AVAILABLE;
      subtree.childSubtreeAvailability.constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

      // No buffer views exist, so this bitstream index is invalid.
      subtree.contentAvailability.emplace_back().bitstream = 5;
      subtree.contentAvailability.emplace_back().constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

      CesiumUtility::Result<CesiumGltf::Model> result =
          SubtreeInterop::convertToGltfModel(std::move(subtree));
      CHECK_UNARY_FALSE(result.value);
      CHECK_UNARY_FALSE(result.errors.errors.empty());
    }

    SUBCASE("errors when contentAvailability bitstream buffer has unresolved "
            "data") {
      CesiumTilesetJson::Subtree subtree;
      subtree.tileAvailability.constant =
          CesiumTilesetJson::Availability::Constant::AVAILABLE;
      subtree.childSubtreeAvailability.constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

      CesiumTilesetJson::Buffer& buffer = subtree.buffers.emplace_back();
      buffer.byteLength = 2;
      // buffer.cesium.data is intentionally left empty to simulate an
      // unresolved external buffer.

      CesiumTilesetJson::BufferView& bufferView =
          subtree.bufferViews.emplace_back();
      bufferView.buffer = 0;
      bufferView.byteLength = buffer.byteLength;

      subtree.contentAvailability.emplace_back().bitstream = 0;
      subtree.contentAvailability.emplace_back().constant =
          CesiumTilesetJson::Availability::Constant::UNAVAILABLE;

      CesiumUtility::Result<CesiumGltf::Model> result =
          SubtreeInterop::convertToGltfModel(std::move(subtree));
      CHECK_UNARY_FALSE(result.value);
      CHECK_UNARY_FALSE(result.errors.errors.empty());
    }
  }

  SUBCASE("convertToTilesetJsonSubtree") {
    SUBCASE("errors when glTF model is missing 3DTILES_subtree extension") {
      CesiumGltf::Model model;

      CesiumUtility::Result<CesiumTilesetJson::Subtree> result =
          SubtreeInterop::convertToTilesetJsonSubtree(std::move(model));
      CHECK_UNARY_FALSE(result.value);
      CHECK(result.errors.hasErrors());
    }

    SUBCASE("with no contentAvailability") {
      CesiumGltf::Model model;
      auto& subtreeExtension =
          model.addExtension<CesiumGltf::Extension3dTilesSubtree>();
      subtreeExtension.tileAvailability.constant =
          CesiumGltf::Availability::Constant::AVAILABLE;
      subtreeExtension.childSubtreeAvailability.constant =
          CesiumGltf::Availability::Constant::UNAVAILABLE;

      CesiumUtility::Result<CesiumTilesetJson::Subtree> result =
          SubtreeInterop::convertToTilesetJsonSubtree(std::move(model));
      REQUIRE_UNARY(result.value);

      CesiumTilesetJson::Subtree& subtree = *result.value;
      CHECK_EQ(
          subtree.tileAvailability.constant,
          CesiumGltf::Availability::Constant::AVAILABLE);
      CHECK_EQ(
          subtree.childSubtreeAvailability.constant,
          CesiumGltf::Availability::Constant::UNAVAILABLE);
      CHECK_UNARY(subtree.contentAvailability.empty());
    }

    SUBCASE("with contentAvailability") {
      CesiumGltf::Model model;
      auto& subtreeExtension =
          model.addExtension<CesiumGltf::Extension3dTilesSubtree>();
      subtreeExtension.tileAvailability.constant =
          CesiumGltf::Availability::Constant::AVAILABLE;
      subtreeExtension.contentAvailability.emplace().constant =
          CesiumGltf::Availability::Constant::AVAILABLE;
      subtreeExtension.childSubtreeAvailability.bitstream = 0;

      std::vector<std::byte> childBitstream{std::byte(255), std::byte(0)};
      {
        CesiumGltf::Buffer& buffer = model.buffers.emplace_back();
        buffer.cesium.data = childBitstream;
        buffer.byteLength = int64_t(childBitstream.size());

        CesiumGltf::BufferView& bufferView = model.bufferViews.emplace_back();
        bufferView.buffer = 0;
        bufferView.byteLength = buffer.byteLength;
      }

      CesiumUtility::Result<CesiumTilesetJson::Subtree> result =
          SubtreeInterop::convertToTilesetJsonSubtree(std::move(model));
      REQUIRE_UNARY(result.value);

      CesiumTilesetJson::Subtree& subtree = *result.value;
      CHECK_EQ(
          subtree.tileAvailability.constant,
          CesiumGltf::Availability::Constant::AVAILABLE);
      REQUIRE_EQ(subtree.contentAvailability.size(), 1);
      CHECK_EQ(
          subtree.contentAvailability[0].constant,
          CesiumTilesetJson::Availability::Constant::AVAILABLE);

      CHECK_EQ(subtree.childSubtreeAvailability.bitstream, 0);

      REQUIRE_EQ(subtree.bufferViews.size(), 1);
      CHECK_EQ(subtree.bufferViews[0].buffer, 0);
      CHECK_EQ(
          subtree.bufferViews[0].byteLength,
          int64_t(childBitstream.size()));

      REQUIRE_EQ(subtree.buffers.size(), 1);
      CHECK_EQ(subtree.buffers[0].cesium.data, childBitstream);
      CHECK_EQ(subtree.buffers[0].byteLength, int64_t(childBitstream.size()));
    }
  }
}
