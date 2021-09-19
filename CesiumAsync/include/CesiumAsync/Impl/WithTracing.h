#pragma once

#include "CesiumAsync/Impl/unwrapFuture.h"

#include <CesiumUtility/Tracing.h>

namespace CesiumAsync {
namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename T> struct WithTracing {
  template <typename Func>
  static auto
  begin([[maybe_unused]] const char* tracingName, [[maybe_unused]] Func&& f) {
#if CESIUM_TRACING_ENABLED
    return
        [tracingName, CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()](T&& result) mutable {
          CESIUM_TRACE_USE_CAPTURED_TRACK();
          if (tracingName) {
            CESIUM_TRACE_BEGIN_IN_TRACK(tracingName);
          }
          return std::move(result);
        };
#else
    return Impl::unwrapFuture<Func, T>(std::forward<Func>(f));
#endif
  }

  template <typename Func>
  static auto end([[maybe_unused]] const char* tracingName, Func&& f) {
#if CESIUM_TRACING_ENABLED
    return [tracingName,
            f = Impl::unwrapFuture<Func, T>(std::forward<Func>(f)),
            CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()](T&& result) mutable {
      CESIUM_TRACE_USE_CAPTURED_TRACK();
      if (tracingName) {
        CESIUM_TRACE_END_IN_TRACK(tracingName);
      }
      return f(std::move(result));
    };
#else
    return Impl::unwrapFuture<Func, T>(std::forward<Func>(f));
#endif
  }
};

template <typename T> struct WithTracingShared {
  template <typename Func>
  static auto
  begin([[maybe_unused]] const char* tracingName, [[maybe_unused]] Func&& f) {
#if CESIUM_TRACING_ENABLED
    return [tracingName,
            CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()](const T& result) mutable {
      CESIUM_TRACE_USE_CAPTURED_TRACK();
      if (tracingName) {
        CESIUM_TRACE_BEGIN_IN_TRACK(tracingName);
      }
      return result;
    };
#else
    return Impl::unwrapFuture<Func, T>(std::forward<Func>(f));
#endif
  }

  template <typename Func>
  static auto end([[maybe_unused]] const char* tracingName, Func&& f) {
#if CESIUM_TRACING_ENABLED
    return [tracingName,
            f = Impl::unwrapFuture<Func, T>(std::forward<Func>(f)),
            CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()](const T& result) mutable {
      CESIUM_TRACE_USE_CAPTURED_TRACK();
      if (tracingName) {
        CESIUM_TRACE_END_IN_TRACK(tracingName);
      }
      return f(result);
    };
#else
    return Impl::unwrapFuture<Func, T>(std::forward<Func>(f));
#endif
  }
};

template <> struct WithTracing<void> {
  template <typename Func>
  static auto
  begin([[maybe_unused]] const char* tracingName, [[maybe_unused]] Func&& f) {
#if CESIUM_TRACING_ENABLED
    return [tracingName, CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()]() mutable {
      CESIUM_TRACE_USE_CAPTURED_TRACK();
      if (tracingName) {
        CESIUM_TRACE_END_IN_TRACK(tracingName);
      }
    };
#else
    return Impl::unwrapFuture<Func>(std::forward<Func>(f));
#endif
  }

  template <typename Func>
  static auto end([[maybe_unused]] const char* tracingName, Func&& f) {
#if CESIUM_TRACING_ENABLED
    return [tracingName,
            f = Impl::unwrapFuture<Func>(std::forward<Func>(f)),
            CESIUM_TRACE_LAMBDA_CAPTURE_TRACK()]() mutable {
      CESIUM_TRACE_USE_CAPTURED_TRACK();
      if (tracingName) {
        CESIUM_TRACE_END_IN_TRACK(tracingName);
      }
      return f();
    };
#else
    return Impl::unwrapFuture<Func>(std::forward<Func>(f));
#endif
  }
};

// With a void Future, shared and non-shared are identical.
template <> struct WithTracingShared<void> : public WithTracing<void> {};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
