#include "VectorStyleExpressionNodes.h"

#include <cctype>
#include <cmath>
#include <cstddef>
#include <regex>
#include <string>
#include <variant>

namespace CesiumVectorData {
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

bool isVectorValue(VectorStyleExpressionValue& v) {
  return std::holds_alternative<glm::dvec2>(v) ||
         std::holds_alternative<glm::dvec3>(v) ||
         std::holds_alternative<glm::dvec4>(v);
}
} // namespace

CesiumUtility::Result<VectorStyleExpressionValue>
ConstantNode::execute(VectorStyleExpressionContext& /*context*/) const {
  return Result<VectorStyleExpressionValue>{this->value};
}

CesiumUtility::Result<VectorStyleExpressionValue>
VariableNode::execute(VectorStyleExpressionContext& context) const {
  const rapidjson::Value* pValue = variablePointer.Get(context.feature);
  return Result<VectorStyleExpressionValue>(
      expressionValueFromRapidjsonValue(pValue));
}

CesiumUtility::Result<VectorStyleExpressionValue>
UnaryNode::execute(VectorStyleExpressionContext& context) const {
  Result<VectorStyleExpressionValue> operandResult = operand->execute(context);
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

CesiumUtility::Result<VectorStyleExpressionValue>
BinaryBooleanOperatorNode::execute(
    VectorStyleExpressionContext& context) const {
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

CesiumUtility::Result<VectorStyleExpressionValue>
BinaryOperatorNode::execute(VectorStyleExpressionContext& context) const {
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

CesiumUtility::Result<VectorStyleExpressionValue>
BinaryBooleanComparisonOperatorNode::executeWithOperands(
    VectorStyleExpressionValue& value1,
    VectorStyleExpressionValue& value2) const {
  return Result<VectorStyleExpressionValue>(
      type == BinaryBooleanComparisonOperatorType::Equals
          ? TypeConverter::areExactlyEqual(value1, value2)
          : !TypeConverter::areExactlyEqual(value1, value2));
}

CesiumUtility::Result<VectorStyleExpressionValue>
BinaryNumberComparisonOperatorNode::executeWithOperands(
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

CesiumUtility::Result<VectorStyleExpressionValue>
BinaryAdditionOperatorNode::executeWithOperands(
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
      "Binary `+` operator can't operate on types {} and {}.",
      getValueTypeName(value1),
      getValueTypeName(value2))));
}

CesiumUtility::Result<VectorStyleExpressionValue>
BinarySubtractionOperatorNode::executeWithOperands(
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
      "Binary `-` operator can't operate on types {} and {}.",
      getValueTypeName(value1),
      getValueTypeName(value2))));
}

CesiumUtility::Result<VectorStyleExpressionValue>
BinaryMultiplyOperatorNode::executeWithOperands(
    VectorStyleExpressionValue& value1,
    VectorStyleExpressionValue& value2) const {
  if (std::holds_alternative<double>(value1) && isVectorValue(value2)) {
    return this->applyVectorDouble(std::get<double>(value1), value2);
  } else if (isVectorValue(value1) && std::holds_alternative<double>(value2)) {
    return this->applyVectorDouble(std::get<double>(value2), value1);
  }

  if (value1.index() != value2.index()) {
    return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
        "Binary `*` operator expects operands of matching types, got {} and "
        "{}.",
        getValueTypeName(value1),
        getValueTypeName(value2))));
  }

  if (std::holds_alternative<double>(value1)) {
    return Result<VectorStyleExpressionValue>(
        std::get<double>(value1) * std::get<double>(value2));
  } else if (std::holds_alternative<glm::dvec2>(value1)) {
    return Result<VectorStyleExpressionValue>(
        std::get<glm::dvec2>(value1) * std::get<glm::dvec2>(value2));
  } else if (std::holds_alternative<glm::dvec3>(value1)) {
    return Result<VectorStyleExpressionValue>(
        std::get<glm::dvec3>(value1) * std::get<glm::dvec3>(value2));
  } else if (std::holds_alternative<glm::dvec4>(value1)) {
    return Result<VectorStyleExpressionValue>(
        std::get<glm::dvec4>(value1) * std::get<glm::dvec4>(value2));
  }

  return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
      "Binary `*` operator can't operate on types {} and {}.",
      getValueTypeName(value1),
      getValueTypeName(value2))));
}

VectorStyleExpressionValue BinaryMultiplyOperatorNode::applyVectorDouble(
    double d,
    VectorStyleExpressionValue vec) const {
  if (std::holds_alternative<glm::dvec2>(vec)) {
    return d * std::get<glm::dvec2>(vec);
  } else if (std::holds_alternative<glm::dvec3>(vec)) {
    return d * std::get<glm::dvec3>(vec);
  } else if (std::holds_alternative<glm::dvec4>(vec)) {
    return d * std::get<glm::dvec4>(vec);
  } else {
    return UndefinedPlaceholder{};
  }
}

