#include <CesiumAsync/HttpHeaders.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace CesiumAsync {
struct NocaseCompare {
  bool operator()(const unsigned char& c1, const unsigned char& c2) const {
    return tolower(c1) < tolower(c2);
  }
};

bool CaseInsensitiveCompare::operator()(
    const std::string& s1,
    const std::string& s2) const {
  return std::lexicographical_compare(
      s1.begin(),
      s1.end(),
      s2.begin(),
      s2.end(),
      NocaseCompare());
}
} // namespace CesiumAsync
