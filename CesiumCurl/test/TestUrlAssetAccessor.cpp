#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumCurl/UrlAssetAccessor.h>
#include <CesiumNativeTests/ThreadTaskProcessor.h>
#include <CesiumUtility/ScopeGuard.h>
#include <CesiumUtility/Uri.h>

#include <doctest/doctest.h>
#include <httplib.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <span>

using namespace CesiumCurl;
using namespace CesiumNativeTests;
using namespace CesiumUtility;

TEST_CASE("UrlAssetAccessor") {
  CesiumAsync::AsyncSystem asyncSystem(std::make_shared<ThreadTaskProcessor>());
  std::shared_ptr<UrlAssetAccessor> pAssetAccessor =
      std::make_shared<UrlAssetAccessor>();

  SUBCASE("can do HTTP requests") {
    std::shared_ptr<httplib::Server> pServer =
        std::make_shared<httplib::Server>();
    const int port = pServer->bind_to_any_port("127.0.0.1");

    CesiumUtility::ScopeGuard stopServer([pServer]() { pServer->stop(); });

    std::thread([pServer]() { pServer->listen_after_bind(); }).detach();

    SUBCASE("GET") {
      pServer->Get(
          "/test/some/file.txt",
          [](const httplib::Request& /* request */,
             httplib::Response& response) {
            response.set_content("this is my response text", "text/plain");
          });

      std::shared_ptr<CesiumAsync::IAssetRequest> pRequest =
          pAssetAccessor
              ->get(
                  asyncSystem,
                  std::format("http://127.0.01:{}/test/some/file.txt", port),
                  {})
              .waitInMainThread();

      REQUIRE(pRequest);

      const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
      REQUIRE(pResponse);

      CHECK_EQ(pResponse->statusCode(), 200);
      CHECK_EQ(
          std::string(
              reinterpret_cast<const char*>(pResponse->data().data()),
              pResponse->data().size()),
          "this is my response text");
    }

    SUBCASE("POST") {
      pServer->Post(
          "/my/post/target",
          [](const httplib::Request& request, httplib::Response& response) {
            CHECK_EQ(request.body, "this is the post payload");
            response.set_content("this is my response text", "text/plain");
          });

      std::string postPayloadString = "this is the post payload";
      std::span<const std::byte> postPayload(
          reinterpret_cast<const std::byte*>(postPayloadString.data()),
          postPayloadString.size());

      std::shared_ptr<CesiumAsync::IAssetRequest> pRequest =
          pAssetAccessor
              ->request(
                  asyncSystem,
                  "POST",
                  std::format("http://127.0.01:{}/my/post/target", port),
                  {},
                  postPayload)
              .waitInMainThread();

      REQUIRE(pRequest);

      const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
      REQUIRE(pResponse);

      CHECK_EQ(pResponse->statusCode(), 200);
      CHECK_EQ(
          std::string(
              reinterpret_cast<const char*>(pResponse->data().data()),
              pResponse->data().size()),
          "this is my response text");
    }

    SUBCASE("PUT") {
      pServer->Put(
          "/my/put/target",
          [](const httplib::Request& request, httplib::Response& response) {
            CHECK_EQ(request.body, "this is the put payload");
            response.set_content("this is my response text", "text/plain");
          });

      std::string putPayloadString = "this is the put payload";
      std::span<const std::byte> putPayload(
          reinterpret_cast<const std::byte*>(putPayloadString.data()),
          putPayloadString.size());

      std::shared_ptr<CesiumAsync::IAssetRequest> pRequest =
          pAssetAccessor
              ->request(
                  asyncSystem,
                  "PUT",
                  std::format("http://127.0.01:{}/my/put/target", port),
                  {},
                  putPayload)
              .waitInMainThread();

      REQUIRE(pRequest);

      const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
      REQUIRE(pResponse);

      CHECK_EQ(pResponse->statusCode(), 200);
      CHECK_EQ(
          std::string(
              reinterpret_cast<const char*>(pResponse->data().data()),
              pResponse->data().size()),
          "this is my response text");
    }
  }

  SUBCASE("can do file:/// requests") {
    std::filesystem::path testFilePath =
        std::filesystem::temp_directory_path() / "test.txt";
    std::filesystem::remove(testFilePath);

    SUBCASE("GET") {
      // Create a temporary file to attempt to read
      std::ofstream writer(testFilePath);
      writer << "some text in the file";
      writer.close();

      Uri fileUrl("file:///");
      fileUrl.setPath(Uri::nativePathToUriPath(testFilePath.string()));
      CHECK(fileUrl.toString().starts_with("file:///"));

      std::shared_ptr<CesiumAsync::IAssetRequest> pRequest =
          pAssetAccessor->get(asyncSystem, std::string(fileUrl.toString()), {})
              .waitInMainThread();

      REQUIRE(pRequest);

      const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
      REQUIRE(pResponse);

      CHECK_EQ(pResponse->statusCode(), 0);
      CHECK_EQ(
          std::string(
              reinterpret_cast<const char*>(pResponse->data().data()),
              pResponse->data().size()),
          "some text in the file");
    }

    SUBCASE("PUT") {
      Uri fileUrl("file:///");
      fileUrl.setPath(Uri::nativePathToUriPath(testFilePath.string()));
      CHECK(fileUrl.toString().starts_with("file:///"));

      std::string putPayloadString = "this is the content in the file";
      std::span<const std::byte> putPayload(
          reinterpret_cast<const std::byte*>(putPayloadString.data()),
          putPayloadString.size());

      std::shared_ptr<CesiumAsync::IAssetRequest> pRequest =
          pAssetAccessor
              ->request(
                  asyncSystem,
                  "PUT",
                  std::string(fileUrl.toString()),
                  {},
                  putPayload)
              .waitInMainThread();

      REQUIRE(pRequest);

      const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
      REQUIRE(pResponse);

      CHECK_EQ(pResponse->statusCode(), 0);
      CHECK_EQ(pResponse->data().size(), 0);

      REQUIRE(std::filesystem::exists(testFilePath));

      size_t size = std::filesystem::file_size(testFilePath);
      std::string content;
      content.resize(size);

      std::ifstream reader(testFilePath);
      reader.read(content.data(), static_cast<int64_t>(size));
      reader.close();

      CHECK_EQ(content, "this is the content in the file");
    }

    SUBCASE("creates directories as needed") {
      std::filesystem::path testDirectoryPath =
          std::filesystem::temp_directory_path() / "test-directory";
      std::filesystem::remove_all(testDirectoryPath);

      std::filesystem::path testFileInSubdirectoryPath =
          testDirectoryPath / "myfile.txt";

      Uri fileUrl("file:///");
      fileUrl.setPath(
          Uri::nativePathToUriPath(testFileInSubdirectoryPath.string()));
      CHECK(fileUrl.toString().starts_with("file:///"));

      std::string putPayloadString = "this is the content in the file";
      std::span<const std::byte> putPayload(
          reinterpret_cast<const std::byte*>(putPayloadString.data()),
          putPayloadString.size());

      std::shared_ptr<CesiumAsync::IAssetRequest> pRequest =
          pAssetAccessor
              ->request(
                  asyncSystem,
                  "PUT",
                  std::string(fileUrl.toString()),
                  {},
                  putPayload)
              .waitInMainThread();

      REQUIRE(pRequest);

      const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
      REQUIRE(pResponse);

      CHECK_EQ(pResponse->statusCode(), 0);
      CHECK_EQ(pResponse->data().size(), 0);

      REQUIRE(std::filesystem::exists(testDirectoryPath));
      REQUIRE(std::filesystem::exists(testFileInSubdirectoryPath));

      size_t size = std::filesystem::file_size(testFileInSubdirectoryPath);
      std::string content;
      content.resize(size);

      std::ifstream reader(testFileInSubdirectoryPath);
      reader.read(content.data(), static_cast<int64_t>(size));
      reader.close();

      CHECK_EQ(content, "this is the content in the file");
    }
  }
}