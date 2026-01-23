// Copyright 2023 - 2026 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_DETAIL_MOD_IMPL_HPP
#define BOOST_DECIMAL_DETAIL_MOD_IMPL_HPP

#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/detail/power_tables.hpp>
#include <boost/decimal/detail/int128.hpp>
#include <boost/decimal/detail/u256.hpp>

namespace boost {
namespace decimal {
namespace detail {

template <typename DecimalType, typename ComponentsType>
constexpr auto d32_mod_impl(const DecimalType& lhs, const ComponentsType& lhs_components,
                            const DecimalType& rhs, const ComponentsType& rhs_components,
                            const DecimalType& q, DecimalType& r) noexcept -> void
{
    const auto common_exp {std::min(lhs_components.exp, rhs_components.exp)};
    const auto lhs_scaling {lhs_components.exp - common_exp};
    const auto rhs_scaling {rhs_components.exp - common_exp};

    // An approximation of the most digits we can hold without actually having to count the digits
    constexpr auto max_scaling {std::numeric_limits<std::uint64_t>::digits10 - std::numeric_limits<DecimalType>::digits10};

    if (std::max(lhs_scaling, rhs_scaling) <= max_scaling)
    {
        BOOST_DECIMAL_ASSERT(lhs_scaling >= 0);
        BOOST_DECIMAL_ASSERT(rhs_scaling >= 0);

        std::uint64_t scaled_lhs {lhs_components.sig};
        std::uint64_t scaled_rhs {rhs_components.sig};

        scaled_lhs *= pow10(static_cast<std::uint64_t>(lhs_scaling));
        scaled_rhs *= pow10(static_cast<std::uint64_t>(rhs_scaling));

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
