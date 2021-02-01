#include <rapidjson/writer.h>
#include <vector>
#include <cstdint>

namespace CesiumGltf {
    template<typename T>
    void addGLBBuffers(rapidjson::Writer<T> writer);
}