#include "CameraWriter.h"
#include "ExtensionWriter.h"
#include <CesiumGltf/Camera.h>
#include <CesiumGltf/CameraOrthographic.h>
#include <CesiumGltf/CameraPerspective.h>
#include <CesiumJsonWriter/JsonObjectWriter.h>
#include <CesiumJsonWriter/JsonWriter.h>
#include <cstdint>
#include <utility>
#include <vector>

void writeOrthographicCamera(
    const CesiumGltf::CameraOrthographic& cameraOrthographic,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
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

  if (!cameraOrthographic.extensions.empty()) {
    CesiumGltf::writeExtensions(cameraOrthographic.extensions, j);
  }

  if (!cameraOrthographic.extras.empty()) {
    j.Key("extras");
    CesiumJsonWriter::writeJsonValue(cameraOrthographic.extras, j);
  }

  j.EndObject();
}

void writePerspectiveCamera(
    const CesiumGltf::CameraPerspective& cameraPerspective,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
  auto& j = jsonWriter;
  j.Key("perspective");
  j.StartObject();
  if (cameraPerspective.aspectRatio) {
    j.KeyPrimitive("aspectRatio", *cameraPerspective.aspectRatio);
  }
  j.Key("yfov");
  j.Double(cameraPerspective.yfov);
  if (cameraPerspective.zfar) {
    j.Key("zfar");
    j.Double(*cameraPerspective.zfar);
  }
  j.Key("znear");
  j.Double(cameraPerspective.znear);

  if (!cameraPerspective.extensions.empty()) {
    CesiumGltf::writeExtensions(cameraPerspective.extensions, j);
  }

  if (!cameraPerspective.extras.empty()) {
    j.Key("extras");
    CesiumJsonWriter::writeJsonValue(cameraPerspective.extras, j);
  }

  j.EndObject();
}

void CesiumGltf::writeCamera(
    const std::vector<Camera>& cameras,
    CesiumJsonWriter::JsonWriter& jsonWriter) {
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
    j.String(camera.type.c_str());

    if (!camera.name.empty()) {
      j.Key("name");
      j.String(camera.name.c_str());
    }

    if (!camera.extensions.empty()) {
      writeExtensions(camera.extensions, j);
    }

    if (!camera.extras.empty()) {
      j.Key("extras");
      writeJsonValue(camera.extras, j);
    }

    j.EndObject();
  }
  j.EndArray();
}
