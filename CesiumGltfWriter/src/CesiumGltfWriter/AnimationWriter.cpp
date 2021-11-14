#include "CesiumGltfWriter/AnimationWriter.h"

#include "CesiumGltfWriter/ExtensionWriter.h"
#include "CesiumGltfWriter/WriteModelResult.h"

#include <CesiumGltf/Animation.h>
#include <CesiumGltf/AnimationChannel.h>
#include <CesiumGltf/AnimationChannelTarget.h>
#include <CesiumGltf/AnimationSampler.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>

#include <stdexcept>
#include <type_traits>

namespace CesiumGltfWriter {
namespace {
void writeAnimationChannel(
    const CesiumGltf::AnimationChannel& animationChannel,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
  auto& j = jsonWriter;
  j.StartObject();
  j.Key("sampler");
  j.Int(animationChannel.sampler);
  j.Key("target");
  {
    j.StartObject();
    j.Key("node");
    j.Int(animationChannel.target.node);
    j.Key("path");
    j.String(animationChannel.target.path.c_str());

    if (!animationChannel.target.extensions.empty()) {
      writeExtensions(animationChannel.target.extensions, j);
    }

    if (!animationChannel.target.extras.empty()) {
      j.Key("extras");
      CesiumJsonWriter::writeJsonValue(animationChannel.target.extras, j);
    }
  }
  j.EndObject();
}

void writeAnimationSampler(
    const CesiumGltf::AnimationSampler& animationSampler,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
  auto& j = jsonWriter;

  j.StartObject();
  j.KeyPrimitive("input", animationSampler.input);
  j.KeyPrimitive("interpolation", animationSampler.interpolation);
  j.KeyPrimitive("output", animationSampler.output);

  if (!animationSampler.extensions.empty()) {
    writeExtensions(animationSampler.extensions, j);
  }

  if (!animationSampler.extras.empty()) {
    j.Key("extras");
    CesiumJsonWriter::writeJsonValue(animationSampler.extras, j);
  }

  j.EndObject();
}
} // namespace

void writeAnimation(
    WriteModelResult& result,
    const std::vector<CesiumGltf::Animation>& animations,
    CesiumJsonWriter::JsonWriter& jsonWriter) {

  if (animations.empty()) {
    return;
  }

  auto& j = jsonWriter;
  j.Key("animations");
  j.StartArray();
  for (std::size_t i = 0; i < animations.size(); ++i) {
    auto& animation = animations.at(i);
    j.StartObject();
    const bool hasAnimationChannels = !animation.channels.empty();
    if (!hasAnimationChannels) {
      const std::string culpableAnimation =
          "animations[" + std::to_string(i) + "]";
      result.warnings.emplace_back(
          "EmptyAnimationChannels: " + culpableAnimation + " " +
          "is missing animation channels. The generated " +
          "glTF asset will not be glTF 2.0 spec-compliant");
      j.StartArray();
      j.EndArray();
    } else {
      j.StartArray();
      for (const auto& animationChannel : animation.channels) {
        writeAnimationChannel(animationChannel, j);
      }
      j.EndArray();
    }

    j.StartArray();
    for (const auto& animationSampler : animation.samplers) {
      writeAnimationSampler(animationSampler, j);
    }
    j.EndArray();

    if (!animation.extensions.empty()) {
      writeExtensions(animation.extensions, j);
    }

    if (!animation.extras.empty()) {
      j.Key("extras");
      writeJsonValue(animation.extras, j);
    }

    j.EndObject();
  }
  j.EndArray();
}
} // namespace CesiumGltfWriter