#pragma once

#include "CesiumAsync/Impl/unwrapFuture.h"
#include "CesiumUtility/Tracing.h"

namespace CesiumAsync {
namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename Func, typename T> struct WithTracing {
  static auto wrap(const char* tracingName, Func&& f) {
#if TRACING_ENABLED
    int64_t tracingID = CesiumUtility::Profiler::instance().getEnlistedID();

    return [tracingID,
            tracingName,
            f = Impl::unwrapFuture<Func, T>(std::forward<Func>(f))](
               T&& result) mutable {
      CESIUM_TRACE_ASYNC_ENLIST(tracingID);
      if (tracingName) {
        CESIUM_TRACE_END_ID(tracingName, tracingID);
      }
      return f(std::move(result));
    };
#else
    return Impl::unwrapFuture<Func, T>(std::forward<Func>(f));
#endif
  }
};

template <typename Func> struct WithTracing<Func, void> {
  static auto wrap(const char* tracingName, Func&& f) {
#if TRACING_ENABLED
    int64_t tracingID = CesiumUtility::Profiler::instance().getEnlistedID();

    return [tracingID,
            tracingName,
            f = Impl::unwrapFuture<Func>(std::forward<Func>(f))]() mutable {
      CESIUM_TRACE_ASYNC_ENLIST(tracingID);
      if (tracingName) {
        CESIUM_TRACE_END_ID(tracingName, tracingID);
      }
      return f();
    };
#else
    return Impl::unwrapFuture<Func, T>(std::forward<Func>(f));
#endif
  }
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
