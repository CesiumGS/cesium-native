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
    std::regex>;

struct ArrayPlaceholder {
  std::vector<VectorStyleExpressionValue> values;
};

struct VectorStyleExpressionContext {
  rapidjson::Document feature;
};

namespace {
VectorStyleExpressionValue
expressionValueFromRapidjsonValue(const rapidjson::Value* pValue) {
  if (!pValue) {
    return UndefinedPlaceholder{};
  }

  if (pValue->IsDouble()) {
    return pValue->GetDouble();
  } else if (pValue->IsString()) {
    return pValue->GetString();
  } else if (pValue->IsBool()) {
    return pValue->GetBool();
  } else if (pValue->IsArray()) {
    ArrayPlaceholder array;
    array.values.reserve(pValue->GetArray().Size());

    for (const rapidjson::Value& value : pValue->GetArray()) {
      array.values.emplace_back(expressionValueFromRapidjsonValue(&value));
    }

    return array;
  } else {
    return nullptr;
  }
}

std::string getValueTypeName(const VectorStyleExpressionValue& value) {
  struct Visitor {
    std::string operator()(const bool&) { return "Boolean"; }
    std::string operator()(const std::nullptr_t&) { return "null"; }
    std::string operator()(const UndefinedPlaceholder&) { return "undefined"; }
    std::string operator()(const double&) { return "Number"; }
    std::string operator()(const std::string&) { return "String"; }
    std::string operator()(const ArrayPlaceholder&) { return "Array"; }
    std::string operator()(const glm::dvec2&) { return "vec2"; }
    std::string operator()(const glm::dvec3&) { return "vec3"; }
    std::string operator()(const glm::dvec4&) { return "vec4"; }
    std::string operator()(const std::regex&) { return "RegExp"; }
  };

  return std::visit(Visitor{}, value);
}
} // namespace

/**
 * @brief Implementing the horrible JavaScript type conversion rules.
 */
class TypeConverter {
public:
  static bool toBoolean(const VectorStyleExpressionValue& value) {
    struct Visitor {
      bool operator()(const bool& v) { return v; }
      bool operator()(const std::nullptr_t&) { return false; }
      bool operator()(const UndefinedPlaceholder&) { return false; }
      bool operator()(const double& v) { return !std::isnan(v) && v != 0; }
      bool operator()(const std::string& v) { return !v.empty() && v != "0"; }
      bool operator()(const ArrayPlaceholder&) { return true; }
      bool operator()(const glm::dvec2&) { return true; }
      bool operator()(const glm::dvec3&) { return true; }
      bool operator()(const glm::dvec4&) { return true; }
      bool operator()(const std::regex&) { return true; }
    };

    return std::visit(Visitor{}, value);
  }

