#include <CesiumUtility/BabyNames.h>

#include <fstream>

namespace CesiumUtility {

namespace {
BabyNames babyNamesInstance{};
}

const BabyNames& BabyNames::instance() { return babyNamesInstance; }

BabyNames::BabyNames() : _names() {
  // Get this file from:
  // https://data.gov.au/dataset/ds-sa-9849aa7f-e316-426e-8ab5-74658a62c7e6/details
  // "Most popular Baby Names (1944-2013)(CSV)"
  // https://data.sa.gov.au/data/dataset/9849aa7f-e316-426e-8ab5-74658a62c7e6/resource/534d13f2-237c-4448-a6a3-93c07b1bb614/download/baby-names-1944-2013.zip
  std::ifstream file("C:\\Users\\kevin\\Downloads\\female_cy2011_top.csv");

  std::string line;
  while (std::getline(file, line)) {
    size_t commaPos = line.find(',');
    std::string firstColumn = line.substr(0, commaPos);
    if (firstColumn.empty())
      continue;

    if (*firstColumn.begin() == '\"')
      firstColumn.erase(firstColumn.begin());
    if (firstColumn.empty())
      continue;
    if (*firstColumn.rbegin() == '\"')
      firstColumn = firstColumn.substr(0, firstColumn.size() - 1);

    this->_names.emplace_back(firstColumn);
  }
}

} // namespace CesiumUtility
