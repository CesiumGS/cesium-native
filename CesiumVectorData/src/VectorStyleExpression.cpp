#include "CesiumUtility/Result.h"

#include <fmt/format.h>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

using namespace CesiumUtility;

enum class VectorStyleExpressionTokenType : uint8_t {
  EndOfFile = 0,
  Identifier,
  Number,
  String,
  OpenSquareBracket,
  CloseSquareBracket,
  OpenCurlyBracket,
  CloseCurlyBracket,
  OpenParen,
  CloseParen,
  TemplateSign,
  Plus,
  Minus,
  ExclamationPoint,
  Or,
  And,
  Equals,
  NotEquals,
  GreaterThan,
  LessThan,
  GreaterThanEq,
  LessThanEq,
  ForwardSlash,
  Percent,
  RegexEq,
  RegexNeq,
  QuestionMark,
  Colon,
  Dot
};

struct VectorStyleExpressionToken {
  VectorStyleExpressionTokenType type;
  size_t position;
  std::string_view value;
};

class VectorStyleExpressionTokenizer {
public:
  VectorStyleExpressionTokenizer(std::string&& text) : _text(std::move(text)) {}

  Result<std::vector<VectorStyleExpressionToken>> tokenizeAll() {
    size_t position = 0;
    std::vector<VectorStyleExpressionToken> tokens;

    while (position <= this->_text.length()) {
      Result<VectorStyleExpressionToken> tokenResult =
          this->nextToken(position);
      if (!tokenResult.value) {
        return Result<std::vector<VectorStyleExpressionToken>>(
            tokenResult.errors);
      }

      if (tokenResult.value->type ==
          VectorStyleExpressionTokenType::EndOfFile) {
        break;
      }

      position += tokenResult.value->value.length();
      tokens.emplace_back(std::move(*tokenResult.value));
    }

    return tokens;
  }

private:
  Result<VectorStyleExpressionToken> nextToken(size_t position) {
    if (position >= this->_text.length()) {
      return Result<VectorStyleExpressionToken>(VectorStyleExpressionToken{
          VectorStyleExpressionTokenType::EndOfFile,
          position,
          {}});
    }

    const char nextCh = this->_text[position];
    // Handle single-char tokens
    if (nextCh == '[') {
      return tokenFromChar(
          VectorStyleExpressionTokenType::OpenSquareBracket,
          position);
    } else if (nextCh == ']') {
      return tokenFromChar(
          VectorStyleExpressionTokenType::CloseSquareBracket,
          position);
    } else if (nextCh == '{') {
      return tokenFromChar(
          VectorStyleExpressionTokenType::OpenCurlyBracket,
          position);
    } else if (nextCh == '}') {
      return tokenFromChar(
          VectorStyleExpressionTokenType::CloseCurlyBracket,
          position);
    } else if (nextCh == '(') {
      return tokenFromChar(VectorStyleExpressionTokenType::OpenParen, position);
    } else if (nextCh == ')') {
      return tokenFromChar(
          VectorStyleExpressionTokenType::CloseParen,
          position);
    } else if (nextCh == '$' && nextCharEquals(position, '{')) {
      // If dollar sign and next char isn't a {, it's probably a variable
      return tokenFromChar(
          VectorStyleExpressionTokenType::TemplateSign,
          position);
    } else if (nextCh == '+') {
      return tokenFromChar(VectorStyleExpressionTokenType::Plus, position);
    } else if (nextCh == '-') {
      return tokenFromChar(VectorStyleExpressionTokenType::Minus, position);
    } else if (nextCh == '/') {
      return tokenFromChar(
          VectorStyleExpressionTokenType::ForwardSlash,
          position);
    } else if (nextCh == '%') {
      return tokenFromChar(VectorStyleExpressionTokenType::Percent, position);
    } else if (nextCh == '?') {
      return tokenFromChar(
          VectorStyleExpressionTokenType::QuestionMark,
          position);
    } else if (nextCh == ':') {
      return tokenFromChar(VectorStyleExpressionTokenType::Colon, position);
    } else if (nextCh == '.') {
      return tokenFromChar(VectorStyleExpressionTokenType::Dot, position);
    }

    // Handle compound or possibly compound tokens
    if (nextCh == '!') {
      if (nextCharEquals(position, '~')) {
        return tokenFromSubstr(
            VectorStyleExpressionTokenType::RegexNeq,
            position,
            2);
      } else if (nextCharEquals(position, '=')) {
        return nextCharEquals(position + 1, '=')
                   ? tokenFromSubstr(
                         VectorStyleExpressionTokenType::NotEquals,
                         position,
                         3)
                   : tokenFromSubstr(
                         VectorStyleExpressionTokenType::NotEquals,
                         position,
                         2);
      }

      return tokenFromChar(
          VectorStyleExpressionTokenType::ExclamationPoint,
          position);
    }

    if (nextCh == '|') {
      if (nextCharEquals(position, '|')) {
        return tokenFromSubstr(VectorStyleExpressionTokenType::Or, position, 2);
      }

      return unexpectedTokenError(position, 2);
    } else if (nextCh == '&') {
      if (nextCharEquals(position, '&')) {
        return tokenFromSubstr(
            VectorStyleExpressionTokenType::And,
            position,
            2);
      }

      return unexpectedTokenError(position, 2);
    } else if (nextCh == '=') {
      if (nextCharEquals(position, '~')) {
        return tokenFromSubstr(
            VectorStyleExpressionTokenType::RegexEq,
            position,
            2);
      } else if (nextCharEquals(position, '=')) {
        return nextCharEquals(position + 1, '=')
                   ? tokenFromSubstr(
                         VectorStyleExpressionTokenType::Equals,
                         position,
                         3)
                   : tokenFromSubstr(
                         VectorStyleExpressionTokenType::Equals,
                         position,
                         2);
      }

      return unexpectedTokenError(position, 2);
    } else if (nextCh == '<') {
      if (nextCharEquals(position, '=')) {
        return tokenFromSubstr(
            VectorStyleExpressionTokenType::LessThanEq,
            position,
            2);
      }

      return tokenFromChar(VectorStyleExpressionTokenType::LessThan, position);
    } else if (nextCh == '>') {
      if (nextCharEquals(position, '=')) {
        return tokenFromSubstr(
            VectorStyleExpressionTokenType::GreaterThanEq,
            position,
            2);
      }

      return tokenFromChar(
          VectorStyleExpressionTokenType::GreaterThan,
          position);
    }

    // Strings, numbers, identifiers
    if (nextCh == '\'' || nextCh == '\"') {
      for (size_t i = position + 1; i < this->_text.length(); i++) {
        if (this->_text[i] == nextCh) {
          size_t len = i - (position + 1);
          return tokenFromSubstr(
              VectorStyleExpressionTokenType::String,
              position,
              len + 1);
        }
      }

      return Result<VectorStyleExpressionToken>(ErrorList::error(
          fmt::format("Unterminated string at position {}", position)));
    } else if (isAsciiDigit(nextCh)) {
      bool foundDecimalPoint = false;
      size_t len = 1;
      for (size_t i = position + 1; i < this->_text.length(); i++) {
        if (this->_text[i] == '.' && !foundDecimalPoint) {
          foundDecimalPoint = true;
        } else if (this->_text[i] == '.' && foundDecimalPoint) {
          return unexpectedTokenError(position, i - position);
        } else if (!isAsciiDigit(this->_text[i])) {
          break;
        }

        len++;
      }

      return tokenFromSubstr(
          VectorStyleExpressionTokenType::Number,
          position,
          len);
    } else if (nextCh == '$' || nextCh == '_' || isAsciiAlpha(nextCh)) {
      size_t len = 1;
      for (size_t i = position + 1; i < this->_text.length(); i++) {
        if (!isValidIdentifierCh(this->_text[i])) {
          break;
        }

        len++;
      }

      return tokenFromSubstr(
          VectorStyleExpressionTokenType::Identifier,
          position,
          len);
    }

    return unexpectedTokenError(position, 1);
  };

