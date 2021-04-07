#include "ExtensionKHR_draco_mesh_compression.h"

using namespace CesiumGltf;

/*static*/ const std::string
    ExtensionKHR_draco_mesh_compression::ExtensionName =
        "KHR_draco_mesh_compression";

std::unique_ptr<IJsonHandler>
ExtensionKHR_draco_mesh_compression::readExtension(
    const ReadModelOptions& options,
    const std::string_view& extensionName,
    ExtensibleObject& parent,
    IJsonHandler* pParentHandler,
    const std::string& ownerType) {

  if (ownerType != MeshPrimitive::TypeName) {
    std::unique_ptr<JsonObjectJsonHandler> pHandler =
        std::make_unique<JsonObjectJsonHandler>(options);
    std::any& value =
        parent.extensions.emplace(extensionName, JsonValue(JsonValue::Object()))
            .first->second;
    pHandler->reset(pParentHandler, &std::any_cast<JsonValue&>(value));
    return pHandler;
  }

  std::unique_ptr<KHR_draco_mesh_compressionJsonHandler> pHandler =
      std::make_unique<KHR_draco_mesh_compressionJsonHandler>(options);
  std::any& value =
      parent.extensions.emplace(extensionName, KHR_draco_mesh_compression())
          .first->second;
  pHandler->reset(
      pParentHandler,
      &std::any_cast<KHR_draco_mesh_compression&>(value));
  return pHandler;
}

void ExtensionKHR_draco_mesh_compression::postprocess(
    ModelReaderResult& /* readModel */,
    const ReadModelOptions& /* options */) {}
