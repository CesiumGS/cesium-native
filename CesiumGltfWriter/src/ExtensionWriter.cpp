#include "ExtensionWriter.h"

void CesiumGltf::writeExtensions(
    const std::vector<std::any>& extensions, 
    CesiumGltf::JsonWriter& jsonWriter
) {
    if (extensions.empty()) {
        return;
    }
    
    auto& j = jsonWriter;
    
    j.Key("extensions");
    
    j.StartArray();
    /*
    for (const auto& extension :1 extensions) {
        // TODO: implement me
    }
    */
    j.EndArray();
}