  Result<VectorStyleExpressionToken>
  tokenFromChar(VectorStyleExpressionTokenType type, size_t position) {
    return tokenFromSubstr(type, position, 1);
  }

  Result<VectorStyleExpressionToken> tokenFromSubstr(
      VectorStyleExpressionTokenType type,
      size_t position,
      size_t length) {
    return Result<VectorStyleExpressionToken>(VectorStyleExpressionToken{
        type,
        position,
        {this->_text.begin() + (int64_t)position,
         this->_text.begin() + (int64_t)position + (int64_t)length}});
  }

  bool hasNextChar(size_t position) {
    return (position + 1) < this->_text.length();
  }

  bool nextCharEquals(size_t position, char ch) {
    return hasNextChar(position) && this->_text[position + 1] == ch;
  }

  bool isAsciiDigit(char ch) { return ch >= '0' && ch <= '9'; }

  bool isAsciiAlpha(char ch) {
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
  }

  bool isValidIdentifierCh(char ch) {
    return isAsciiAlpha(ch) || isAsciiDigit(ch) || ch == '$' || ch == '_';
  }

  Result<VectorStyleExpressionToken>
  unexpectedTokenError(size_t position, size_t sequenceLen) {
    return Result<VectorStyleExpressionToken>(ErrorList::error(fmt::format(
        "Unexpected sequence '{}' at position {}",
        this->_text.substr(position, sequenceLen),
        position)));
  }

