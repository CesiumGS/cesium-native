#include "SamplerWriter.h"

#include "ExtensionWriter.h"

#include <CesiumJsonWriter/JsonObjectWriter.h>

void CesiumGltf::writeSampler(
    const std::vector<Sampler>& samplers,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
  if (samplers.empty()) {
    return;
  }

  auto& j = jsonWriter;

  j.Key("samplers");
  j.StartArray();
  for (const auto& sampler : samplers) {
    j.StartObject();

    if (sampler.magFilter) {
      j.Key("magFilter");
      j.Int(*sampler.magFilter);
    }

    if (sampler.minFilter) {
      j.Key("minFilter");
      j.Int(*sampler.minFilter);
    }

    if (sampler.wrapS != Sampler::WrapS::REPEAT) {
      j.Key("wrapS");
      j.Int(sampler.wrapS);
    }

    if (sampler.wrapT != Sampler::WrapT::REPEAT) {
      j.Key("wrapT");
      j.Int(sampler.wrapT);
    }

    if (!sampler.name.empty()) {
      j.KeyPrimitive("name", sampler.name);
    }

    if (sampler.extensions.empty()) {
      writeExtensions(sampler.extensions, j);
    }

    if (!sampler.extras.empty()) {
      j.Key("extras");
      writeJsonValue(sampler.extras, j);
    }

    j.EndObject();
  }
  j.EndArray();
}
