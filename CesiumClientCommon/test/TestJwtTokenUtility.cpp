#include <CesiumClientCommon/JwtTokenUtility.h>
#include <CesiumUtility/Result.h>

#include <doctest/doctest.h>
#include <rapidjson/document.h>

using namespace CesiumClientCommon;
using namespace CesiumUtility;

TEST_CASE("Correctly decodes a JWT token") {
  // This is a dummy JWT token generated using jwt.io. It means nothing and
  // provides access to nothing.
  const std::string token =
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
      "eyJzdWIiOiIxMjM0NTY3ODkwIiwiaWF0IjoxNTE2MjM5MDIyLCJleHAiOjEyMzR9."
      "vk7saKf0To6wX-f8j4uRetpm_ZkhLpI9CdScHRMcH78";
  Result<rapidjson::Document> parseResult =
      JwtTokenUtility::parseTokenPayload(token);
  REQUIRE(parseResult.value);
  CHECK(!parseResult.errors.hasErrors());
  REQUIRE(parseResult.value->IsObject());

  const rapidjson::Value::Object& obj = parseResult.value->GetObject();
  rapidjson::Value::Object::ConstMemberIterator it = obj.FindMember("exp");
  REQUIRE(it != obj.MemberEnd());
  REQUIRE(it->value.IsNumber());
  CHECK(it->value.GetInt64() == 1234);
}