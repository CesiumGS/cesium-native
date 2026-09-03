#include <cstddef>
#include <tuple>
#include <utility>

namespace CesiumUtility {

namespace CesiumImpl {

/**
 * @brief The type that `std::get<I>` yields for a tuple of type `Tuple`,
 * preserving `Tuple`'s value category and const-ness.
 */
template <typename Tuple, size_t I>
using TupleElementReference = decltype(std::get<I>(std::declval<Tuple>()));

} // namespace CesiumImpl

/**
 * @brief Transforms each element of a tuple by applying a function to it.
 *
 * The function is guaranteed to be applied to each element in order.
 *
 * @tparam Tuple The tuple to transform.
 * @tparam Func The function to apply to each element.
 * @param tuple The tuple whose elements are to be transformed.
 * @param f The function to apply to each element.
 * @return A new tuple with each element transformed by the given function.
 */
template <typename Tuple, typename Func>
auto transformTuple(Tuple&& tuple, Func&& f) {
  constexpr size_t N = std::tuple_size_v<std::remove_reference_t<Tuple>>;

  return [&]<size_t... Is>(std::index_sequence<Is...>) {
    // Use brace-initialization to guarantee arguments are evaluated in order.
    // This is not generally guaranteed by "ordinary" function invocation. For
    // example, func(funcA(), funcB()) could have funcA() _or_ funcB() called
    // first; the order is unspecified.
    // Each element is forwarded individually, rather than forwarding the whole
    // tuple once per element, so that only distinct elements are ever moved.
    std::tuple result{
        f(std::forward<CesiumImpl::TupleElementReference<Tuple, Is>>(
            std::get<Is>(tuple)))...};
    return result;
  }
  (std::make_index_sequence<N>{});
}

} // namespace CesiumUtility
