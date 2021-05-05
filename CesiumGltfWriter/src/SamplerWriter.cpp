#include "SamplerWriter.h"
#include "ExtensionWriter.h"
#include "JsonObjectWriter.h"
#include <magic_enum.hpp>

void CesiumGltf::writeSampler(
    const std::vector<Sampler>& samplers,
    CesiumGltf::JsonWriter& jsonWriter) {
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
      j.Int(magic_enum::enum_integer(*sampler.magFilter));
    }

    if (sampler.minFilter) {
      j.Key("minFilter");
      j.Int(magic_enum::enum_integer(*sampler.minFilter));
    }

    if (sampler.wrapS != Sampler::WrapS::REPEAT) {
      j.Key("wrapS");
      j.Int(magic_enum::enum_integer(sampler.wrapS));
    }

    if (sampler.wrapT != Sampler::WrapT::REPEAT) {
      j.Key("wrapT");
      j.Int(magic_enum::enum_integer(sampler.wrapT));
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
