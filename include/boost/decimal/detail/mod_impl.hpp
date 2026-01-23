// Copyright 2023 - 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_MOD_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_MOD_IMPL_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/u256.hpp>
#include <boost/decimal/detail/promotion.hpp>
#include <boost/decimal/detail/type_traits.hpp>

namespace boost {
namespace decimal {
namespace detail {

template <typename T>
constexpr auto get_pow_10(const std::size_t value) noexcept -> T
{
    return pow10<T>(value);
}

template <>
constexpr auto get_pow_10(const std::size_t value) noexcept -> int128::uint128_t
{
    return pow10_u128(value);
}

template <>
constexpr auto get_pow_10(const std::size_t value) noexcept -> u256
{
    return pow10_256(value);
}

template <typename DecimalType, typename ComponentsType>
constexpr auto generic_mod_impl(const DecimalType& lhs, const ComponentsType& lhs_components,
                                const DecimalType& rhs, const ComponentsType& rhs_components,
                                const DecimalType& q, DecimalType& r) noexcept -> void
{
    static_assert(is_decimal_floating_point_v<DecimalType>, "Decimal type must be a decimal floating point type");
    static_assert(is_decimal_floating_point_components_v<ComponentsType>, "Components must be a decimal floating point components");

    using promoted_integer_type = std::conditional_t<decimal_val_v<DecimalType> < 64, std::uint64_t,
                                     std::conditional_t<decimal_val_v<DecimalType> < 128, int128::uint128_t, u256>>;

    const auto common_exp {std::min(lhs_components.exp, rhs_components.exp)};
    const auto lhs_scaling {lhs_components.exp - common_exp};
    const auto rhs_scaling {rhs_components.exp - common_exp};

    // An approximation of the most digits we can hold without actually having to count the digits
    constexpr auto max_scaling {std::numeric_limits<promoted_integer_type>::digits10 - std::numeric_limits<DecimalType>::digits10};

    if (std::max(lhs_scaling, rhs_scaling) <= max_scaling)
    {
        BOOST_DECIMAL_ASSERT(lhs_scaling >= 0);
        BOOST_DECIMAL_ASSERT(rhs_scaling >= 0);

        promoted_integer_type scaled_lhs {lhs_components.sig};
        promoted_integer_type scaled_rhs {rhs_components.sig};

        scaled_lhs *= get_pow_10<promoted_integer_type>(static_cast<std::size_t>(lhs_scaling));
        scaled_rhs *= get_pow_10<promoted_integer_type>(static_cast<std::size_t>(rhs_scaling));

        const auto remainder_coeff {scaled_lhs % scaled_rhs};

        r = DecimalType{remainder_coeff, common_exp, lhs_components.sign};
    }
    else
    {
        constexpr DecimalType zero {0, 0};

        // https://en.cppreference.com/w/cpp/numeric/math/fmod
        const auto q_trunc {q > zero ? floor(q) : ceil(q)};
        r = lhs - (q_trunc * rhs);
    }
}

} // namespace detail
} // namespace decimal
} // namespace boost

#endif // BOOST_DECIMAL_DETAIL_MOD_IMPL_HPP