  std::string _text;
};

struct UndefinedPlaceholder {};

struct ArrayPlaceholder;

using VectorStyleExpressionValue = std::variant<
    bool,
    std::nullptr_t,
    UndefinedPlaceholder,
    double,
    std::string,
    ArrayPlaceholder,
    glm::dvec2,
    glm::dvec3,
    glm::dvec4,
    std::regex,
    rapidjson::Pointer>;

struct ArrayPlaceholder {
  std::vector<VectorStyleExpressionValue> values;
};

struct VectorStyleExpressionContext {
  rapidjson::Document feature;
};

namespace {
std::string rapidJsonValueToString(const rapidjson::Value* pValue) {
  if (!pValue) {
    return "undefined";
  } else if (pValue->IsBool()) {
    return pValue->GetBool() ? "true" : "false";
  } else if (pValue->IsDouble()) {
    return std::isnan(pValue->GetDouble())
               ? "NaN"
               : std::to_string(pValue->GetDouble());
  } else if (pValue->IsString()) {
    return pValue->GetString();
  } else if (pValue->IsObject()) {
    return "[object Object]";
  } else if (pValue->IsNull()) {
    return "null";
  } else if (pValue->IsArray()) {
    std::string str;
    for (auto& value : pValue->GetArray()) {
      if (!str.empty()) {
        str += ",";
      }

      str += rapidJsonValueToString(&value);
    }

    return str;
  } else {
    return "";
  }
}
} // namespace

/**
 * @brief Implementing the horrible JavaScript type conversion rules.
 */
class TypeConverter {
public:
  static bool toBoolean(
      const VectorStyleExpressionContext& context,
      const VectorStyleExpressionValue& value) {
    struct Visitor {
      const VectorStyleExpressionContext& context;
      bool operator()(const bool& v) { return v; }
      bool operator()(const std::nullptr_t&) { return false; }
      bool operator()(const UndefinedPlaceholder&) { return false; }
      bool operator()(const double& v) { return !std::isnan(v) && v != 0; }
      bool operator()(const std::string& v) { return !v.empty() && v != "0"; }
      bool operator()(const rapidjson::Pointer& p) {
        const rapidjson::Value* pValue = p.Get(context.feature);
        if (!pValue || pValue->IsFalse() || pValue->IsNull()) {
          return false;
        }

        if (pValue->IsDouble()) {
          const double val = pValue->GetDouble();
          return !std::isnan(val) && val != 0;
        }

        if (pValue->IsString()) {
          return pValue->GetStringLength() != 0 &&
                 !(pValue->GetStringLength() == 1 &&
                   pValue->GetString()[0] == '0');
        }

        return true;
      }
      bool operator()(const ArrayPlaceholder&) { return true; }
      bool operator()(const glm::dvec2&) { return true; }
      bool operator()(const glm::dvec3&) { return true; }
      bool operator()(const glm::dvec4&) { return true; }
      bool operator()(const std::regex&) { return true; }
    };

    return std::visit(Visitor{context}, value);
  }

