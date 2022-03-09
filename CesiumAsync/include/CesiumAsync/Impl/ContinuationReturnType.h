#pragma once

#include <type_traits>

namespace CesiumAsync {
namespace CesiumImpl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename Func, typename T> struct ContinuationReturnType {
  using type = typename std::invoke_result<Func, T>::type;
};

template <typename Func> struct ContinuationReturnType<Func, void> {
  using type = typename std::invoke_result<Func>::type;
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace CesiumImpl
} // namespace CesiumAsync
