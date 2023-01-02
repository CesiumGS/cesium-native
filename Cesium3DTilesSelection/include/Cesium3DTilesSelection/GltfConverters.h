#pragma once

#include "Library.h"

#include <Cesium3DTilesSelection/GltfConverterResult.h>
#include <CesiumGltfReader/GltfReader.h>

#include <gsl/span>

#include <optional>
#include <string>
#include <string_view>

namespace Cesium3DTilesSelection {
/**
 * @brief Creates {@link GltfConverterResult} objects from a
 * a binary content.
 *
 * The class offers a lookup functionality for registering
 * {@link ConverterFunction} instances that can create
 * {@link GltfConverterResult} instances from a binary content.
 *
 * The loaders are registered based on the magic header or the file extension
 * of the input data. The binary data is usually received as a response to a
 * network request, and the first four bytes of the raw data form the magic
 * header. Based on this header or the file extension of the network response,
 * the loader that will be used for processing the input can be looked up.
 */
class CESIUM3DTILESSELECTION_API GltfConverters {
public:
  /**
   * @brief A function pointer that can create a {@link GltfConverterResult} from a
   * tile binary content.
   */
  using ConverterFunction = GltfConverterResult (*)(
      const gsl::span<const std::byte>& content,
      const CesiumGltfReader::GltfReaderOptions& options);

  /**
   * @brief Register the given function for the given magic header.
   *
   * The given magic header is a 4-character string. It will be compared
   * to the first 4 bytes of the raw input data, to decide whether the
   * given factory function should be used to create the
   * {@link GltfConverterResult} from the input data.
   *
   * @param magic The string describing the magic header.
   * @param converter The converter that will be used to create the tile gltf
   * content.
   */
  static void
  registerMagic(const std::string& magic, ConverterFunction converter);

  /**
   * @brief Register the given function for the given file extension.
   *
   * The given string is a file extension including the "." (e.g. ".ext"). It
   * is used for deciding whether the given factory function should be used to
   * create the
   * {@link GltfConverterResult} from the input data with the
   * same file extension in its url.
   *
   * @param fileExtension The file extension.
   * @param converter The converter that will be used to create the tile gltf
   * content
   */
  static void registerFileExtension(
      const std::string& fileExtension,
      ConverterFunction converter);

  /**
   * @brief Retrieve the converter function that is already registered for the
   * given file extension. If no such function is found, nullptr will be
   * returned
   *
   * @param filePath The file path that contains the file extension.
   * @return The {@link ConverterFunction} that is registered with the file extension.
   */
  static ConverterFunction
  getConverterByFileExtension(const std::string& filePath);

  /**
   * @brief Retrieve the converter function that is registered for the given
   * magic header. If no such function is found, nullptr will be returned
   *
   * The given magic header is a 4-character string. It will be compared
   * to the first 4 bytes of the raw input data, to decide whether the
   * given factory function should be used to create the
   * {@link GltfConverterResult} from the input data.
   *
   * @param content The binary tile content that contains the magic header.
   * @return The {@link ConverterFunction} that is registered with the magic header.
   */
  static ConverterFunction
  getConverterByMagic(const gsl::span<const std::byte>& content);

  /**
   * @brief Creates the {@link GltfConverterResult} from the given
   * binary content.
   *
   * This will look up the {@link ConverterFunction} that can be used to
   * process the given input data, based on all loaders that
   * have been registered with {@link GltfConverters::registerMagic}
   * or {@link GltfConverters::registerFileExtension}.
   *
   * It will first try to find a loader based on the magic header
   * of the `content` in the given input. If no matching loader is found, then
   * it will look up a loader based on the file extension of `filePath` of the
   * given input.
   *
   * If no such loader is found then an empty `GltfConverterResult` is returned.
   *
   * If a matching loader is found, it will be applied to the given
   * input, and the result will be returned.
   *
   * @param filePath The file path that contains the file extension to look up
   * the converter.
   * @param content The tile binary content that may contains the magic header
   * to look up the converter and is used to convert to gltf model.
   * @param options The {@link CesiumGltfReader::GltfReaderOptions} for how to read a glTF.
   * @return The {@link GltfConverterResult} that stores the gltf model converted from the binary data.
   */
  static GltfConverterResult convert(
      const std::string& filePath,
      const gsl::span<const std::byte>& content,
      const CesiumGltfReader::GltfReaderOptions& options);

  /**
   * @brief Creates the {@link GltfConverterResult} from the given
   * binary content.
   *
   * This will look up the {@link ConverterFunction} that can be used to
   * process the given input data, based on all loaders that
   * have been registered with {@link GltfConverters::registerMagic}.
   *
   * It will try to find a loader based on the magic header
   * of the `content` in the given input. If no such loader is found then an
   * empty `GltfConverterResult` is returned.
   *
   * If a matching loader is found, it will be applied to the given
   * input, and the result will be returned.
   *
   * @param content The tile binary content that may contains the magic header
   * to look up the converter and is used to convert to gltf model.
   * @param options The {@link CesiumGltfReader::GltfReaderOptions} for how to read a glTF.
   * @return The {@link GltfConverterResult} that stores the gltf model converted from the binary data.
   */
  static GltfConverterResult convert(
      const gsl::span<const std::byte>& content,
      const CesiumGltfReader::GltfReaderOptions& options);

private:
  static std::string toLowerCase(const std::string_view& str);

  static std::string getFileExtension(const std::string_view& filePath);

  static ConverterFunction getConverterByFileExtension(
      const std::string& filePath,
      std::string& fileExtension);

  static ConverterFunction getConverterByMagic(
      const gsl::span<const std::byte>& content,
      std::string& magic);

  static std::unordered_map<std::string, ConverterFunction> _loadersByMagic;
  static std::unordered_map<std::string, ConverterFunction>
      _loadersByFileExtension;
};
} // namespace Cesium3DTilesSelection