  static double toNumber(
      const VectorStyleExpressionContext& context,
      const VectorStyleExpressionValue& value) {

    struct Visitor {
      const VectorStyleExpressionContext& context;
      double operator()(const bool& v) { return v ? 1.0 : 0.0; }
      double operator()(const std::nullptr_t&) { return 0.0; }
      double operator()(const UndefinedPlaceholder&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const double& v) { return v; }
      double operator()(const std::string& v) {
        char* pEnd = nullptr;
        double d = std::strtod(v.c_str(), &pEnd);
        if (v.c_str() == pEnd) {
          return std::numeric_limits<double>::quiet_NaN();
        }

        return d;
      }
      double operator()(const rapidjson::Pointer& p) {
        const rapidjson::Value* pValue = p.Get(context.feature);
        if (!pValue) {
          return std::numeric_limits<double>::quiet_NaN();
        }

        if (pValue->IsBool()) {
          return pValue->GetBool() ? 1.0 : 0.0;
        }

        if (pValue->IsDouble()) {
          return pValue->GetDouble();
        }

        if (pValue->IsString()) {
          return TypeConverter::toNumber(context, pValue->GetString());
        }

        if (pValue->IsArray()) {
          if (pValue->GetArray().Size() == 0) {
            return 0.0;
          }

          if (pValue->GetArray().Size() == 1) {
            if (pValue->GetArray()[0].IsNumber()) {
              return pValue->GetArray()[0].GetDouble();
            }
          }
        }

        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const ArrayPlaceholder& arr) {
        if (arr.values.size() == 0) {
          return 0.0;
        }

        if (arr.values.size() == 1) {
          const double* pValue = std::get_if<double>(&arr.values[0]);
          if (pValue) {
            return *pValue;
          }
        }

        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const glm::dvec2&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const glm::dvec3&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const glm::dvec4&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
      double operator()(const std::regex&) {
        return std::numeric_limits<double>::quiet_NaN();
      }
    };

    return std::visit(Visitor{context}, value);
  }

  static std::string toString(
      const VectorStyleExpressionContext& context,
      const VectorStyleExpressionValue& value) {
    struct Visitor {
      const VectorStyleExpressionContext& context;
      std::string operator()(const bool& v) { return v ? "true" : "false"; }
      std::string operator()(const std::nullptr_t&) { return "null"; }
      std::string operator()(const UndefinedPlaceholder&) {
        return "undefined";
      }
      std::string operator()(const double& v) {
        return std::isnan(v) ? "NaN" : std::to_string(v);
      }
      std::string operator()(const std::string& v) { return v; }
      std::string operator()(const rapidjson::Pointer& p) {
        return rapidJsonValueToString(p.Get(context.feature));
      }
      std::string operator()(const ArrayPlaceholder& arr) {
        std::string str;
        for (const VectorStyleExpressionValue& v : arr.values) {
          if (!str.empty()) {
            str += ",";
          }

          str += TypeConverter::toString(context, v);
        }
        return str;
      }
      std::string operator()(const glm::dvec2& v) {
        return fmt::format("({}, {})", v.x, v.y);
      }
      std::string operator()(const glm::dvec3& v) {
        return fmt::format("({}, {}, {})", v.x, v.y, v.z);
      }
      std::string operator()(const glm::dvec4& v) {
        return fmt::format("({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      }
      std::string operator()(const std::regex&) { return "RegExp"; }
    };

    return std::visit(Visitor{context}, value);
  }
};

struct VectorStyleExpressionASTNode {
  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) = 0;
};

struct ConstantNode : public VectorStyleExpressionASTNode {
  VectorStyleExpressionValue value;
  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& /*context*/) {
    return Result<VectorStyleExpressionValue>{this->value};
  }
};

enum class UnaryOperatorType : uint8_t { Plus, Minus, Not };

struct UnaryNode : public VectorStyleExpressionASTNode {
  std::unique_ptr<VectorStyleExpressionASTNode> operand;
  UnaryOperatorType type;

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) {
    Result<VectorStyleExpressionValue> operandResult =
        operand->execute(context);
    if (!operandResult.value) {
      return Result<VectorStyleExpressionValue>(operandResult.errors);
    }

    switch (type) {
    case UnaryOperatorType::Plus:
      return Result<VectorStyleExpressionValue>(
          TypeConverter::toNumber(context, *operandResult.value));
    case UnaryOperatorType::Minus:
      return Result<VectorStyleExpressionValue>(
          -TypeConverter::toNumber(context, *operandResult.value));
    case UnaryOperatorType::Not:
      return Result<VectorStyleExpressionValue>(
          !TypeConverter::toBoolean(context, *operandResult.value));
    }

    return Result<VectorStyleExpressionValue>(
        ErrorList::error("Unknown unary operator type"));
  }
};

