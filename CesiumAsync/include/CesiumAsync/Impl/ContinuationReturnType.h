#pragma once

#include <type_traits>

namespace CesiumAsync {
namespace Impl {
// Begin omitting doxgen warnings for Impl namespace
//! @cond Doxygen_Suppress

template <typename Func, typename T> struct ContinuationReturnType {
  typedef typename std::invoke_result<Func, T>::type type;
};

template <typename Func> struct ContinuationReturnType<Func, void> {
  typedef typename std::invoke_result<Func>::type type;
};

//! @endcond
// End omitting doxgen warnings for Impl namespace
} // namespace Impl
} // namespace CesiumAsync