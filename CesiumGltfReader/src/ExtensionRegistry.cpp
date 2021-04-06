#include "CesiumGltf/ExtensionRegistry.h"
#include "ExtensionKHR_draco_mesh_compression.h"

using namespace CesiumGltf;

namespace {
class ExtensionAny : public Extension {
public:
  virtual std::unique_ptr<IJsonHandler> readExtension(
      const ReadModelOptions& options,
      const std::string& extensionName,
      ExtensibleObject& parent,
      IJsonHandler* pParentHandler,
      const std::string& ownerType) override;

  virtual void postprocess(
      ModelReaderResult& readModel,
      const ReadModelOptions& options) override;
};

std::unique_ptr<IJsonHandler> ExtensionAny::readExtension(
    const ReadModelOptions& options,
    const std::string& extensionName,
    ExtensibleObject& parent,
    IJsonHandler* pParentHandler,
    const std::string& /* ownerType */) {
  std::unique_ptr<JsonObjectJsonHandler> pHandler =
      std::make_unique<JsonObjectJsonHandler>(options);
  std::any& value =
      parent.extensions.emplace(extensionName, JsonValue(JsonValue::Object()))
          .first->second;
  pHandler->reset(pParentHandler, &std::any_cast<JsonValue&>(value));
  return pHandler;
}

void ExtensionAny::postprocess(
    ModelReaderResult& /* readModel */,
    const ReadModelOptions& /* options */) {}

std::shared_ptr<ExtensionRegistry> createExtensionRegistry() {
  std::shared_ptr<ExtensionRegistry> p = std::make_shared<ExtensionRegistry>();

  p->registerExtension<ExtensionKHR_draco_mesh_compression>();
  p->registerDefault<ExtensionAny>();

  return p;
}
} // namespace

/*static*/ std::shared_ptr<ExtensionRegistry> ExtensionRegistry::getDefault() {
  static std::shared_ptr<ExtensionRegistry> pDefault =
      createExtensionRegistry();
  return pDefault;
}

Extension* ExtensionRegistry::findExtension(const std::string& name) const {
  auto it = this->_extensions.find(name);
  if (it == this->_extensions.end()) {
    return this->_pDefault.get();
  } else {
    return it->second.get();
  }
}
