#pragma once

#include "CesiumGltf/Extension.h"
#include "CesiumGltf/KHR_draco_mesh_compression.h"
#include "JsonObjectJsonHandler.h"
#include "KHR_draco_mesh_compressionJsonHandler.h"
#include "MeshPrimitiveJsonHandler.h"
#include <string>

namespace CesiumGltf {

class ExtensionKHR_draco_mesh_compression : public Extension {
public:
  static const std::string ExtensionName;

  virtual std::unique_ptr<IJsonHandler> readExtension(
      const ReadModelOptions& options,
      const std::string& extensionName,
      ExtensibleObject& parent,
      IJsonHandler* pParentHandler,
      const std::string& ownerType) override;

  virtual void postprocess(
      ModelReaderResult& /* readModel */,
      const ReadModelOptions& /* options */) override;
};

} // namespace CesiumGltf
