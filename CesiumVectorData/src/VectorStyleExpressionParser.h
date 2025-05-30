#pragma once

#include "CesiumUtility/Assert.h"
#include "CesiumUtility/ErrorList.h"
#include "VectorStyleExpression.h"
#include "VectorStyleExpressionNodes.h"
#include "VectorStyleExpressionTokenizer.h"

#include <fmt/format.h>

#include <functional>
#include <memory>

namespace CesiumVectorData {
namespace {
uint8_t
getOperatorPrecedence(VectorStyleExpressionTokenType token, bool isBinary) {
  // Rules adapted from
  // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Operator_precedence
  switch (token) {
  case VectorStyleExpressionTokenType::Plus:
  case VectorStyleExpressionTokenType::Minus:
    return isBinary ? 11 : 14;
  case VectorStyleExpressionTokenType::ForwardSlash:
  case VectorStyleExpressionTokenType::Times:
  case VectorStyleExpressionTokenType::Percent:
    return 12;
  case VectorStyleExpressionTokenType::ExclamationPoint:
    return 14;
  case VectorStyleExpressionTokenType::Or:
    return 3;
  case VectorStyleExpressionTokenType::And:
    return 4;
  case VectorStyleExpressionTokenType::Equals:
  case VectorStyleExpressionTokenType::NotEquals:
  case VectorStyleExpressionTokenType::RegexEq:
  case VectorStyleExpressionTokenType::RegexNeq:
    return 8;
  case VectorStyleExpressionTokenType::GreaterThan:
  case VectorStyleExpressionTokenType::LessThan:
  case VectorStyleExpressionTokenType::GreaterThanEq:
  case VectorStyleExpressionTokenType::LessThanEq:
    return 9;
  case VectorStyleExpressionTokenType::QuestionMark:
    return 2;
  default:
    return 1;
  }
}

struct FoundOperator {
  VectorStyleExpressionTokenType type;
  bool isBinary;
};
} // namespace

class VectorStyleExpressionParser {
public:
  VectorStyleExpressionParser(std::vector<VectorStyleExpressionToken>&& tokens)
      : _tokens(std::move(tokens)) {}

private:
  Result<std::unique_ptr<VectorStyleExpressionASTNode>> parseSubsequence(
      size_t startPos,
      const std::function<bool(VectorStyleExpressionTokenType type)>& until) {
    std::stack<std::unique_ptr<VectorStyleExpressionASTNode>> operandStack;
    std::stack<FoundOperator> operatorStack;
    bool expectsOperator = false;
    while (startPos < this->_tokens.size() && !until(_tokens[startPos].type)) {
      const VectorStyleExpressionTokenType type = _tokens[startPos].type;
      const std::string_view& value = _tokens[startPos].value;
      const size_t position = _tokens[startPos].position;

      // Handle constants
      if (type == VectorStyleExpressionTokenType::Identifier &&
          (value == "true" || value == "false")) {
        operandStack.emplace(
            std::make_unique<ConstantNode>(position, value == "true"));
        expectsOperator = true;
        continue;
      } else if (
          type == VectorStyleExpressionTokenType::Identifier &&
          value == "null") {
        operandStack.emplace(std::make_unique<ConstantNode>(position, nullptr));
        expectsOperator = true;
        continue;
      } else if (
          type == VectorStyleExpressionTokenType::Identifier &&
          value == "undefined") {
        operandStack.emplace(
            std::make_unique<ConstantNode>(position, UndefinedPlaceholder{}));
        expectsOperator = true;
        continue;
      } else if (type == VectorStyleExpressionTokenType::Number) {
        double result;
        std::from_chars(value.data(), value.data() + value.size(), result);
        operandStack.emplace(std::make_unique<ConstantNode>(position, result));
        expectsOperator = true;
        continue;
      } else if (type == VectorStyleExpressionTokenType::String) {
        operandStack.emplace(
            std::make_unique<ConstantNode>(position, std::string(value)));
        expectsOperator = true;
        continue;
      }
      // todo: arrays and function-initialized types (vecN, regexp)

      std::optional<ErrorList> errorsOpt = this->pushOperator(operandStack, operatorStack, FoundOperator { type, expectsOperator });
      if(errorsOpt) {
        return Result<std::unique_ptr<VectorStyleExpressionASTNode>>(*errorsOpt);
      }
      expectsOperator = false;
    }

    std::optional<ErrorList> makeOperatorErr =
        this->makeOperatorNode(operandStack, operatorStack.top());
    if (makeOperatorErr) {
      return Result<std::unique_ptr<VectorStyleExpressionASTNode>>(*makeOperatorErr);
    }
    operatorStack.pop();

    CESIUM_ASSERT(operandStack.size() == 1);
    CESIUM_ASSERT(operatorStack.empty());

    return std::move(operandStack.top());
  }

