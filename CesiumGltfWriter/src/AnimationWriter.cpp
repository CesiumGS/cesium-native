#include "JsonObjectWriter.h"
#include "ExtensionWriter.h"
#include "AnimationWriter.h"
#include <CesiumGltf/AnimationChannel.h>
#include <CesiumGltf/AnimationChannelTarget.h>
#include <magic_enum.hpp>
#include <stdexcept>
#include <type_traits>

void writeAnimationChannel(
    const CesiumGltf::AnimationChannel& animationChannel,
    CesiumGltf::JsonWriter& jsonWriter) {
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
        const auto pathAsString = std::string(magic_enum::enum_name(animationChannel.target.path));
        j.String(pathAsString.c_str());

        if (!animationChannel.target.extensions.empty()) {
            writeExtensions(animationChannel.target.extensions, j);
        }

        if (!animationChannel.target.extras.empty()) {
            j.Key("extras");
            CesiumGltf::writeJsonValue(animationChannel.target.extras, j, false);
        }
    }
    j.EndObject();
}

void writeAnimationSampler(
    const CesiumGltf::AnimationSampler& animationSampler,
    CesiumGltf::JsonWriter& jsonWriter
) {
    auto&j = jsonWriter;

    j.StartObject();
    j.KeyPrimitive("input", animationSampler.input);
    j.KeyPrimitive("interpolation", magic_enum::enum_name(animationSampler.interpolation));
    j.KeyPrimitive("output", animationSampler.output);

    if (!animationSampler.extensions.empty()) {
        writeExtensions(animationSampler.extensions, j);
    }

    if (!animationSampler.extras.empty()) {
        j.Key("extras");
        CesiumGltf::writeJsonValue(animationSampler.extras, j, false);
    }

    j.EndObject();
}

void CesiumGltf::writeAnimation(
    const std::vector<Animation>& animations,
    CesiumGltf::JsonWriter& jsonWriter) {

    if (animations.empty()) {
        return;
    }

    auto& j = jsonWriter;
    j.Key("animations");
    j.StartArray();
    for (const auto& animation : animations) {
        j.StartObject();
        if (animation.channels.empty()) {
            throw std::runtime_error("Must have at least 1 animation channel");
        }

        j.StartArray();
        for (const auto& animationChannel : animation.channels) {
            writeAnimationChannel(animationChannel, j);
        }
        j.EndArray();

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
            writeJsonValue(animation.extras, j, false);
        }

        j.EndObject();
    }
    j.EndArray();
}