#include "MetadataProperty.h"

#include <map>
#include <string>

namespace Cesium3DTilesContent {
const std::map<std::string, MetadataProperty::ComponentType>
    MetadataProperty::stringToMetadataComponentType{
        {"BYTE", MetadataProperty::ComponentType::BYTE},
        {"UNSIGNED_BYTE", MetadataProperty::ComponentType::UNSIGNED_BYTE},
        {"SHORT", MetadataProperty::ComponentType::SHORT},
        {"UNSIGNED_SHORT", MetadataProperty::ComponentType::UNSIGNED_SHORT},
        {"INT", MetadataProperty::ComponentType::INT},
        {"UNSIGNED_INT", MetadataProperty::ComponentType::UNSIGNED_INT},
        {"FLOAT", MetadataProperty::ComponentType::FLOAT},
        {"DOUBLE", MetadataProperty::ComponentType::DOUBLE},
    };

const std::map<std::string, MetadataProperty::Type>
    MetadataProperty::stringToMetadataType{
        {"SCALAR", MetadataProperty::Type::SCALAR},
        {"VEC2", MetadataProperty::Type::VEC2},
        {"VEC3", MetadataProperty::Type::VEC3},
        {"VEC4", MetadataProperty::Type::VEC4}};

} // namespace Cesium3DTilesContent
