#pragma once

#include "CesiumAsync/Impl/unwrapFuture.h"
#include "CesiumUtility/Tracing.h"

namespace CesiumAsync {
namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename Func, typename T> struct WithTracing {
  static auto wrap([[maybe_unused]] const char* tracingName, Func&& f) {
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

template <typename Func> struct WithTracing<Func, void> {
  static auto wrap([[maybe_unused]] const char* tracingName, Func&& f) {
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

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
