#include "CesiumGltf/Reader.h"
#include "JsonHandler.h"
#include "ModelJsonHandler.h"
#include "RejectAllJsonHandler.h"
#include <rapidjson/reader.h>
#include <string>

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

    result.model.emplace();
    modelHandler.reset(&rejectHandler, &result.model.value());

    reader.IterativeParseInit();

    bool success = true;
    while (success && !reader.IterativeParseComplete()) {
        success = reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(inputStream, dispatcher);
    }

    if (reader.HasParseError()) {
        result.model.reset();

        std::string s("glTF JSON parsing error at byte offset ");
        s += std::to_string(reader.GetErrorOffset());
        s += ": ";

        switch (reader.GetParseErrorCode()) {
            case rapidjson::ParseErrorCode::kParseErrorDocumentEmpty:
                s += "The document is empty.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorDocumentRootNotSingular:
                s += "The document root must not be followed by other values.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorValueInvalid:
                s += "Invalid value.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorObjectMissName:
                s += "Missing a name for object member.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorObjectMissColon:
                s += "Missing a colon after a name of object member.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorObjectMissCommaOrCurlyBracket:
                s += "Missing a comma or '}' after an object member.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorArrayMissCommaOrSquareBracket:
                s += "Missing a comma or ']' after an array element.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorStringUnicodeEscapeInvalidHex:
                s += "Incorrect hex digit after \\u escape in string.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorStringUnicodeSurrogateInvalid:
                s += "The surrogate pair in string is invalid.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorStringEscapeInvalid:
                s += "Invalid escape character in string.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorStringMissQuotationMark:
                s += "Missing a closing quotation mark in string.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorStringInvalidEncoding:
                s += "Invalid encoding in string.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorNumberTooBig:
                s += "Number too big to be stored in double.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorNumberMissFraction:
                s += "Missing fraction part in number.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorNumberMissExponent:
                s += "Missing exponent in number.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorTermination:
                s += "Parsing was terminated.";
                break;
            case rapidjson::ParseErrorCode::kParseErrorUnspecificSyntaxError:
            default:
                s += "Unspecific syntax error.";
                break;
        }

        if (!result.errors.empty()) {
            result.errors.push_back('\n');
        }
        result.errors += s;
    }

    return result;
}
