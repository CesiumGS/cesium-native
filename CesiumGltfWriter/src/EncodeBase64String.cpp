#include "EncodeBase64String.h"
#include <modp_b64.h>

std::string
CesiumGltf::encodeAsBase64String(const std::vector<std::byte>& data) noexcept {
  const std::size_t paddedLength = modp_b64_encode_len(data.size());
  std::string result(paddedLength, '\0');
  const char* asCharPointer = reinterpret_cast<const char*>(data.data());
  const std::size_t actualLength =
      modp_b64_encode(result.data(), asCharPointer, data.size());
  result.resize(actualLength);
  return result;
}
