// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

// Needed for operations with boost math
#define BOOST_DECIMAL_ALLOW_IMPLICIT_INTEGER_CONVERSIONS

#include "where_file.hpp"
#include <boost/decimal.hpp>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// Warning suppression for boost.math
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wfloat-equal"
#  pragma clang diagnostic ignored "-Wsign-conversion"
#  pragma clang diagnostic ignored "-Wundef"
#  pragma clang diagnostic ignored "-Wstring-conversion"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <boost/math/statistics/univariate_statistics.hpp>

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

using namespace boost::decimal;

struct daily_data
{
    std::string date;
    decimal64_t open;
    decimal64_t high;
    decimal64_t low;
    decimal64_t close;
    decimal64_t volume;
};

// Function to split a CSV line into daily_data
auto parse_csv_line(const std::string& line) -> daily_data
{
    std::stringstream ss(line);
    std::string token;
    daily_data data;

    // Parse each column
    std::getline(ss, data.date, ',');
    std::getline(ss, token, ',');
    from_chars(token, data.open);

    std::getline(ss, token, ',');
    from_chars(token, data.high);

    std::getline(ss, token, ',');
    from_chars(token, data.low);

    std::getline(ss, token, ',');
    from_chars(token, data.close);

    std::getline(ss, token, ',');
    from_chars(token, data.volume);

    return data;
}

int main()
{
    std::vector<daily_data> stock_data;

    // Open and read the CSV file
    std::ifstream file(where_file("AAPL.csv"));
    std::string line;

    // Skip header line
    std::getline(file, line);

    // Read data
    while (std::getline(file, line))
    {
        stock_data.push_back(parse_csv_line(line));
    }

    // Get the closing prices for the entire year
    std::vector<decimal64_t> closing_prices;
    for (const auto& day : stock_data)
    {
        closing_prices.emplace_back(day.close);
    }

    const auto mean_closing_price = boost::math::statistics::mean(closing_prices);
    const auto median_closing_price = boost::math::statistics::median(closing_prices);
    const auto variance_closing_price = boost::math::statistics::variance(closing_prices);
    const auto std_dev_closing_price = sqrt(variance_closing_price);

    // 2-Sigma Bollinger Bands
    const auto upper_band = mean_closing_price + 2 * std_dev_closing_price;
    const auto lower_band = mean_closing_price - 2 * std_dev_closing_price;

    std::cout << std::fixed << std::setprecision(2)
              << "  Mean Closing Price: " << mean_closing_price << '\n'
              << "  Standard Deviation: " << std_dev_closing_price << '\n'
              << "Upper Bollinger Band: " << upper_band << '\n'
              << "Lower Bollinger Band: " << lower_band << std::endl;

    //   Mean = 207.21
    // Median = 214.27
    return mean_closing_price > median_closing_price;
}

