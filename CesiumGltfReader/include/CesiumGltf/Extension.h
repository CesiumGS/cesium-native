#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace CesiumGltf {

class IJsonHandler;
class JsonHandler;
struct ExtensibleObject;
struct ModelReaderResult;
struct ReadModelOptions;

class Extension {
public:
  virtual ~Extension() noexcept {}

  virtual std::unique_ptr<IJsonHandler> readExtension(
      const ReadModelOptions& options,
      const std::string& extensionName,
      ExtensibleObject& parent,
      IJsonHandler* pParentHandler,
      const std::string& ownerType) = 0;

  virtual void postprocess(
      ModelReaderResult& readModel,
      const ReadModelOptions& options) = 0;
};

} // namespace CesiumGltf