enum class BinaryBooleanOperatorType : uint8_t { Or, And };
struct BinaryBooleanOperatorNode : public VectorStyleExpressionASTNode {
  std::unique_ptr<VectorStyleExpressionASTNode> operand1;
  std::unique_ptr<VectorStyleExpressionASTNode> operand2;
  BinaryBooleanOperatorType type;

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) {
    Result<VectorStyleExpressionValue> operand1Result =
        operand1->execute(context);
    if (!operand1Result.value) {
      return Result<VectorStyleExpressionValue>(operand1Result.errors);
    }

    bool operand1Bool =
        TypeConverter::toBoolean(context, *operand1Result.value);

    // Short circuiting
    if ((type == BinaryBooleanOperatorType::Or && operand1Bool) ||
        (type == BinaryBooleanOperatorType::And && !operand1Bool)) {
      return Result<VectorStyleExpressionValue>{operand1Bool};
    }

    Result<VectorStyleExpressionValue> operand2Result =
        operand2->execute(context);
    if (!operand2Result.value) {
      return Result<VectorStyleExpressionValue>(operand2Result.errors);
    }

    bool operand2Bool =
        TypeConverter::toBoolean(context, *operand2Result.value);

    return Result<VectorStyleExpressionValue>(
        type == BinaryBooleanOperatorType::Or ? (operand1Bool || operand2Bool)
                                              : (operand1Bool && operand2Bool));
  }
};

enum class BinaryBooleanComparisonOperatorType : uint8_t { Equals, NotEquals };

struct BinaryBooleanComparisonOperatorNode
    : public VectorStyleExpressionASTNode {
  std::unique_ptr<VectorStyleExpressionASTNode> operand1;
  std::unique_ptr<VectorStyleExpressionASTNode> operand2;
  BinaryBooleanComparisonOperatorType type;

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) {
    Result<VectorStyleExpressionValue> operand1Result =
        operand1->execute(context);
    if (!operand1Result.value) {
      return Result<VectorStyleExpressionValue>(operand1Result.errors);
    }

    Result<VectorStyleExpressionValue> operand2Result =
        operand2->execute(context);
    if (!operand2Result.value) {
      return Result<VectorStyleExpressionValue>(operand2Result.errors);
    }

    if (operand1Result.value->index() != operand2Result.value->index()) {
      // Different types, they can't be equal since these are === and !==
      return Result<VectorStyleExpressionValue>(
          type == BinaryBooleanComparisonOperatorType::Equals ? false : true);
    }
  }
};