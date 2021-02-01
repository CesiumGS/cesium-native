#include "CesiumGltf/Writer.h"
#include "AccessorWriter.h"
#include "AnimationWriter.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <stdexcept>
#include <iostream>

using namespace rapidjson;

std::vector<std::byte>
CesiumGltf::writeModelToByteArray(const Model& model, WriteOptions options) {

  std::vector<std::byte> result;
  if (options != (WriteOptions::GLB | WriteOptions::EmbedBuffers |
                  WriteOptions::EmbedImages)) {
    throw std::runtime_error("Unsupported configuration. Only (GLB + "
                             "EmbedBuffers + EmbedImages) supported for now.");
  }

  StringBuffer strBuffer;
  Writer<StringBuffer> writer(strBuffer);
  writer.StartObject();
  CesiumGltf::writeAccessor(model.accessors, writer);
  CesiumGltf::writeAnimation(model.animations, writer);
  // writeMesh
  // writePrimitive
  // writeBuffer...

  writer.EndObject();
  return result;
}