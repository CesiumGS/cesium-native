#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>

#include <catch2/catch.hpp>

#include <optional>
#include <string>
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
                       std::vector<std::byte>()))},
              {"http://example.com/image.png",
               std::make_shared<SimpleAssetRequest>(
                   "GET",
                   "https://example.com",
                   CesiumAsync::HttpHeaders(),
                   std::make_unique<SimpleAssetResponse>(
                       uint16_t(200),
                       "image.png",
                       CesiumAsync::HttpHeaders(),
                       std::vector<std::byte>()))}});
  return pAssetAccessor;
}

struct ProcessedContent {
  static ProcessedContent createFailed(const std::string& message) {
    return ProcessedContent{message};
  }

  std::optional<std::string> failureMessage;

  bool isFailed() const { return failureMessage.has_value(); }
  std::string getFailureMessage() const {
    return failureMessage ? *failureMessage : "";
  }
};

ProcessedContent
processDownloadedContent(const gsl::span<const std::byte>& /*bytes*/) {
  return ProcessedContent();
}
void useDownloadedContent(const gsl::span<const std::byte>& /*bytes*/) {}
void updateApplicationWithProcessedContent(
    const ProcessedContent& /*content*/) {}

CesiumAsync::Future<ProcessedContent>
startOperationThatMightFail(const CesiumAsync::AsyncSystem& asyncSystem) {
  return asyncSystem.createResolvedFuture(ProcessedContent());
}

void showError(const std::string& /*message*/) {}

std::string
findReferencedImageUrl(const gsl::span<const std::byte>& /*bytes*/) {
  return "http://example.com/image.png";
}

std::string findReferencedImageUrl(const ProcessedContent& /*processed*/) {
  return "http://example.com/image.png";
}

std::vector<std::string>
findReferencedImageUrls(const ProcessedContent& /*processed*/) {
  return {"http://example.com/image.png"};
}

void useLoadedImage(const std::shared_ptr<CesiumAsync::IAssetRequest>&) {}
void usePage(const std::shared_ptr<CesiumAsync::IAssetRequest>&) {}

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
    CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>>
        requestFuture = pAssetAccessor->get(asyncSystem, "https://example.com");

    CesiumAsync::Future<void> future =
        std::move(requestFuture)
            .thenInMainThread(
                [](std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const CesiumAsync::IAssetResponse* pResponse =
                      pRequest->response();
                  // handling of an error response ommitted
                  useDownloadedContent(pResponse->data());
                });
    //! [continuation]

    std::move(future).waitInMainThread();
  }

  SECTION("chaining") {
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
        getAssetAccessor();

    //! [chaining]
    CesiumAsync::Future<void> future =
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

    std::move(future).waitInMainThread();
  }

  SECTION("catch") {
    //! [catch]
    CesiumAsync::Future<void> future =
        startOperationThatMightFail(asyncSystem)
            .catchImmediately([](std::exception&& e) {
              return ProcessedContent::createFailed(e.what());
            })
            .thenInMainThread([](ProcessedContent&& processed) {
              if (processed.isFailed()) {
                showError(processed.getFailureMessage());
              } else {
                updateApplicationWithProcessedContent(processed);
              }
            });
    //! [catch]

    std::move(future).waitInMainThread();
  }

  SECTION("unwrapping") {
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
        getAssetAccessor();

    //! [unwrapping]
    CesiumAsync::Future<void> future =
        pAssetAccessor->get(asyncSystem, "https://example.com")
            .thenInWorkerThread(
                [pAssetAccessor, asyncSystem](
                    std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const CesiumAsync::IAssetResponse* pResponse =
                      pRequest->response();
                  // handling of an error response ommitted
                  std::string url = findReferencedImageUrl(pResponse->data());
                  return pAssetAccessor->get(asyncSystem, url);
                })
            .thenInMainThread([](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                     pImageRequest) {
              // Do something with the loaded image
              useLoadedImage(pImageRequest);
            });
    //! [unwrapping]

    std::move(future).waitInMainThread();
  }

  SECTION("then-pass-through") {
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
        getAssetAccessor();

    //! [then-pass-through]
    CesiumAsync::Future<void> future =
        pAssetAccessor->get(asyncSystem, "https://example.com")
            .thenInWorkerThread(
                [pAssetAccessor, asyncSystem](
                    std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const CesiumAsync::IAssetResponse* pResponse =
                      pRequest->response();

                  // handling of an error response ommitted

                  ProcessedContent processed =
                      processDownloadedContent(pResponse->data());
                  std::string url = findReferencedImageUrl(processed);
                  return pAssetAccessor->get(asyncSystem, url)
                      .thenPassThrough(std::move(processed));
                })
            .thenInMainThread(
                [](std::tuple<
                    ProcessedContent,
                    std::shared_ptr<CesiumAsync::IAssetRequest>>&& tuple) {
                  auto& [processed, pImageRequest] = tuple;
                  useLoadedImage(pImageRequest);
                  updateApplicationWithProcessedContent(processed);
                });
    //! [then-pass-through]

    std::move(future).waitInMainThread();
  }

  SECTION("all") {
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
        getAssetAccessor();

    //! [all]
    CesiumAsync::Future<void> future =
        pAssetAccessor->get(asyncSystem, "https://example.com")
            .thenInWorkerThread(
                [pAssetAccessor, asyncSystem](
                    std::shared_ptr<CesiumAsync::IAssetRequest>&& pRequest) {
                  const CesiumAsync::IAssetResponse* pResponse =
                      pRequest->response();

                  // handling of an error response ommitted

                  ProcessedContent processed =
                      processDownloadedContent(pResponse->data());

                  std::vector<std::string> urls =
                      findReferencedImageUrls(processed);

                  std::vector<CesiumAsync::Future<
                      std::shared_ptr<CesiumAsync::IAssetRequest>>>
                      futures;
                  futures.reserve(urls.size());

                  for (const std::string& url : urls) {
                    futures.emplace_back(pAssetAccessor->get(asyncSystem, url));
                  }

                  return asyncSystem.all(std::move(futures))
                      .thenPassThrough(std::move(processed));
                })
            .thenInMainThread(
                [](std::tuple<
                    ProcessedContent,
                    std::vector<std::shared_ptr<CesiumAsync::IAssetRequest>>>&&
                       tuple) {
                  auto& [processed, imageRequests] = tuple;

                  for (const std::shared_ptr<CesiumAsync::IAssetRequest>&
                           pImageRequest : imageRequests) {
                    useLoadedImage(pImageRequest);
                  }

                  updateApplicationWithProcessedContent(processed);
                });
    //! [all]

    std::move(future).waitInMainThread();
  }
}
