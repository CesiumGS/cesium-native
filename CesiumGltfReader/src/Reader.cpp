#include "CesiumGltf/Reader.h"
#include "JsonHandler.h"
#include "ModelJsonHandler.h"
#include "RejectAllJsonHandler.h"
#include <rapidjson/reader.h>

using namespace CesiumGltf;

namespace {
    struct Dispatcher {
        JsonHandler* pCurrent;

        bool update(JsonHandler* pNext) {
            if (pNext == nullptr) {
                return false;
            }
            
            this->pCurrent = pNext;
            return true;
        }

        bool Null() { return update(pCurrent->Null()); }
        bool Bool(bool b) { return update(pCurrent->Bool(b)); }
        bool Int(int i) { return update(pCurrent->Int(i)); }
        bool Uint(unsigned i) { return update(pCurrent->Uint(i)); }
        bool Int64(int64_t i) { return update(pCurrent->Int64(i)); }
        bool Uint64(uint64_t i) { return update(pCurrent->Uint64(i)); }
        bool Double(double d) { return update(pCurrent->Double(d)); }
        bool RawNumber(const char* str, size_t length, bool copy) { return update(pCurrent->RawNumber(str, length, copy)); }
        bool String(const char* str, size_t length, bool copy) { return update(pCurrent->String(str, length, copy)); }
        bool StartObject() { return update(pCurrent->StartObject()); }
        bool Key(const char* str, size_t length, bool copy) { return update(pCurrent->Key(str, length, copy)); }
        bool EndObject(size_t memberCount) { return update(pCurrent->EndObject(memberCount)); }
        bool StartArray() { return update(pCurrent->StartArray()); }
        bool EndArray(size_t elementCount) { return update(pCurrent->EndArray(elementCount)); }
    };
}

ModelReaderResult CesiumGltf::readModel(const gsl::span<const uint8_t>& data) {
    rapidjson::Reader reader;
    rapidjson::MemoryStream inputStream(reinterpret_cast<const char*>(data.data()), data.size());

    ModelReaderResult result;
    ModelJsonHandler modelHandler;
    RejectAllJsonHandler rejectHandler;
    Dispatcher dispatcher { &modelHandler };

    modelHandler.reset(&rejectHandler, &result.model);

    reader.IterativeParseInit();

    bool success = true;
    while (success && !reader.IterativeParseComplete()) {
        success = reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(inputStream, dispatcher);
    }

    return result;
}
