#pragma once

#include <type_traits>

namespace details_is_template_base_of
{
template <template <typename...> class C, typename...Ts>
std::true_type is_template_base_of_impl(const C<Ts...>*);

template <template <typename...> class C>
std::false_type is_template_base_of_impl(...);
}

template <template <typename...> class Base, typename Derived>
struct is_template_base_of :
    std::integral_constant<
        bool,
        decltype(details_is_template_base_of::is_template_base_of_impl<Base>(std::declval<Derived*>()))::value
    >
{};
