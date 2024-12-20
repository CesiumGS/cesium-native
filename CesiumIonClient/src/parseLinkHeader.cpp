#include "parseLinkHeader.h"

#include <optional>
#include <regex>
#include <string>
#include <utility>
#include <vector>

// This implementation is loosely based on the JavaScript implementation found
// here:
// https://github.com/thlorenz/parse-link-header/blob/8521bd7a1256c1c3875e13d88be9dbfa7640663e/index.js.
//
// The license of that implementation is as follows (MIT):
//
// Copyright 2013 Thorsten Lorenz.
// All rights reserved.

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

namespace CesiumIonClient {

namespace {

std::optional<Link> parseLink(const std::string& linkText) {
  std::regex splitLink("<?([^>]*)>(.*)", std::regex_constants::ECMAScript);

  std::sregex_iterator matchIt(linkText.begin(), linkText.end(), splitLink);
  if (matchIt == std::sregex_iterator()) {
    return std::nullopt;
  }

  const std::smatch& match = *matchIt;
  if (match.size() != 3) {
    return std::nullopt;
  }

  Link result;

  result.url = match[1];

  std::string params = match[2];

  std::regex splitParts(
      ";\\s*(.+)\\s*=\\s*\"?([^\"]+)\"?",
      std::regex_constants::ECMAScript);
  std::sregex_iterator partIt(params.begin(), params.end(), splitParts);

  for (; partIt != std::sregex_iterator(); ++partIt) {
    const std::smatch& partMatch = *partIt;
    if (partMatch.size() != 3) {
      continue;
    }

    if (partMatch[1] == "rel") {
      result.rel = partMatch[2];
    } else {
      result.otherParameters[partMatch[1]] = partMatch[2];
    }
  }

  return result;
}

} // namespace

std::vector<Link> parseLinkHeader(const std::string& linkHeader) {
  std::vector<Link> result;

  std::regex splitLinks(",\\s*<", std::regex_constants::ECMAScript);

  std::sregex_token_iterator it(
      linkHeader.begin(),
      linkHeader.end(),
      splitLinks,
      -1);
  for (; it != std::sregex_token_iterator(); ++it) {
    std::string linkText = *it;
    std::optional<Link> link = parseLink(linkText);
    if (link) {
      result.emplace_back(std::move(*link));
    }
  }

  return result;
}

} // namespace CesiumIonClient
