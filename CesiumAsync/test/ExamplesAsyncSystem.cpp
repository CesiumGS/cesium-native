#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>

#include <catch2/catch.hpp>

#include <thread>

using namespace CesiumNativeTests;

namespace {

//! [simplest-task-processor]
class SimplestTaskProcessor : public CesiumAsync::ITaskProcessor {
public:
  void startTask(std::function<void()> f) override {
    std::thread(std::move(f)).detach();
  }
};
//! [simplest-task-processor]

//! [async-system-singleton]
CesiumAsync::AsyncSystem& getAsyncSystem() {
  static CesiumAsync::AsyncSystem asyncSystem(
      std::make_shared<SimplestTaskProcessor>());
  return asyncSystem;
}
//! [async-system-singleton]

std::shared_ptr<CesiumAsync::IAssetAccessor> getAssetAccessor() {
  static std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
      std::make_shared<SimpleAssetAccessor>(
          std::map<std::string, std::shared_ptr<SimpleAssetRequest>>{
              {"https://example.com",
               std::make_shared<SimpleAssetRequest>(
                   "GET",
                   "https://example.com",
                   CesiumAsync::HttpHeaders(),
                   std::make_unique<SimpleAssetResponse>(
                       uint16_t(200),
                       "text/html",
                       CesiumAsync::HttpHeaders(),
                       std::vector<std::byte>()))}});
  return pAssetAccessor;
}

struct ProcessedContent {};
ProcessedContent
processDownloadedContent(const gsl::span<const std::byte>& /*bytes*/) {
  return ProcessedContent();
}
void useDownloadedContent(const gsl::span<const std::byte>& /*bytes*/) {}
void updateApplicationWithProcessedContent(
    const ProcessedContent& /*content*/) {}

} // namespace

TEST_CASE("AsyncSystem Examples") {
  //! [create-async-system]
  CesiumAsync::AsyncSystem asyncSystem(
      std::make_shared<SimplestTaskProcessor>());
  //! [create-async-system]

  //! [capture-by-value]
  auto someLambda = [asyncSystem]() {
    return asyncSystem.createResolvedFuture(4);
  };
  //! [capture-by-value]

  SECTION("waitInMainThread") {
    //! [create-request-future]
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
        getAssetAccessor();

    CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> future =
        pAssetAccessor->get(asyncSystem, "https://example.com");
    //! [create-request-future]

    //! [wait-in-main-thread]
    std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest =
        std::move(future).waitInMainThread();
    //! [wait-in-main-thread]
  }

  SECTION("thenInMainThread") {
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
        getAssetAccessor();

    //! [continuation]
    CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> future =
        pAssetAccessor->get(asyncSystem, "https://example.com");

    std::move(future).thenInMainThread(
        [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
          const CesiumAsync::IAssetResponse* pResponse = pRequest->response();
          // handling of an error response ommitted
          useDownloadedContent(pResponse->data());
        });
    //! [continuation]
  }

  SECTION("chaining") {
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
        getAssetAccessor();

    //! [chaining]
    pAssetAccessor->get(asyncSystem, "https://example.com")
        .thenInWorkerThread(
            [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
              const CesiumAsync::IAssetResponse* pResponse =
                  pRequest->response();
              // handling of an error response ommitted
              ProcessedContent processed =
                  processDownloadedContent(pResponse->data());
              return processed;
            })
        .thenInMainThread([](ProcessedContent&& processed) {
          updateApplicationWithProcessedContent(processed);
        });
    //! [chaining]
  }
}
