#pragma once

#include "ContinuationReturnType.h"
#include "RemoveFuture.h"

namespace CesiumAsync {

template <typename T> class Future;

namespace CesiumImpl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename Func, typename T> struct ContinuationFutureType {
  using type = Future<typename RemoveFuture<
      typename ContinuationReturnType<Func, T>::type>::type>;
};

template <typename Func, typename T>
using ContinuationFutureType_t = typename ContinuationFutureType<Func, T>::type;

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace CesiumImpl
} // namespace CesiumAsync