  static double toNumber(const VectorStyleExpressionValue& value) {

    struct Visitor {
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

    return std::visit(Visitor{}, value);
  }

  static std::string toString(const VectorStyleExpressionValue& value) {
    struct Visitor {
      std::string operator()(const bool& v) { return v ? "true" : "false"; }
      std::string operator()(const std::nullptr_t&) { return "null"; }
      std::string operator()(const UndefinedPlaceholder&) {
        return "undefined";
      }
      std::string operator()(const double& v) {
        if (std::isinf(v)) {
          return "Infinity";
        } else if (std::isnan(v)) {
          return "NaN";
        }

        return std::to_string(v);
      }
      std::string operator()(const std::string& v) { return v; }
      std::string operator()(const ArrayPlaceholder& arr) {
        std::string str;
        for (const VectorStyleExpressionValue& v : arr.values) {
          if (!str.empty()) {
            str += ",";
          }

          str += TypeConverter::toString(v);
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

    return std::visit(Visitor{}, value);
  }

  static bool areExactlyEqual(
      const VectorStyleExpressionValue& value1,
      const VectorStyleExpressionValue& value2) {
    if (value1.index() != value2.index()) {
      // Different types, they can't be exactly equal
      return false;
    }

    struct Visitor {
      const VectorStyleExpressionValue& value2;
      bool operator()(const bool& v) { return v == std::get<bool>(value2); }
      bool operator()(const std::nullptr_t&) { return true; }
      bool operator()(const UndefinedPlaceholder&) { return true; }
      bool operator()(const double& v) { return v == std::get<double>(value2); }
      bool operator()(const std::string& v) {
        return v == std::get<std::string>(value2);
      }
      bool operator()(const ArrayPlaceholder&) { return false; }
      bool operator()(const glm::dvec2& v) {
        const glm::dvec2 v2 = std::get<glm::dvec2>(value2);
        return v.x == v2.x && v.y == v2.y;
      }
      bool operator()(const glm::dvec3& v) {
        const glm::dvec3 v2 = std::get<glm::dvec3>(value2);
        return v.x == v2.x && v.y == v2.y && v.z == v2.z;
      }
      bool operator()(const glm::dvec4& v) {
        const glm::dvec4 v2 = std::get<glm::dvec4>(value2);
        return v.x == v2.x && v.y == v2.y && v.z == v2.z && v.w == v2.w;
      }
      bool operator()(const std::regex&) { return false; }
    };

    return std::visit(Visitor{value2}, value1);
  }
};

struct VectorStyleExpressionASTNode {
  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const = 0;
};

struct ConstantNode : public VectorStyleExpressionASTNode {
  VectorStyleExpressionValue value;
  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& /*context*/) const {
    return Result<VectorStyleExpressionValue>{this->value};
  }
};

struct VariableNode : public VectorStyleExpressionASTNode {
  rapidjson::Pointer variablePointer;
  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const {
    const rapidjson::Value* pValue = variablePointer.Get(context.feature);
    return Result<VectorStyleExpressionValue>(
        expressionValueFromRapidjsonValue(pValue));
  }
};

enum class UnaryOperatorType : uint8_t { Plus, Minus, Not };

struct UnaryNode : public VectorStyleExpressionASTNode {
  std::unique_ptr<VectorStyleExpressionASTNode> operand;
  UnaryOperatorType type;

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const {
    Result<VectorStyleExpressionValue> operandResult =
        operand->execute(context);
    if (!operandResult.value) {
      return Result<VectorStyleExpressionValue>(operandResult.errors);
    }

    switch (type) {
    case UnaryOperatorType::Plus:
      return Result<VectorStyleExpressionValue>(
          TypeConverter::toNumber(*operandResult.value));
    case UnaryOperatorType::Minus:
      return Result<VectorStyleExpressionValue>(
          -TypeConverter::toNumber(*operandResult.value));
    case UnaryOperatorType::Not:
      return Result<VectorStyleExpressionValue>(
          !TypeConverter::toBoolean(*operandResult.value));
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
  execute(VectorStyleExpressionContext& context) const {
    Result<VectorStyleExpressionValue> operand1Result =
        operand1->execute(context);
    if (!operand1Result.value) {
      return Result<VectorStyleExpressionValue>(operand1Result.errors);
    }

    bool operand1Bool = TypeConverter::toBoolean(*operand1Result.value);

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

    bool operand2Bool = TypeConverter::toBoolean(*operand2Result.value);

    return Result<VectorStyleExpressionValue>(
        type == BinaryBooleanOperatorType::Or ? (operand1Bool || operand2Bool)
                                              : (operand1Bool && operand2Bool));
  }
};

struct BinaryOperatorNode : public VectorStyleExpressionASTNode {
  std::unique_ptr<VectorStyleExpressionASTNode> operand1;
  std::unique_ptr<VectorStyleExpressionASTNode> operand2;

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const = 0;

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const {
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

    return this->executeWithOperands(
        *operand1Result.value,
        *operand2Result.value);
  }
};

enum class BinaryBooleanComparisonOperatorType : uint8_t { Equals, NotEquals };

struct BinaryBooleanComparisonOperatorNode : public BinaryOperatorNode {
  BinaryBooleanComparisonOperatorType type;

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const {
    return Result<VectorStyleExpressionValue>(
        type == BinaryBooleanComparisonOperatorType::Equals
            ? TypeConverter::areExactlyEqual(value1, value2)
            : !TypeConverter::areExactlyEqual(value1, value2));
  }
};

enum class BinaryNumberComparisonOperatorType {
  LessThan,
  LessThanEqual,
  GreaterThan,
  GreaterThanEqual
};

struct BinaryNumberComparisonOperatorNode : public BinaryOperatorNode {
  BinaryNumberComparisonOperatorType type;

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const {
    const double* n1 = std::get_if<double>(&value1);
    const double* n2 = std::get_if<double>(&value2);

    if (!n1 || !n2) {
      return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
          "Binary number comparison operator expects types to be numbers, used "
          "on types {} and {}",
          getValueTypeName(value1),
          getValueTypeName(value2))));
    }

    switch (type) {
    case BinaryNumberComparisonOperatorType::LessThan:
      return Result<VectorStyleExpressionValue>(*n1 < *n2);
    case BinaryNumberComparisonOperatorType::LessThanEqual:
      return Result<VectorStyleExpressionValue>(*n1 <= *n2);
    case BinaryNumberComparisonOperatorType::GreaterThan:
      return Result<VectorStyleExpressionValue>(*n1 > *n2);
    case BinaryNumberComparisonOperatorType::GreaterThanEqual:
      return Result<VectorStyleExpressionValue>(*n1 >= *n2);
    }

    return Result<VectorStyleExpressionValue>(false);
  }
};

struct BinaryAdditionOperatorNode : public BinaryOperatorNode {
  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const {
    const std::string* pStrValue = std::get_if<std::string>(&value1);
    if (pStrValue) {
      return Result<VectorStyleExpressionValue>(
          *pStrValue + TypeConverter::toString(value2));
    }

    if (value1.index() != value2.index()) {
      return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
          "Binary `+` operator expects operands of matching types, got {} and "
          "{}.",
          getValueTypeName(value1),
          getValueTypeName(value2))));
    }

