#pragma once

#include "CesiumAsync/Impl/ContinuationReturnType.h"
#include "CesiumAsync/Impl/RemoveFuture.h"

namespace CesiumAsync {
namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename Func, typename T> struct ContinuationFutureType {
  typedef typename RemoveFuture<
      typename ContinuationReturnType<Func, T>::type>::type type;
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync