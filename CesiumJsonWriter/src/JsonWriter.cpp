#include "CesiumJsonWriter/JsonWriter.h"

#include <rapidjson/reader.h>

#include <cassert>

using namespace CesiumJsonWriter;

namespace {

struct Dispatcher {
  IJsonHandler* pCurrent;

  bool update(IJsonHandler* pNext) noexcept {
    if (pNext == nullptr) {
      return false;
    }

    this->pCurrent = pNext;
    return true;
  }

  bool Null() { return update(pCurrent->writeNull()); }
  bool Bool(bool b) { return update(pCurrent->writeBool(b)); }
  bool Int(int i) { return update(pCurrent->writeInt32(i)); }
  bool Uint(unsigned i) { return update(pCurrent->writeUint32(i)); }
  bool Int64(int64_t i) { return update(pCurrent->writeInt64(i)); }
  bool Uint64(uint64_t i) { return update(pCurrent->writeUint64(i)); }
  bool Double(double d) { return update(pCurrent->writeDouble(d)); }
  bool RawNumber(
      const char* /* str */,
      size_t /* length */,
      bool /* copy */) noexcept {
    // This should not be called.
    assert(false);
    return false;
  }
  bool String(const char* str, size_t length, bool /* copy */) {
    return update(pCurrent->writeString(std::string_view(str, length)));
  }
  bool StartObject() { return update(pCurrent->writeObjectStart()); }
  bool Key(const char* str, size_t length, bool /* copy */) {
    return update(pCurrent->writeObjectKey(std::string_view(str, length)));
  }
  bool EndObject(size_t /* memberCount */) {
    return update(pCurrent->writeObjectEnd());
  }
  bool StartArray() { return update(pCurrent->writeArrayStart()); }
  bool EndArray(size_t /* elementCount */) {
    return update(pCurrent->writeArrayEnd());
  }
};

std::string getMessageFromRapidJsonError(rapidjson::ParseErrorCode code) {
  switch (code) {
  case rapidjson::ParseErrorCode::kParseErrorDocumentEmpty:
    return "The document is empty.";
  case rapidjson::ParseErrorCode::kParseErrorDocumentRootNotSingular:
    return "The document root must not be followed by other values.";
  case rapidjson::ParseErrorCode::kParseErrorValueInvalid:
    return "Invalid value.";
  case rapidjson::ParseErrorCode::kParseErrorObjectMissName:
    return "Missing a name for object member.";
  case rapidjson::ParseErrorCode::kParseErrorObjectMissColon:
    return "Missing a colon after a name of object member.";
  case rapidjson::ParseErrorCode::kParseErrorObjectMissCommaOrCurlyBracket:
    return "Missing a comma or '}' after an object member.";
  case rapidjson::ParseErrorCode::kParseErrorArrayMissCommaOrSquareBracket:
    return "Missing a comma or ']' after an array element.";
  case rapidjson::ParseErrorCode::kParseErrorStringUnicodeEscapeInvalidHex:
    return "Incorrect hex digit after \\u escape in string.";
  case rapidjson::ParseErrorCode::kParseErrorStringUnicodeSurrogateInvalid:
    return "The surrogate pair in string is invalid.";
  case rapidjson::ParseErrorCode::kParseErrorStringEscapeInvalid:
    return "Invalid escape character in string.";
  case rapidjson::ParseErrorCode::kParseErrorStringMissQuotationMark:
    return "Missing a closing quotation mark in string.";
  case rapidjson::ParseErrorCode::kParseErrorStringInvalidEncoding:
    return "Invalid encoding in string.";
  case rapidjson::ParseErrorCode::kParseErrorNumberTooBig:
    return "Number too big to be stored in double.";
  case rapidjson::ParseErrorCode::kParseErrorNumberMissFraction:
    return "Missing fraction part in number.";
  case rapidjson::ParseErrorCode::kParseErrorNumberMissExponent:
    return "Missing exponent in number.";
  case rapidjson::ParseErrorCode::kParseErrorTermination:
    return "Parsing was terminated.";
  case rapidjson::ParseErrorCode::kParseErrorUnspecificSyntaxError:
  default:
    return "Unspecific syntax error.";
  }
}

} // namespace

JsonWriter::FinalJsonHandler::FinalJsonHandler(
    std::vector<std::string>& warnings)
    : JsonHandler(), _warnings(warnings), _pInputStream(nullptr) {
  reset(this);
}

void JsonWriter::FinalJsonHandler::reportWarning(
    const std::string& warning,
    std::vector<std::string>&& context) {
  std::string fullWarning = warning;
  fullWarning += "\n  While parsing: ";
  for (auto it = context.rbegin(); it != context.rend(); ++it) {
    fullWarning += *it;
  }

  fullWarning += "\n  From byte offset: ";
  fullWarning += this->_pInputStream
                     ? std::to_string(this->_pInputStream->Tell())
                     : "unknown";

  this->_warnings.emplace_back(std::move(fullWarning));
}

void JsonWriter::FinalJsonHandler::setInputStream(
    rapidjson::MemoryStream* pInputStream) noexcept {
  this->_pInputStream = pInputStream;
}

/*static*/ void JsonWriter::internalWrite(
    const gsl::span<const std::byte>& data,
    IJsonHandler& handler,
    FinalJsonHandler& finalHandler,
    std::vector<std::string>& errors,
    std::vector<std::string>& /* warnings */) {

  rapidjson::Reader writer;
  rapidjson::MemoryStream inputStream(
      reinterpret_cast<const char*>(data.data()),
      data.size());

  finalHandler.setInputStream(&inputStream);

  Dispatcher dispatcher{&handler};

  writer.IterativeParseInit();

  bool success = true;
  while (success && !writer.IterativeParseComplete()) {
    success = writer.IterativeParseNext<rapidjson::kParseDefaultFlags>(
        inputStream,
        dispatcher);
  }

  if (writer.HasParseError()) {
    std::string s("JSON parsing error at byte offset ");
    s += std::to_string(writer.GetErrorOffset());
    s += ": ";
    s += getMessageFromRapidJsonError(writer.GetParseErrorCode());
    errors.emplace_back(std::move(s));
  }
}
