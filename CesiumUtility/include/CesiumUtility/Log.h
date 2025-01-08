#pragma once
#include <rapidjson/document.h>
#include <spdlog/fmt/fmt.h>

/** @cond Doxygen_Exclude */
template <>
struct fmt::formatter<rapidjson::ParseErrorCode> : formatter<string_view> {
  // parse is inherited from formatter<string_view>.

  auto format(rapidjson::ParseErrorCode code, format_context& ctx) const {
    string_view name = "unknown";
    switch (code) {
    case rapidjson::ParseErrorCode::kParseErrorNone:
      name = "No error.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorDocumentEmpty:
      name = "The document is empty.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorDocumentRootNotSingular:
      name = "The document root must not follow by other values.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorValueInvalid:
      name = "Invalid value.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorObjectMissName:
      name = "Missing a name for object member.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorObjectMissColon:
      name = "Missing a colon after a name of object member.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorObjectMissCommaOrCurlyBracket:
      name = "Missing a comma or '}' after an object member.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorArrayMissCommaOrSquareBracket:
      name = "Missing a comma or ']' after an array element.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorStringUnicodeEscapeInvalidHex:
      name = "Incorrect hex digit after \\u escape in string.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorStringUnicodeSurrogateInvalid:
      name = "The surrogate pair in string is invalid.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorStringEscapeInvalid:
      name = "Invalid escape character in string.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorStringMissQuotationMark:
      name = "Missing a closing quotation mark in string.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorStringInvalidEncoding:
      name = "Invalid encoding in string.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorNumberTooBig:
      name = "Number too big to be stored in double.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorNumberMissFraction:
      name = "Miss fraction part in number.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorNumberMissExponent:
      name = "Miss exponent in number.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorTermination:
      name = "Parsing was terminated.";
      break;
    case rapidjson::ParseErrorCode::kParseErrorUnspecificSyntaxError:
      name = "Unspecific syntax error.";
      break;
    default:
      break;
    }
    return formatter<string_view>::format(name, ctx);
  }
};
/** @endcond */