#include "CameraWriter.h"
#include <CesiumGltf/Camera.h>
#include <CesiumGltf/CameraOrthographic.h>
#include <CesiumGltf/CameraPerspective.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <utility>
#include <vector>
#include <cstdint>
#include <magic_enum.hpp>

void writeOrthographicCamera(
    const CesiumGltf::CameraOrthographic& cameraOrthographic,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
) {
    auto& j = jsonWriter;
    j.Key("orthographic");
    j.StartObject();
    j.Key("xmag");
    j.Double(cameraOrthographic.xmag);
    j.Key("ymag");
    j.Double(cameraOrthographic.ymag);
    j.Key("zfar");
    j.Double(cameraOrthographic.zfar);
    j.Key("znear");
    j.Double(cameraOrthographic.znear);
    j.EndObject();
    // todo: extensions / extras
}

void writePerspectiveCamera(
    const CesiumGltf::CameraPerspective& cameraPerspective,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
) {
    auto& j = jsonWriter;
    j.Key("perspective");
    j.StartObject();
    if (cameraPerspective.aspectRatio) {
        j.Key("aspectRatio");
        j.Double(*cameraPerspective.aspectRatio);
    }
    j.Key("yfov");
    j.Double(cameraPerspective.yfov);
    if (cameraPerspective.zfar) {
        j.Key("zfar");
        j.Double(*cameraPerspective.zfar);
    }
    j.Key("znear");
    j.Double(cameraPerspective.znear);
    // todo: extensions / extras
    j.EndObject();
}

void CesiumGltf::writeCamera(
    const std::vector<Camera>& cameras,
    rapidjson::Writer<rapidjson::StringBuffer>& jsonWriter
) {
    if (cameras.empty()) {
        return;
    }

    auto& j = jsonWriter;

    j.Key("cameras");
    j.StartArray();

    for (const auto& camera : cameras) {
        j.StartObject();
        if (camera.orthographic) {
            writeOrthographicCamera(*camera.orthographic, j);
        }

        else if (camera.perspective) {
            writePerspectiveCamera(*camera.perspective, j);
        }

        j.Key("type");
        auto cameraTypeAsString = std::string(magic_enum::enum_name(camera.type));
        j.String(cameraTypeAsString.c_str());

        if (!camera.name.empty()) {
            j.Key("name");
            j.String(camera.name.c_str());
        }
        j.EndObject();
    }
    j.EndArray();
}