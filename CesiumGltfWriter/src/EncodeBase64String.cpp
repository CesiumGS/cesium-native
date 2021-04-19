#include "EncodeBase64String.h"
#include <modp_b64.h>

std::string
CesiumGltf::encodeAsBase64String(const std::vector<std::byte>& data) noexcept {
  const std::size_t len = modp_b64_encode_len(data.size());
  std::vector<char> destination(len, 0);
  const char* asCharPointer = reinterpret_cast<const char*>(data.data());
  modp_b64_encode(destination.data(), asCharPointer, data.size());

  // TODO: Returning std:string(destination.begin(), destination.end())
  //       adds *ALL* the padding bytes instead of just stopping at the first
  //       null byte. It seems more correct to do that, however, if I do that
  //       buffer.cesium.data is initalized to nothing when CesiumGltfReader
  //       reads the glTF asset back in. Bug?
  return std::string(destination.data());
}