    if (std::holds_alternative<double>(value1)) {
      return Result<VectorStyleExpressionValue>(
          std::get<double>(value1) + std::get<double>(value2));
    } else if (std::holds_alternative<glm::dvec2>(value1)) {
      return Result<VectorStyleExpressionValue>(
          std::get<glm::dvec2>(value1) + std::get<glm::dvec2>(value2));
    } else if (std::holds_alternative<glm::dvec3>(value1)) {
      return Result<VectorStyleExpressionValue>(
          std::get<glm::dvec3>(value1) + std::get<glm::dvec3>(value2));
    } else if (std::holds_alternative<glm::dvec4>(value1)) {
      return Result<VectorStyleExpressionValue>(
          std::get<glm::dvec4>(value1) + std::get<glm::dvec4>(value2));
    }

    return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
        "Binary `+` operator can't operator on types {} and {}.",
        getValueTypeName(value1),
        getValueTypeName(value2))));
  }
};

struct BinarySubtractionOperatorNode : public BinaryOperatorNode {
  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const {
    if (value1.index() != value2.index()) {
      return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
          "Binary `-` operator expects operands of matching types, got {} and "
          "{}.",
          getValueTypeName(value1),
          getValueTypeName(value2))));
    }

    if (std::holds_alternative<double>(value1)) {
      return Result<VectorStyleExpressionValue>(
          std::get<double>(value1) - std::get<double>(value2));
    } else if (std::holds_alternative<glm::dvec2>(value1)) {
      return Result<VectorStyleExpressionValue>(
          std::get<glm::dvec2>(value1) - std::get<glm::dvec2>(value2));
    } else if (std::holds_alternative<glm::dvec3>(value1)) {
      return Result<VectorStyleExpressionValue>(
          std::get<glm::dvec3>(value1) - std::get<glm::dvec3>(value2));
    } else if (std::holds_alternative<glm::dvec4>(value1)) {
      return Result<VectorStyleExpressionValue>(
          std::get<glm::dvec4>(value1) - std::get<glm::dvec4>(value2));
    }

    return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
        "Binary `-` operator can't operator on types {} and {}.",
        getValueTypeName(value1),
        getValueTypeName(value2))));
  }
};