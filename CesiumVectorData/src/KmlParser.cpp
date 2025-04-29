#include "KmlParser.h"

#include <CesiumUtility/Result.h>

#include <fmt/format.h>
#include <tinyxml2.h>

#include <cstddef>
#include <functional>
#include <span>

using namespace CesiumUtility;
using namespace tinyxml2;

namespace CesiumVectorData {
namespace {
Result<VectorNode> parseKmlChild(XMLElement* pNode) {
  const std::string name(pNode->Name());
  std::function<bool(const std::string& name)> childPredicate = [](const std::string&){return true;}
  if (name == "Document" || name == "Folder" || name == "Feature" ||
      name == "NetworkLink" || name == "Placemark" ||
      name.ends_with("Overlay")) {
    
  }
}
} // namespace

Result<VectorNode> parseKml(const std::span<const std::byte>& bytes) {
  XMLDocument doc;
  XMLError error =
      doc.Parse(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (error != XML_SUCCESS) {
    return Result<VectorNode>(ErrorList::error(fmt::format(
        "Failed to parse KML: {}",
        XMLDocument::ErrorIDToName(error))));
  }

  XMLNode* pKml = doc.FirstChildElement("kml");
  if (!pKml) {
    return Result<VectorNode>(
        ErrorList::error("Invalid KML document, missing <kml></kml> node."));
  }

  Result<VectorNode> root(VectorNode{});
  for (tinyxml2::XMLElement* pNode = pKml->FirstChildElement();
       pNode != nullptr;
       pNode = pNode->NextSiblingElement()) {
    Result<VectorNode> child = parseKmlChild(pNode);
    root.errors.merge(child.errors);
    if (!child.value) {
      return Result<VectorNode>(std::move(root.errors));
    }

    root.value->children.emplace_back(std::move(*child.value));
  }

  return root;
}

} // namespace CesiumVectorData