  std::optional<ErrorList> pushOperator(
      std::stack<std::unique_ptr<VectorStyleExpressionASTNode>>& operandStack,
      std::stack<FoundOperator>& operatorStack,
      FoundOperator&& op) {
    if (operatorStack.empty()) {
      operatorStack.push(std::move(op));
      return std::nullopt;
    }

    const uint8_t precedence = getOperatorPrecedence(op.type, op.isBinary);
    const uint8_t existingPrecedence = getOperatorPrecedence(
        operatorStack.top().type,
        operatorStack.top().isBinary);

    if (precedence > existingPrecedence) {
      operatorStack.push(std::move(op));
      return std::nullopt;
    } else {
      std::optional<ErrorList> makeOperatorErr =
          this->makeOperatorNode(operandStack, operatorStack.top());
      if (makeOperatorErr) {
        return makeOperatorErr;
      }
      operatorStack.pop();
      return this->pushOperator(operandStack, operatorStack, std::move(op));
    }
  }

  std::optional<ErrorList> makeOperatorNode(
      std::stack<std::unique_ptr<VectorStyleExpressionASTNode>>& operandStack,
      const FoundOperator& op) {
    if (!op.isBinary) {
      if (op.type != VectorStyleExpressionTokenType::Plus &&
          op.type != VectorStyleExpressionTokenType::Minus &&
          op.type != VectorStyleExpressionTokenType::ExclamationPoint) {
        return ErrorList::error(
            fmt::format("unexpected token {}", tokenToString(op.type)));
      }

      if (operandStack.empty()) {
        return ErrorList::error(fmt::format(
            "missing operands for unary operator '{}'",
            tokenToString(op.type)));
      }

      std::unique_ptr<VectorStyleExpressionASTNode> operand2 =
          std::move(operandStack.top());
      operandStack.pop();

      operandStack.push(std::make_unique<UnaryNode>(
          operand2->sourceIndex,
          std::move(operand2),
          op.type == VectorStyleExpressionTokenType::Plus
              ? UnaryOperatorType::Plus
              : (op.type == VectorStyleExpressionTokenType::Minus
                     ? UnaryOperatorType::Minus
                     : UnaryOperatorType::Not)));
      return std::nullopt;
    }

    // todo: ternary handling

    switch (op.type) {
    case VectorStyleExpressionTokenType::Plus:
    case VectorStyleExpressionTokenType::Minus:
    case VectorStyleExpressionTokenType::ForwardSlash:
    case VectorStyleExpressionTokenType::Times:
    case VectorStyleExpressionTokenType::Percent:
    case VectorStyleExpressionTokenType::Or:
    case VectorStyleExpressionTokenType::And:
    case VectorStyleExpressionTokenType::Equals:
    case VectorStyleExpressionTokenType::NotEquals:
    case VectorStyleExpressionTokenType::RegexEq:
    case VectorStyleExpressionTokenType::RegexNeq:
    case VectorStyleExpressionTokenType::GreaterThan:
    case VectorStyleExpressionTokenType::LessThan:
    case VectorStyleExpressionTokenType::GreaterThanEq:
    case VectorStyleExpressionTokenType::LessThanEq:
      if (operandStack.size() < 2) {
        return ErrorList::error(fmt::format(
            "missing operands for binary operator '{}'",
            tokenToString(op.type)));
      }
      std::unique_ptr<VectorStyleExpressionASTNode> operand2 =
          std::move(operandStack.top());
      operandStack.pop();

      std::unique_ptr<VectorStyleExpressionASTNode> operand1 =
          std::move(operandStack.top());
      operandStack.pop();

      if (op.type == VectorStyleExpressionTokenType::Plus) {
        operandStack.push(std::make_unique<BinaryAdditionOperatorNode>(
            operand1->sourceIndex,
            std::move(operand1),
            std::move(operand2)));
      } else if (op.type == VectorStyleExpressionTokenType::Minus) {
        operandStack.push(std::make_unique<BinarySubtractionOperatorNode>(
            operand1->sourceIndex,
            std::move(operand1),
            std::move(operand2)));
      } else if (op.type == VectorStyleExpressionTokenType::ForwardSlash) {
        operandStack.push(std::make_unique<BinaryDivideOperatorNode>(
            operand1->sourceIndex,
            std::move(operand1),
            std::move(operand2)));
      } else if (op.type == VectorStyleExpressionTokenType::Times) {
        operandStack.push(std::make_unique<BinaryMultiplyOperatorNode>(
            operand1->sourceIndex,
            std::move(operand1),
            std::move(operand2)));
      } else if (op.type == VectorStyleExpressionTokenType::Percent) {
        operandStack.push(std::make_unique<BinaryModuloOperatorNode>(
            operand1->sourceIndex,
            std::move(operand1),
            std::move(operand2)));
      } else if (op.type == VectorStyleExpressionTokenType::Or) {
        operandStack.push(std::make_unique<BinaryBooleanOperatorNode>(
            operand1->sourceIndex,
            BinaryBooleanOperatorType::Or,
            std::move(operand1),
            std::move(operand2)));
      } else if (op.type == VectorStyleExpressionTokenType::And) {
        operandStack.push(std::make_unique<BinaryBooleanOperatorNode>(
            operand1->sourceIndex,
            BinaryBooleanOperatorType::And,
            std::move(operand1),
            std::move(operand2)));
      }
    }
  }

  std::vector<VectorStyleExpressionToken> _tokens;
};
} // namespace CesiumVectorData