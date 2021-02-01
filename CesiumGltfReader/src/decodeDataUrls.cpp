#include "CesiumGltf/Model.h"
#include "CesiumGltf/Reader.h"
#include "decodeDataUrls.h"
#include "modp_b64.h"

namespace {

    std::vector<uint8_t> decodeBase64(gsl::span<const uint8_t> data) {
        std::vector<uint8_t> result(modp_b64_decode_len(data.size()));

        size_t resultLength = modp_b64_decode(reinterpret_cast<char*>(result.data()), reinterpret_cast<const char*>(data.data()), data.size());
        if (resultLength == size_t(-1)) {
            result.clear();
            result.shrink_to_fit();
        } else {
            result.resize(resultLength);
        }

        return result;
    }

    struct DecodeResult {
        std::string mimeType;
        std::vector<uint8_t> data;
    };

    std::optional<DecodeResult> tryDecode(const std::string& uri) {
        constexpr std::string_view dataPrefix = "data:";
        constexpr size_t dataPrefixLength = dataPrefix.size();

        constexpr std::string_view base64Indicator = ";base64";
        constexpr size_t base64IndicatorLength = base64Indicator.size();

        if (uri.substr(0, dataPrefixLength) != dataPrefix) {
            return std::nullopt;
        }

        size_t dataDelimeter = uri.find(',', dataPrefixLength);
        if (dataDelimeter == std::string::npos) {
            return std::nullopt;
        }

        bool isBase64Encoded = false;

        DecodeResult result;

        result.mimeType = uri.substr(dataPrefixLength, dataDelimeter - dataPrefixLength);
        if (
            result.mimeType.size() >= base64IndicatorLength &&
            result.mimeType.substr(result.mimeType.size() - base64IndicatorLength, base64IndicatorLength) == base64Indicator
        ) {
            isBase64Encoded = true;
            result.mimeType = result.mimeType.substr(0, result.mimeType.size() - base64IndicatorLength);
        }

        gsl::span<const uint8_t> data(reinterpret_cast<const uint8_t*>(uri.data()) + dataDelimeter + 1, uri.size() - dataDelimeter - 1);

        if (isBase64Encoded) {
            result.data = decodeBase64(data);
            if (result.data.empty() && !data.empty()) {
                // base64 decode failed.
                return std::nullopt;
            }
        } else {
            result.data = std::vector<uint8_t>(data.begin(), data.end());
        }

        return result;
    }
}

namespace CesiumGltf {

void decodeDataUrls(ModelReaderResult& readModel, bool clearDecodedDataUrls) {
    if (!readModel.model) {
        return;
    }

    Model& model = readModel.model.value();

    for (Buffer& buffer : model.buffers) {
        if (!buffer.uri) {
            continue;
        }

        std::optional<DecodeResult> decoded = tryDecode(buffer.uri.value());
        if (!decoded) {
            continue;
        }

        buffer.cesium.data = std::move(decoded.value().data);

        if (clearDecodedDataUrls) {
            buffer.uri.reset();
        }
    }

    for (Image& image : model.images) {
        if (!image.uri) {
            continue;
        }

        std::optional<DecodeResult> decoded = tryDecode(image.uri.value());
        if (!decoded) {
            continue;
        }

        ImageReaderResult imageResult = readImage(decoded.value().data);
        if (imageResult.image) {
            image.cesium = std::move(imageResult.image.value());
        }

        if (clearDecodedDataUrls) {
            image.uri.reset();
        }
    }
}

}
