#include <CesiumAsync/AsyncSystem.h>
#include <CesiumAsync/Future.h>
#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumAsync/IAssetRequest.h>
#include <CesiumAsync/IAssetResponse.h>
#include <CesiumAsync/ITaskProcessor.h>
#include <CesiumAsync/Promise.h>
#include <CesiumGltf/Model.h>
#include <CesiumNativeTests/SimpleAssetAccessor.h>
#include <CesiumNativeTests/SimpleAssetRequest.h>
#include <CesiumNativeTests/SimpleAssetResponse.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

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
processDownloadedContent(const std::span<const std::byte>& /*bytes*/) {
  return ProcessedContent();
}
void useDownloadedContent(const std::span<const std::byte>& /*bytes*/) {}
void updateApplicationWithProcessedContent(
    const ProcessedContent& /*content*/) {}

CesiumAsync::Future<ProcessedContent>
startOperationThatMightFail(const CesiumAsync::AsyncSystem& asyncSystem) {
  return asyncSystem.createResolvedFuture(ProcessedContent());
}

void showError(const std::string& /*message*/) {}

std::string
findReferencedImageUrl(const std::span<const std::byte>& /*bytes*/) {
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

struct SlowValue {};
void computeSomethingSlowly(
    const std::string& /*parameter*/,
    std::function<void(const SlowValue&)> f) {
  f(SlowValue());
}

template <typename T> void doSomething(const T&) {}

//! [compute-something-slowly-wrapper]
CesiumAsync::Future<SlowValue> myComputeSomethingSlowlyWrapper(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& someParameter) {
  CesiumAsync::Promise<SlowValue> promise =
      asyncSystem.createPromise<SlowValue>();

  computeSomethingSlowly(someParameter, [promise](const SlowValue& value) {
    promise.resolve(value);
  });

  return promise.getFuture();
}
//! [compute-something-slowly-wrapper]

//! [compute-something-slowly-wrapper-handle-exception]
CesiumAsync::Future<SlowValue> myComputeSomethingSlowlyWrapper2(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const std::string& someParameter) {
  return asyncSystem.createFuture<SlowValue>(
      [&](const CesiumAsync::Promise<SlowValue>& promise) {
        computeSomethingSlowly(
            someParameter,
            [promise](const SlowValue& value) { promise.resolve(value); });
      });
}
//! [compute-something-slowly-wrapper-handle-exception]

CesiumGltf::Model getModelFromSomewhere() { return {}; }

void giveBackModel(CesiumGltf::Model&&) {}

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

  SUBCASE("wait") {
    //! [create-request-future]
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
        getAssetAccessor();

    CesiumAsync::Future<std::shared_ptr<CesiumAsync::IAssetRequest>> future =
        pAssetAccessor->get(asyncSystem, "https://example.com");
    //! [create-request-future]

    //! [wait]
    std::shared_ptr<CesiumAsync::IAssetRequest> pCompletedRequest =
        std::move(future).wait();
    //! [wait]
  }

  SUBCASE("thenInMainThread") {
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

  SUBCASE("chaining") {
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

  SUBCASE("catch") {
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

  SUBCASE("unwrapping") {
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

  SUBCASE("then-pass-through") {
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

  SUBCASE("all") {
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

  SUBCASE("create-resolved-future") {
    std::shared_ptr<CesiumAsync::IAssetAccessor> pAssetAccessor =
        getAssetAccessor();

    //! [create-resolved-future]
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
                  std::optional<std::string> maybeUrl =
                      findReferencedImageUrl(processed);
                  if (!maybeUrl) {
                    return asyncSystem.createResolvedFuture<
                        std::shared_ptr<CesiumAsync::IAssetRequest>>(nullptr);
                  } else {
                    return pAssetAccessor->get(asyncSystem, *maybeUrl);
                  }
                })
            .thenInMainThread([](std::shared_ptr<CesiumAsync::IAssetRequest>&&
                                     pImageRequest) {
              if (pImageRequest) {
                useLoadedImage(pImageRequest);
              }
            });
    //! [create-resolved-future]

    std::move(future).waitInMainThread();
  }

  SUBCASE("promise") {
    //! [compute-something-slowly]
    computeSomethingSlowly("some parameter", [](const SlowValue& value) {
      doSomething(value);
    });
    //! [compute-something-slowly]

    //! [compute-something-slowly-async-system]
    CesiumAsync::Promise<SlowValue> promise =
        asyncSystem.createPromise<SlowValue>();

    computeSomethingSlowly("some parameter", [promise](const SlowValue& value) {
      promise.resolve(value);
    });

    CesiumAsync::Future<void> future =
        promise.getFuture().thenInMainThread([](SlowValue&& value) {
          // Continue working with the slowly-computed value in the main thread.
          doSomething(value);
        });
    //! [compute-something-slowly-async-system]

    future.waitInMainThread();
  }

  SUBCASE("lambda-move") {
    //! [lambda-move]
    CesiumGltf::Model model = getModelFromSomewhere();
    CesiumAsync::Future<void> future =
        asyncSystem
            .runInWorkerThread([model = std::move(model)]() mutable {
              doSomething(model);
              return std::move(model);
            })
            .thenInMainThread([](CesiumGltf::Model&& model) {
              giveBackModel(std::move(model));
            });
    //! [lambda-move]

    future.waitInMainThread();
  }

  SUBCASE("use example functions") {
    CesiumAsync::AsyncSystem& localAsyncSystem = getAsyncSystem();
    myComputeSomethingSlowlyWrapper(localAsyncSystem, "something")
        .waitInMainThread();
    myComputeSomethingSlowlyWrapper2(localAsyncSystem, "something")
        .waitInMainThread();
  }
}
