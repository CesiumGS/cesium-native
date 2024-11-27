#pragma once

#include <swl/variant.hpp>

#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>

namespace CesiumUtility {

// template <typename... TTypesInVariant>
// using EnableIfTriviallyDestructible = std::enable_if_t<std::is_trivially>,
//       bool > ;

template <typename... T> using SimpleVariant = swl::variant<T...>;
template <typename... T> using FullVariant = std::variant<T...>;

template <typename... T>
constexpr bool isSimpleVariant =
    std::conjunction_v<std::is_trivially_destructible<T>...>;

template <bool isSimple, typename... T>
struct VariantSpecializer;

template <typename... T>
struct VariantSpecializer<true, T...> {
  using Type = SimpleVariant<T...>;
};

template <typename... T>
struct VariantSpecializer<false, T...> {
  using Type = FullVariant<T...>;
};

template <typename... T>
using Variant = VariantSpecializer<isSimpleVariant<T...>, T...>::Type;

// template <typename... TTypesInVariant>
// using EnableIfVariantsAreDifferent = std::enable_if_t<
//     !std::is_same_v<
//         SimpleVariant<TTypesInVariant...>,
//         FullVariant<TTypesInVariant...>>,
//     bool>;

// visit
// template <typename TVisitor, typename TVariant>
// decltype(auto) visit(TVisitor&& visitor, TVariant&& variant) {
//   return visit(
//       std::forward<TVisitor>(visitor),
//       std::forward<TVariant>(variant));
// }

// template <typename TVisitor, typename... TTypesInVariant>
// decltype(auto)
// visit(TVisitor&& visitor, const Variant<TTypesInVariant...>& variant) {
//   return std::visit<TVisitor, TTypesInVariant...>(
//       std::forward<TVisitor>(visitor),
//       variant);
// }

// template <typename TVisitor, typename... TTypesInVariant>
// decltype(auto) visit(TVisitor&& visitor, Variant<TTypesInVariant...>&
// variant) {
//   return std::visit<TVisitor, TTypesInVariant...>(
//       std::forward<TVisitor>(visitor),
//       variant);
// }

// // Simple visit
// template <typename TVisitor, typename... TTypesInVariant>
// decltype(auto)
// visit(TVisitor&& visitor, SimpleVariant<TTypesInVariant...>&& variant) {
//   return visit(std::forward<TVisitor>(visitor), std::move(variant));
// }

// template <typename TVisitor, typename... TTypesInVariant>
// decltype(auto)
// visit(TVisitor&& visitor, const SimpleVariant<TTypesInVariant...>& variant) {
//   return visit(std::forward<TVisitor>(visitor), variant);
// }

// template <typename TVisitor, typename... TTypesInVariant>
// decltype(auto)
// visit(TVisitor&& visitor, SimpleVariant<TTypesInVariant...>& variant) {
//   return visit(std::forward<TVisitor>(visitor), variant);
// }

// // Full visit
// template <
//     typename TVisitor,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto)
// visit(TVisitor&& visitor, FullVariant<TTypesInVariant...>&& variant) {
//   return visit(std::forward<TVisitor>(visitor), std::move(variant));
// }

// template <
//     typename TVisitor,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto)
// visit(TVisitor&& visitor, const FullVariant<TTypesInVariant...>& variant) {
//   return visit(std::forward<TVisitor>(visitor), variant);
// }

// template <
//     typename TVisitor,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// auto visit(TVisitor&& visitor, FullVariant<TTypesInVariant...>& variant) {
//   return visit(std::forward<TVisitor>(visitor), variant);
// }

// Simple get by index
// template <std::size_t I, typename... TTypesInVariant>
// decltype(auto) get(SimpleVariant<TTypesInVariant...>&& variant) {
//   return get<I, TTypesInVariant...>(std::move(variant));
// }

// template <std::size_t I, typename... TTypesInVariant>
// decltype(auto) get(SimpleVariant<TTypesInVariant...>& variant) {
//   return get<I, TTypesInVariant...>(variant);
// }

// template <std::size_t I, typename... TTypesInVariant>
// decltype(auto) get(const SimpleVariant<TTypesInVariant...>& variant) {
//   return get<I, TTypesInVariant...>(variant);
// }

// // Simple get by type
// template <typename T, typename... TTypesInVariant>
// decltype(auto) get(SimpleVariant<TTypesInVariant...>&& variant) {
//   return get<T, TTypesInVariant...>(std::move(variant));
// }

// template <typename T, typename... TTypesInVariant>
// decltype(auto) get(SimpleVariant<TTypesInVariant...>& variant) {
//   return get<T, TTypesInVariant...>(variant);
// }

// template <typename T, typename... TTypesInVariant>
// decltype(auto) get(const SimpleVariant<TTypesInVariant...>& variant) {
//   return get<T, TTypesInVariant...>(variant);
// }

// // Full get by index
// template <
//     std::size_t I,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto) get(FullVariant<TTypesInVariant...>&& variant) {
//   return get<I, TTypesInVariant...>(std::move(variant));
// }

// template <
//     std::size_t I,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto) get(FullVariant<TTypesInVariant...>& variant) {
//   return get<I, TTypesInVariant...>(variant);
// }

// template <
//     std::size_t I,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto) get(const FullVariant<TTypesInVariant...>& variant) {
//   return get<I, TTypesInVariant...>(variant);
// }

// // Full get by type
// template <
//     typename T,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto) get(FullVariant<TTypesInVariant...>&& variant) {
//   return get<T, TTypesInVariant...>(std::move(variant));
// }

// template <
//     typename T,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto) get(FullVariant<TTypesInVariant...>& variant) {
//   return get<T, TTypesInVariant...>(variant);
// }

// template <
//     typename T,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto) get(const FullVariant<TTypesInVariant...>& variant) {
//   return get<T, TTypesInVariant...>(variant);
// }

// // Simple get_if by index
// template <std::size_t I, typename... TTypesInVariant>
// decltype(auto) get_if(SimpleVariant<TTypesInVariant...>* pVariant) noexcept {
//   return get_if<I, TTypesInVariant...>(pVariant);
// }

// template <std::size_t I, typename... TTypesInVariant>
// decltype(auto)
// get_if(const SimpleVariant<TTypesInVariant...>* pVariant) noexcept {
//   return get_if<I, TTypesInVariant...>(pVariant);
// }

// // Simple get_if by type
// template <typename T, typename... TTypesInVariant>
// decltype(auto) get_if(SimpleVariant<TTypesInVariant...>* pVariant) noexcept {
//   return get_if<T, TTypesInVariant...>(pVariant);
// }

// template <typename T, typename... TTypesInVariant>
// decltype(auto)
// get_if(const SimpleVariant<TTypesInVariant...>* pVariant) noexcept {
//   return get_if<T, TTypesInVariant...>(pVariant);
// }

// // Full get_if by index
// template <
//     std::size_t I,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto) get_if(FullVariant<TTypesInVariant...>* pVariant) noexcept {
//   return get_if<I, TTypesInVariant...>(pVariant);
// }

// template <
//     std::size_t I,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto)
// get_if(const FullVariant<TTypesInVariant...>* pVariant) noexcept {
//   return get_if<I, TTypesInVariant...>(pVariant);
// }

// // Full get_if by type
// template <
//     typename T,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto) get_if(FullVariant<TTypesInVariant...>* pVariant) noexcept {
//   return get_if<T, TTypesInVariant...>(pVariant);
// }

// template <
//     typename T,
//     typename... TTypesInVariant,
//     EnableIfVariantsAreDifferent<TTypesInVariant...> = true>
// decltype(auto)
// get_if(const FullVariant<TTypesInVariant...>* pVariant) noexcept {
//   return get_if<T, TTypesInVariant...>(pVariant);
// }

} // namespace CesiumUtility
