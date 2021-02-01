#include "AnimationWriter.h"
#include <CesiumGltf/AnimationChannel.h>
#include <CesiumGltf/AnimationChannelTarget.h>
#include <magic_enum.hpp>
#include <stdexcept>
#include <type_traits>

void writeAnimationChannel(
    const CesiumGltf::AnimationChannel& animationChannel,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {
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
            throw std::runtime_error("Not implemented");
        }

        if (!animationChannel.target.extras.empty()) {
            throw std::runtime_error("Not implemented");
        }

    }
    j.EndObject();
}

void writeAnimationSampler(
    const CesiumGltf::AnimationSampler& animationSampler,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
) {
    auto&j = jsonWriter;

    j.StartObject();
    j.Key("input");
    j.Int(animationSampler.input);

    j.Key("interpolation");
    const auto interpolationAsString = std::string(magic_enum::enum_name(animationSampler.interpolation));
    j.String(interpolationAsString.c_str());

    j.Key("output");
    j.Int(animationSampler.output);

    // TODO: extensions / extras

    j.EndObject();
}

void CesiumGltf::writeAnimation(
    const std::vector<Animation>& animations,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter) {

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
    }
    j.EndArray();
}