#pragma once

#include "VectorStyleExpressionTokenizer.h"

using namespace CesiumUtility;

namespace CesiumVectorData {

VectorStyleExpressionTokenizer::VectorStyleExpressionTokenizer(
    std::string&& text)
    : _text(std::move(text)) {}

CesiumUtility::Result<std::vector<VectorStyleExpressionToken>>
VectorStyleExpressionTokenizer::tokenizeAll() {
  size_t position = 0;
  std::vector<VectorStyleExpressionToken> tokens;

  while (position <= this->_text.length()) {
    Result<VectorStyleExpressionToken> tokenResult = this->nextToken(position);
    if (!tokenResult.value) {
      return Result<std::vector<VectorStyleExpressionToken>>(
          tokenResult.errors);
    }

    if (tokenResult.value->type == VectorStyleExpressionTokenType::EndOfFile) {
      break;
    }

    position += tokenResult.value->value.length();
    tokens.emplace_back(std::move(*tokenResult.value));
  }

  return tokens;
}

CesiumUtility::Result<VectorStyleExpressionToken>
VectorStyleExpressionTokenizer::nextToken(size_t position) {
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
    return tokenFromChar(VectorStyleExpressionTokenType::CloseParen, position);
  } else if (nextCh == '$' && nextCharEquals(position, '{')) {
    // If dollar sign and next char isn't a {, it's probably a variable
    return tokenFromChar(
        VectorStyleExpressionTokenType::TemplateSign,
        position);
  } else if (nextCh == '+') {
    return tokenFromChar(VectorStyleExpressionTokenType::Plus, position);
  } else if (nextCh == '-') {
    return tokenFromChar(VectorStyleExpressionTokenType::Minus, position);
  } else if (nextCh == '*') {
    return tokenFromChar(
        VectorStyleExpressionTokenType::Times,
        position);
    }else if (nextCh == '/') {
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
      return tokenFromSubstr(VectorStyleExpressionTokenType::And, position, 2);
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

    return tokenFromChar(VectorStyleExpressionTokenType::GreaterThan, position);
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

CesiumUtility::Result<VectorStyleExpressionToken>
VectorStyleExpressionTokenizer::tokenFromChar(
    VectorStyleExpressionTokenType type,
    size_t position) {
  return tokenFromSubstr(type, position, 1);
}

CesiumUtility::Result<VectorStyleExpressionToken>
VectorStyleExpressionTokenizer::tokenFromSubstr(
    VectorStyleExpressionTokenType type,
    size_t position,
    size_t length) {
  return Result<VectorStyleExpressionToken>(VectorStyleExpressionToken{
      type,
      position,
      {this->_text.begin() + (int64_t)position,
       this->_text.begin() + (int64_t)position + (int64_t)length}});
}

bool VectorStyleExpressionTokenizer::hasNextChar(size_t position) {
  return (position + 1) < this->_text.length();
}

bool VectorStyleExpressionTokenizer::nextCharEquals(size_t position, char ch) {
  return hasNextChar(position) && this->_text[position + 1] == ch;
}

bool VectorStyleExpressionTokenizer::isAsciiDigit(char ch) {
  return ch >= '0' && ch <= '9';
}

bool VectorStyleExpressionTokenizer::isAsciiAlpha(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

bool VectorStyleExpressionTokenizer::isValidIdentifierCh(char ch) {
  return isAsciiAlpha(ch) || isAsciiDigit(ch) || ch == '$' || ch == '_';
}

CesiumUtility::Result<VectorStyleExpressionToken>
VectorStyleExpressionTokenizer::unexpectedTokenError(
    size_t position,
    size_t sequenceLen) {
  return Result<VectorStyleExpressionToken>(ErrorList::error(fmt::format(
      "Unexpected sequence '{}' at position {}",
      this->_text.substr(position, sequenceLen),
      position)));
}
constexpr std::string_view tokenToString(VectorStyleExpressionTokenType type) {
  switch (type) {
  case VectorStyleExpressionTokenType::EndOfFile: {
    constexpr std::string_view val = "EOF";
    return val;
  }
  case VectorStyleExpressionTokenType::Identifier: {
    constexpr std::string_view val = "Identifier";
    return val;
  }
  case VectorStyleExpressionTokenType::Number: {
    constexpr std::string_view val = "Number";
    return val;
  }
  case VectorStyleExpressionTokenType::String: {
    constexpr std::string_view val = "String";
    return val;
  }
  case VectorStyleExpressionTokenType::OpenSquareBracket: {
    constexpr std::string_view val = "[";
    return val;
  }
  case VectorStyleExpressionTokenType::CloseSquareBracket: {
    constexpr std::string_view val = "]";
    return val;
  }
  case VectorStyleExpressionTokenType::OpenCurlyBracket: {
    constexpr std::string_view val = "{";
    return val;
  }
  case VectorStyleExpressionTokenType::CloseCurlyBracket: {
    constexpr std::string_view val = "}";
    return val;
  }
  case VectorStyleExpressionTokenType::OpenParen: {
    constexpr std::string_view val = "(";
    return val;
  }
  case VectorStyleExpressionTokenType::CloseParen: {
    constexpr std::string_view val = ")";
    return val;
  }
  case VectorStyleExpressionTokenType::TemplateSign: {
    constexpr std::string_view val = "$";
    return val;
  }
  case VectorStyleExpressionTokenType::Plus: {
    constexpr std::string_view val = "+";
    return val;
  }
  case VectorStyleExpressionTokenType::Minus: {
    constexpr std::string_view val = "-";
    return val;
  }
  case VectorStyleExpressionTokenType::Times: {
    constexpr std::string_view val = "*";
    return val;
  }
  case VectorStyleExpressionTokenType::ExclamationPoint: {
    constexpr std::string_view val = "!";
    return val;
  }
  case VectorStyleExpressionTokenType::Or: {
    constexpr std::string_view val = "||";
    return val;
  }
  case VectorStyleExpressionTokenType::And: {
    constexpr std::string_view val = "&&";
    return val;
  }
  case VectorStyleExpressionTokenType::Equals: {
    constexpr std::string_view val = "==";
    return val;
  }
  case VectorStyleExpressionTokenType::NotEquals: {
    constexpr std::string_view val = "!=";
    return val;
  }
  case VectorStyleExpressionTokenType::GreaterThan: {
    constexpr std::string_view val = ">";
    return val;
  }
  case VectorStyleExpressionTokenType::LessThan: {
    constexpr std::string_view val = "<";
    return val;
  }
  case VectorStyleExpressionTokenType::GreaterThanEq: {
    constexpr std::string_view val = ">=";
    return val;
  }
  case VectorStyleExpressionTokenType::LessThanEq: {
    constexpr std::string_view val = "<=";
    return val;
  }
  case VectorStyleExpressionTokenType::ForwardSlash: {
    constexpr std::string_view val = "/";
    return val;
  }
  case VectorStyleExpressionTokenType::Percent: {
    constexpr std::string_view val = "%";
    return val;
  }
  case VectorStyleExpressionTokenType::RegexEq: {
    constexpr std::string_view val = "=~";
    return val;
  }
  case VectorStyleExpressionTokenType::RegexNeq: {
    constexpr std::string_view val = "!~";
    return val;
  }
  case VectorStyleExpressionTokenType::QuestionMark: {
    constexpr std::string_view val = "?";
    return val;
  }
  case VectorStyleExpressionTokenType::Colon: {
    constexpr std::string_view val = ":";
    return val;
  }
  case VectorStyleExpressionTokenType::Dot: {
    constexpr std::string_view val = ".";
    return val;
  }
  }
}
} // namespace CesiumVectorData