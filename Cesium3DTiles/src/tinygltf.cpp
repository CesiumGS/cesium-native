#define TINYGLTF_USE_CPP14 
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#define TINYGLTF_ENABLE_DRACO
#define TINYGLTF_USE_RAPIDJSON
#define TINYGLTF_NO_INCLUDE_JSON
#define TINYGLTF_USE_RAPIDJSON_CRTALLOCATOR

#ifdef _MSC_VER
#pragma warning(disable:4996 4946 4100 4189 4127)
#endif

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <tiny_gltf.h>
