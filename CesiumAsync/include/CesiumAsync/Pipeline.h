#pragma once

#include <CesiumAsync/AsyncSystem.h>
#include <CesiumUtility/IntrusivePointer.h>

#include <atomic>
#include <variant>

namespace CesiumAsync {

class PipelineFailure {
public:
};

enum class FailureAction { Retry, GiveUp };

template <typename TDerived, typename TFailureType, typename TResultType>
class Pipeline {
public:
  using ResultOrFailure = std::variant<TFailureType, TResultType>;

  Pipeline(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<spdlog::logger>& pLogger)
      : _referenceCount(0), _asyncSystem(asyncSystem), _pLogger(pLogger) {}

  void addReference() const noexcept { ++this->_referenceCount; }
  void releaseReference() const noexcept {
    if (--this->_referenceCount == 0) {
      delete this;
    }
  }

  const CesiumAsync::AsyncSystem& getAsyncSystem() const {
    return this->_asyncSystem;
  }
  const std::shared_ptr<spdlog::logger>& getLogger() const {
    return this->_pLogger;
  }

  CesiumAsync::Future<void> run() noexcept {
    CesiumUtility::IntrusivePointer<TDerived> pThis =
        static_cast<TDerived*>(this);

    auto exceptionToFailure = [](const std::exception& e) -> ResultOrFailure {
      return TFailureType(e);
    };

    auto handleFailure = [pThis](TFailureType&& failure) mutable {
      return pThis->handleFailure(std::move(failure));
    };

    auto retryOrFail =
        [pThis](std::pair<TFailureType, FailureAction>&& failureResult) {
          if (failureResult.second == FailureAction::Retry) {
            return pThis->run();
          } else {
            return pThis->onFailure(std::move(failureResult.first));
          }
        };

    auto onComplete = [pThis, handleFailure, retryOrFail](
                          ResultOrFailure&& result) mutable -> Future<void> {
      TFailureType* pFailure = std::get_if<TFailureType>(&result);
      if (pFailure) {
        return pThis->_asyncSystem.createResolvedFuture(std::move(*pFailure))
            .thenImmediately(handleFailure)
            .thenImmediately(retryOrFail);
      } else {
        return pThis->onSuccess(std::move(std::get<TResultType>(result)));
      }
    };

    return pThis->begin()
        .catchImmediately(exceptionToFailure)
        .thenImmediately(onComplete);
  }

private:
  mutable std::atomic<int32_t> _referenceCount;
  CesiumAsync::AsyncSystem _asyncSystem;
  std::shared_ptr<spdlog::logger> _pLogger;
};

struct FailureType {
  FailureType() {}
  FailureType(const std::exception& /* e */) {}
};
struct ResultType {};

class TestPipeline : public Pipeline<TestPipeline, FailureType, ResultType> {
public:
  TestPipeline(
      const CesiumAsync::AsyncSystem& asyncSystem,
      const std::shared_ptr<spdlog::logger>& pLogger)
      : Pipeline(asyncSystem, pLogger) {}

private:
  friend class Pipeline<TestPipeline, FailureType, ResultType>;

  CesiumAsync::Future<ResultOrFailure> begin() {
    return this->getAsyncSystem().createResolvedFuture(
        ResultOrFailure(ResultType()));
  }

  CesiumAsync::Future<std::pair<FailureType, FailureAction>>
  handleFailure(FailureType&& failure) {
    return this->getAsyncSystem().createResolvedFuture(
        std::make_pair(std::move(failure), FailureAction()));
  }

  CesiumAsync::Future<void> onSuccess(ResultType&&) {
    return this->getAsyncSystem().createResolvedFuture();
  }

  CesiumAsync::Future<void> onFailure(FailureType&&) {
    return this->getAsyncSystem().createResolvedFuture();
  }
};

}; // namespace CesiumAsync
