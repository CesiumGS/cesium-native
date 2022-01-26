#pragma once

#include <map>
#include <string>
#include <vector>

namespace CesiumIonClient {

struct Link {
  std::string url;
  std::string rel;
  std::map<std::string, std::string> otherParameters;
};

std::vector<Link> parseLinkHeader(const std::string& linkHeader);

} // namespace CesiumIonClient
