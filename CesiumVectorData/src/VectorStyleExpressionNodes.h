#pragma once

#include "VectorStyleExpression.h"

#include <CesiumUtility/Result.h>

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <regex>
#include <string>

namespace CesiumVectorData {

struct VectorStyleExpressionASTNode {
  size_t sourceIndex = 0;

  VectorStyleExpressionASTNode(size_t sourceIndex_)
      : sourceIndex(sourceIndex_) {}

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const = 0;
};

struct ConstantNode : public VectorStyleExpressionASTNode {
  VectorStyleExpressionValue value;

  ConstantNode(size_t sourceIndex_, VectorStyleExpressionValue&& value_)
      : VectorStyleExpressionASTNode(sourceIndex_), value(std::move(value_)) {}

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& /*context*/) const;
};

struct VariableNode : public VectorStyleExpressionASTNode {
  rapidjson::Pointer variablePointer;
  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const;
};

enum class UnaryOperatorType : uint8_t { Plus, Minus, Not };

struct UnaryNode : public VectorStyleExpressionASTNode {
  std::unique_ptr<VectorStyleExpressionASTNode> operand;
  UnaryOperatorType type;

  UnaryNode(
      size_t sourceIndex_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand_,
      UnaryOperatorType type_)
      : VectorStyleExpressionASTNode(sourceIndex_),
        operand(std::move(operand_)),
        type(type_) {}

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const;
};

enum class BinaryBooleanOperatorType : uint8_t { Or, And };
struct BinaryBooleanOperatorNode : public VectorStyleExpressionASTNode {
  std::unique_ptr<VectorStyleExpressionASTNode> operand1;
  std::unique_ptr<VectorStyleExpressionASTNode> operand2;
  BinaryBooleanOperatorType type;

  BinaryBooleanOperatorNode(
      size_t sourceIndex_,
      BinaryBooleanOperatorType type_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand1_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand2_)
      : VectorStyleExpressionASTNode(sourceIndex_),
        operand1(std::move(operand1_)),
        operand2(std::move(operand2_)),
        type(type_) {}

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const;
};

struct BinaryOperatorNode : public VectorStyleExpressionASTNode {
  std::unique_ptr<VectorStyleExpressionASTNode> operand1;
  std::unique_ptr<VectorStyleExpressionASTNode> operand2;

  BinaryOperatorNode(
      size_t sourceIndex_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand1_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand2_)
      : VectorStyleExpressionASTNode(sourceIndex_),
        operand1(std::move(operand1_)),
        operand2(std::move(operand2_)) {}

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const = 0;

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const;
};

enum class BinaryBooleanComparisonOperatorType : uint8_t { Equals, NotEquals };

struct BinaryBooleanComparisonOperatorNode : public BinaryOperatorNode {
  BinaryBooleanComparisonOperatorType type;

  BinaryBooleanComparisonOperatorNode(
      size_t sourceIndex_,
      BinaryBooleanComparisonOperatorType type_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand1_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand2_)
      : BinaryOperatorNode(sourceIndex_, std::move(operand1_), std::move(operand2_)), type(type_) {}

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const;
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
      VectorStyleExpressionValue& value2) const;
};

struct BinaryAdditionOperatorNode : public BinaryOperatorNode {
  BinaryAdditionOperatorNode(
      size_t sourceIndex_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand1_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand2_)
      : BinaryOperatorNode(sourceIndex_, std::move(operand1_), std::move(operand2_)) {}

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const;
};

struct BinarySubtractionOperatorNode : public BinaryOperatorNode {
  BinarySubtractionOperatorNode(
      size_t sourceIndex_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand1_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand2_)
      : BinaryOperatorNode(sourceIndex_, std::move(operand1_), std::move(operand2_)) {}

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const;
};

struct BinaryMultiplyOperatorNode : public BinaryOperatorNode {
  BinaryMultiplyOperatorNode(
      size_t sourceIndex_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand1_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand2_)
      : BinaryOperatorNode(sourceIndex_, std::move(operand1_), std::move(operand2_)) {}

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const;

private:
  VectorStyleExpressionValue
  applyVectorDouble(double d, VectorStyleExpressionValue vec) const;
};

struct BinaryDivideOperatorNode : public BinaryOperatorNode {
  BinaryDivideOperatorNode(
      size_t sourceIndex_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand1_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand2_)
      : BinaryOperatorNode(sourceIndex_, std::move(operand1_), std::move(operand2_)) {}

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const;

private:
  VectorStyleExpressionValue
  applyVectorDouble(double d, VectorStyleExpressionValue vec) const;
};

struct BinaryModuloOperatorNode : public BinaryOperatorNode {
  BinaryModuloOperatorNode(
      size_t sourceIndex_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand1_,
      std::unique_ptr<VectorStyleExpressionASTNode>&& operand2_)
      : BinaryOperatorNode(sourceIndex_, std::move(operand1_), std::move(operand2_)) {}

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const;
};

enum class RegexOperatorType { Match, NotMatch };

struct BinaryRegexOperatorNode : public BinaryOperatorNode {
  RegexOperatorType type;

  virtual Result<VectorStyleExpressionValue> executeWithOperands(
      VectorStyleExpressionValue& value1,
      VectorStyleExpressionValue& value2) const;

private:
  bool handleMatch(const std::string& str, const std::regex& regex) const;
};

struct TernaryOperatorNode : public VectorStyleExpressionASTNode {
  std::unique_ptr<VectorStyleExpressionASTNode> conditional;
  std::unique_ptr<VectorStyleExpressionASTNode> branch1;
  std::unique_ptr<VectorStyleExpressionASTNode> branch2;

  virtual Result<VectorStyleExpressionValue>
  execute(VectorStyleExpressionContext& context) const;
};

} // namespace CesiumVectorData