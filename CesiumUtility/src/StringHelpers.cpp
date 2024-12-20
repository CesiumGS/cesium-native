#include <CesiumUtility/StringHelpers.h>

#include <string>

namespace CesiumUtility {

/*static*/ std::string StringHelpers::toStringUtf8(const std::u8string& s) {
  return std::string(reinterpret_cast<const char*>(s.data()), s.size());
}

} // namespace CesiumUtility
