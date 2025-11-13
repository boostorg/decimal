// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// This file briefly demonstrates the difference in results when parsing monetary values between float and decimal32_t

#include "where_file.hpp"
#include <boost/decimal/decimal32_t.hpp>    // For type decimal32_t
#include <boost/decimal/charconv.hpp>       // For support to <charconv>
#include <boost/decimal/iostream.hpp>       // For decimal support to <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <limits>
#include <numeric>
#include <iostream>
#include <iomanip>

using boost::decimal::decimal32_t;

template <typename T>
T parse_opening_price(const std::string& line);

template <>
float parse_opening_price<float>(const std::string& line)
{
    const auto result {std::stof(line)};
    return result;
}

template <>
decimal32_t parse_opening_price<decimal32_t>(const std::string& line)
{
    decimal32_t result;
    const auto r = from_chars(line, result);

    if (!r)
    {
        // LCOV_EXCL_START
        result = std::numeric_limits<decimal32_t>::quiet_NaN();
        BOOST_DECIMAL_THROW_EXCEPTION(std::invalid_argument("Parsing has failed"));
        // LCOV_EXCL_STOP
    }

    return result;
}

template <typename T>
auto parse_csv_line(const std::string& line) -> T
{
    std::stringstream ss(line);
    std::string token;
    std::string date;

    std::getline(ss, date, ',');
    std::getline(ss, token, ',');

    return parse_opening_price<T>(token);
}

int main()
{
    // Open and read the CSV file
    std::ifstream file(boost::decimal::where_file("AAPL.csv"));
    std::string line;

    // Skip header line
    std::getline(file, line);

    std::vector<decimal32_t> decimal_opening_prices;
    std::vector<float> float_opening_prices;

    while (std::getline(file, line))
    {
        decimal_opening_prices.emplace_back(parse_csv_line<decimal32_t>(line));
        float_opening_prices.emplace_back(parse_csv_line<float>(line));
    }

    const std::string ms_result {"52151.99"};
    const auto decimal_sum {std::accumulate(decimal_opening_prices.begin(), decimal_opening_prices.end(), decimal32_t{0})};
    const auto float_sum {std::accumulate(float_opening_prices.begin(), float_opening_prices.end(), float{0})};

    std::cout << std::setprecision(std::numeric_limits<float>::digits10 + 1)
              << "Number of data points: " << decimal_opening_prices.size() << '\n'
              << "    Sum from MS Excel: " << ms_result << '\n'
              << "Sum using decimal32_t: " << decimal_sum << '\n'
              << "      Sum using float: " << float_sum << std::endl;
}