CesiumUtility::Result<VectorStyleExpressionValue>
BinaryDivideOperatorNode::executeWithOperands(
    VectorStyleExpressionValue& value1,
    VectorStyleExpressionValue& value2) const {
  if (isVectorValue(value1) && std::holds_alternative<double>(value2)) {
    return this->applyVectorDouble(std::get<double>(value2), value1);
  }

  if (value1.index() != value2.index()) {
    return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
        "Binary `/` operator expects operands of matching types, got {} and "
        "{}.",
        getValueTypeName(value1),
        getValueTypeName(value2))));
  }

  if (std::holds_alternative<double>(value1)) {
    return Result<VectorStyleExpressionValue>(
        std::get<double>(value1) / std::get<double>(value2));
  } else if (std::holds_alternative<glm::dvec2>(value1)) {
    return Result<VectorStyleExpressionValue>(
        std::get<glm::dvec2>(value1) / std::get<glm::dvec2>(value2));
  } else if (std::holds_alternative<glm::dvec3>(value1)) {
    return Result<VectorStyleExpressionValue>(
        std::get<glm::dvec3>(value1) / std::get<glm::dvec3>(value2));
  } else if (std::holds_alternative<glm::dvec4>(value1)) {
    return Result<VectorStyleExpressionValue>(
        std::get<glm::dvec4>(value1) / std::get<glm::dvec4>(value2));
  }

  return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
      "Binary `/` operator can't operate on types {} and {}.",
      getValueTypeName(value1),
      getValueTypeName(value2))));
}

VectorStyleExpressionValue BinaryDivideOperatorNode::applyVectorDouble(
    double d,
    VectorStyleExpressionValue vec) const {
  if (std::holds_alternative<glm::dvec2>(vec)) {
    return std::get<glm::dvec2>(vec) / d;
  } else if (std::holds_alternative<glm::dvec3>(vec)) {
    return std::get<glm::dvec3>(vec) / d;
  } else if (std::holds_alternative<glm::dvec4>(vec)) {
    return std::get<glm::dvec4>(vec) / d;
  } else {
    return UndefinedPlaceholder{};
  }
}

CesiumUtility::Result<VectorStyleExpressionValue>
BinaryModuloOperatorNode::executeWithOperands(
    VectorStyleExpressionValue& value1,
    VectorStyleExpressionValue& value2) const {
  if (value1.index() != value2.index()) {
    return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
        "Binary `%` operator expects operands of matching types, got {} and "
        "{}.",
        getValueTypeName(value1),
        getValueTypeName(value2))));
  }

  if (std::holds_alternative<double>(value1)) {
    return Result<VectorStyleExpressionValue>(
        std::fmod(std::get<double>(value1), std::get<double>(value2)));
  } else if (std::holds_alternative<glm::dvec2>(value1)) {
    return Result<VectorStyleExpressionValue>(
        glm::modf(std::get<glm::dvec2>(value1), std::get<glm::dvec2>(value2)));
  } else if (std::holds_alternative<glm::dvec3>(value1)) {
    return Result<VectorStyleExpressionValue>(
        glm::modf(std::get<glm::dvec3>(value1), std::get<glm::dvec3>(value2)));
  } else if (std::holds_alternative<glm::dvec4>(value1)) {
    return Result<VectorStyleExpressionValue>(
        glm::modf(std::get<glm::dvec4>(value1), std::get<glm::dvec4>(value2)));
  }

  return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
      "Binary `%` operator can't operate on types {} and {}.",
      getValueTypeName(value1),
      getValueTypeName(value2))));
}

CesiumUtility::Result<VectorStyleExpressionValue>
BinaryRegexOperatorNode::executeWithOperands(
    VectorStyleExpressionValue& value1,
    VectorStyleExpressionValue& value2) const {
  if (std::holds_alternative<std::string>(value1) &&
      std::holds_alternative<std::regex>(value2)) {
    const bool result = this->handleMatch(
        std::get<std::string>(value1),
        std::get<std::regex>(value2));
    return Result<VectorStyleExpressionValue>(
        this->type == RegexOperatorType::Match ? result : !result);
  } else if (
      std::holds_alternative<std::string>(value2) &&
      std::holds_alternative<std::regex>(value1)) {
    const bool result = this->handleMatch(
        std::get<std::string>(value2),
        std::get<std::regex>(value1));

    return Result<VectorStyleExpressionValue>(
        this->type == RegexOperatorType::Match ? result : !result);
  }

  return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
      "Binary `{}` operator can't operate on types {} and {}.",
      this->type == RegexOperatorType::Match ? "=~" : "~!",
      getValueTypeName(value1),
      getValueTypeName(value2))));
}

bool BinaryRegexOperatorNode::handleMatch(
    const std::string& str,
    const std::regex& regex) const {
  return std::regex_match(str, regex, std::regex_constants::match_default);
}

CesiumUtility::Result<VectorStyleExpressionValue>
TernaryOperatorNode::execute(VectorStyleExpressionContext& context) const {
  Result<VectorStyleExpressionValue> condResult = conditional->execute(context);
  if (!condResult.value) {
    return condResult;
  }

  const bool* pResult = std::get_if<bool>(&*condResult.value);
  if (!pResult) {
    return Result<VectorStyleExpressionValue>(ErrorList::error(fmt::format(
        "Expected boolean result from conditional expression of ternary "
        "operator, found {}",
        getValueTypeName(*condResult.value))));
  }

  return *pResult ? branch1->execute(context) : branch2->execute(context);
}

} // namespace CesiumVectorData