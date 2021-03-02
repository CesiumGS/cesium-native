#include "PrettyJsonWriter.h"
#include <rapidjson/prettywriter.h>

namespace CesiumGltf {
    PrettyJsonWriter::PrettyJsonWriter() {
        auto writer = rapidjson::PrettyWriter<rapidjson::StringBuffer>(_prettyBuffer);
        writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
        pretty = std::make_unique<rapidjson::PrettyWriter<rapidjson::StringBuffer>>(std::move(writer));
    }

    bool PrettyJsonWriter::Null() { return pretty->Null(); }

    bool PrettyJsonWriter::Bool(bool b) { return pretty->Bool(b); }

    bool PrettyJsonWriter::Int(int i) { return pretty->Int(i); }

    bool PrettyJsonWriter::Uint(unsigned int i) { return pretty->Uint(i); }

    bool PrettyJsonWriter::Uint64(std::uint64_t i) { return pretty->Uint64(i); }

    bool PrettyJsonWriter::Int64(std::int64_t i) { return pretty->Int64(i); }

    bool PrettyJsonWriter::Double(double d) { return pretty->Double(d); }

    bool
    PrettyJsonWriter::RawNumber(const char* str, unsigned int length, bool copy) {
        return pretty->RawNumber(str, length, copy);
    }

    bool PrettyJsonWriter::String(std::string_view string) {
        return pretty->String(string.data(), static_cast<unsigned int>(string.size()));
    }

    bool PrettyJsonWriter::Key(std::string_view key) {
        return pretty->Key(key.data(), static_cast<unsigned int>(key.size()));
    }

    bool PrettyJsonWriter::StartObject() { return pretty->StartObject(); }

    bool PrettyJsonWriter::EndObject() { return pretty->EndObject(); }

    bool PrettyJsonWriter::StartArray() { return pretty->StartArray(); }

    bool PrettyJsonWriter::EndArray() { return pretty->EndArray(); }


    void PrettyJsonWriter::Primitive(std::int32_t value) { pretty->Int(value); }

    void PrettyJsonWriter::Primitive(std::uint32_t value) { pretty->Uint(value); }

    void PrettyJsonWriter::Primitive(std::int64_t value) { pretty->Int64(value); }

    void PrettyJsonWriter::Primitive(std::uint64_t value) { pretty->Uint64(value); }

    void PrettyJsonWriter::Primitive(float value) {
        pretty->Double(static_cast<double>(value));
    }

    void PrettyJsonWriter::Primitive(double value) { pretty->Double(value); }

    void PrettyJsonWriter::Primitive(std::nullptr_t) { pretty->Null(); }

    void PrettyJsonWriter::Primitive(std::string_view string) { 
        pretty->String(string.data(), static_cast<unsigned int>(string.size()));
    }

    // Integral
    void
    PrettyJsonWriter::KeyPrimitive(std::string_view keyName, std::int32_t value) {
        Key(keyName);
        Primitive(value);
    }

    void
    PrettyJsonWriter::KeyPrimitive(std::string_view keyName, std::uint32_t value) {
        Key(keyName);
        Primitive(value);
    }

    void
    PrettyJsonWriter::KeyPrimitive(std::string_view keyName, std::int64_t value) {
        Key(keyName);
        Primitive(value);
    }

    void
    PrettyJsonWriter::KeyPrimitive(std::string_view keyName, std::uint64_t value) {
        Key(keyName);
        Primitive(value);
    }

    void
    PrettyJsonWriter::KeyPrimitive(std::string_view keyName, std::string_view value) {
        Key(keyName);
        Primitive(value);
    }

    // Floating Point
    void PrettyJsonWriter::KeyPrimitive(std::string_view keyName, float value) {
        Key(keyName);
        Primitive(value);
    }

    void PrettyJsonWriter::KeyPrimitive(std::string_view keyName, double value) {
        Key(keyName);
        Primitive(value);
    }

    // Null
    void
    PrettyJsonWriter::KeyPrimitive(std::string_view keyName, std::nullptr_t value) {
        Key(keyName);
        Primitive(value);
    }

    // Array / Objects
    void PrettyJsonWriter::KeyArray(
        std::string_view keyName,
        std::function<void(void)> insideArray) {
        Key(keyName);
        pretty->StartArray();
        insideArray();
        pretty->EndArray();
    }

    void PrettyJsonWriter::KeyObject(
        std::string_view keyName,
        std::function<void(void)> insideObject) {
        Key(keyName);
        pretty->StartObject();
        insideObject();
        pretty->EndObject();
    }

    std::string_view PrettyJsonWriter::toString() {
        return std::string_view(_prettyBuffer.GetString());
    }

    std::vector<std::uint8_t> PrettyJsonWriter::toBytes() {
        const auto& string = _prettyBuffer.GetString();
        const auto& size = _prettyBuffer.GetSize();
        std::vector<uint8_t> result(size);
        std::copy(string, string + size, result.begin());
        return result;
    }
}
