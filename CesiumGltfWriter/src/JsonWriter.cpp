#include "JsonWriter.h"

namespace CesiumGltf {
    JsonWriter::JsonWriter()
        : writer(std::make_optional(
              rapidjson::Writer<rapidjson::StringBuffer>(_stringBuffer))) {}

    bool JsonWriter::Null() { return writer->Null(); }

    bool JsonWriter::Bool(bool b) { return writer->Bool(b); }

    bool JsonWriter::Int(int i) { return writer->Int(i); }

    bool JsonWriter::Uint(unsigned int i) { return writer->Uint(i); }

    bool JsonWriter::Uint64(std::uint64_t i) { return writer->Uint64(i); }

    bool JsonWriter::Int64(std::int64_t i) { return writer->Int64(i); }

    bool JsonWriter::Double(double d) { return writer->Double(d); }

    bool
    JsonWriter::RawNumber(const char* str, unsigned int length, bool copy) {
        return writer->RawNumber(str, length, copy);
    }

    bool JsonWriter::String(std::string_view string) {
        return writer->String(string.data(), static_cast<unsigned int>(string.size()));
    }

    bool JsonWriter::Key(std::string_view key) {
        return writer->Key(key.data(), static_cast<unsigned int>(key.size()));
    }

    bool JsonWriter::StartObject() { return writer->StartObject(); }

    bool JsonWriter::EndObject() { return writer->EndObject(); }

    bool JsonWriter::StartArray() { return writer->StartArray(); }

    bool JsonWriter::EndArray() { return writer->EndArray(); }


    void JsonWriter::Primitive(std::int32_t value) { writer->Int(value); }

    void JsonWriter::Primitive(std::uint32_t value) { writer->Uint(value); }

    void JsonWriter::Primitive(std::int64_t value) { writer->Int64(value); }

    void JsonWriter::Primitive(std::uint64_t value) { writer->Uint64(value); }

    void JsonWriter::Primitive(float value) {
        writer->Double(static_cast<double>(value));
    }

    void JsonWriter::Primitive(double value) { writer->Double(value); }

    void JsonWriter::Primitive(std::nullptr_t) { writer->Null(); }

    void JsonWriter::Primitive(std::string_view string) { 
        writer->String(string.data(), static_cast<unsigned int>(string.size()));
    }

    // Integral
    void
    JsonWriter::KeyPrimitive(std::string_view keyName, std::int32_t value) {
        Key(keyName);
        Primitive(value);
    }

    void
    JsonWriter::KeyPrimitive(std::string_view keyName, std::uint32_t value) {
        Key(keyName);
        Primitive(value);
    }

    void
    JsonWriter::KeyPrimitive(std::string_view keyName, std::int64_t value) {
        Key(keyName);
        Primitive(value);
    }

    void
    JsonWriter::KeyPrimitive(std::string_view keyName, std::uint64_t value) {
        Key(keyName);
        Primitive(value);
    }

    void
    JsonWriter::KeyPrimitive(std::string_view keyName, std::string_view value) {
        Key(keyName);
        Primitive(value);
    }

    // Floating Point
    void JsonWriter::KeyPrimitive(std::string_view keyName, float value) {
        Key(keyName);
        Primitive(value);
    }

    void JsonWriter::KeyPrimitive(std::string_view keyName, double value) {
        Key(keyName);
        Primitive(value);
    }

    // Null
    void
    JsonWriter::KeyPrimitive(std::string_view keyName, std::nullptr_t value) {
        Key(keyName);
        Primitive(value);
    }

    // Array / Objects
    void JsonWriter::KeyArray(
        std::string_view keyName,
        std::function<void(void)> insideArray) {
        Key(keyName);
        writer->StartArray();
        insideArray();
        writer->EndArray();
    }

    void JsonWriter::KeyObject(
        std::string_view keyName,
        std::function<void(void)> insideObject) {
        Key(keyName);
        writer->StartObject();
        insideObject();
        writer->EndObject();
    }

    std::string_view JsonWriter::toString() {
        return std::string_view(_stringBuffer.GetString());
    }

}
