#pragma once

#include "CesiumAsync/Impl/cesium-async++.h"

namespace CesiumAsync {

template <class T> class Future;

namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename T> struct RemoveFuture { typedef T type; };
template <typename T> struct RemoveFuture<Future<T>> { typedef T type; };
template <typename T> struct RemoveFuture<const Future<T>> { typedef T type; };
template <typename T> struct RemoveFuture<async::task<T>> { typedef T type; };
template <typename T> struct RemoveFuture<const async::task<T>> {
  typedef T type;
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync
