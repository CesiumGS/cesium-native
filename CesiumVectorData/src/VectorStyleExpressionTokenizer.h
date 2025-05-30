#pragma once

#include <CesiumUtility/Result.h>

#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CesiumVectorData {

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
  Times,
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

constexpr std::string_view tokenToString(VectorStyleExpressionTokenType type);

struct VectorStyleExpressionToken {
  VectorStyleExpressionTokenType type;
  size_t position;
  std::string_view value;
};

class VectorStyleExpressionTokenizer {
public:
  VectorStyleExpressionTokenizer(std::string&& text);

  CesiumUtility::Result<std::vector<VectorStyleExpressionToken>> tokenizeAll();

private:
  CesiumUtility::Result<VectorStyleExpressionToken> nextToken(size_t position);

  CesiumUtility::Result<VectorStyleExpressionToken>
  tokenFromChar(VectorStyleExpressionTokenType type, size_t position);

  CesiumUtility::Result<VectorStyleExpressionToken> tokenFromSubstr(
      VectorStyleExpressionTokenType type,
      size_t position,
      size_t length);

  bool hasNextChar(size_t position);
  bool nextCharEquals(size_t position, char ch);
  bool isAsciiDigit(char ch);
  bool isAsciiAlpha(char ch);
  bool isValidIdentifierCh(char ch);

  CesiumUtility::Result<VectorStyleExpressionToken>
  unexpectedTokenError(size_t position, size_t sequenceLen);

  std::string _text;
};
} // namespace CesiumVectorData