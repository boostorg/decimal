// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt
//
// This example shows how to write and read decimal values to file efficiently

#include <boost/decimal/decimal32_t.hpp>     // For type decimal32_t
#include <boost/decimal/bid_conversion.hpp>  // For to and from BID encoded bits functions
#include <boost/decimal/iostream.hpp>        // For decimal type support to <iostream>
#include <fstream>
#include <random>
#include <array>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cstdio>
#include <string>

int main(int argc, char* argv[])
{
   using boost::decimal::decimal32_t;  // The type decimal32_t

   // The name comes from the executable, because the test harness runs every build of this
   // example from one directory. The standard allows argc to be 0, so a literal stands in then
   const std::string file_name {(argc > 0 ? std::string(argv[0]) : std::string("to_from_file")) + "_values.txt"};

   // First we need to generate some values that we will use for further usage
   // This constructs a decimal32_t from random significand and exponent within the domain of decimal32_t
   std::mt19937_64 rng {42};
   std::uniform_int_distribution<std::int32_t> significand_dist {-9'999'999, 9'999'999};
   std::uniform_int_distribution<std::int32_t> exp_dist {-50, 50};

   std::array<decimal32_t, 10> values;
   for (auto& v : values)
   {
      v = decimal32_t{significand_dist(rng), exp_dist(rng)};
   }

   // Now that we have our random decimal32_ts we will write them to file
   // using their bitwise representations with the to_bid function
   //
   // This allows us to losslessly and rapidly recover them from file
   // It is more efficient than writing the string to file with to_chars,
   // and then recovering via the string constructor or from_chars

   std::ofstream file(file_name, std::ios::binary);
   if (!file.is_open())
   {
      std::cerr << "Failed to open file for writing" << std::endl;
      return 1;
   }

   for (const auto& value : values)
   {
      std::uint32_t bid_value {boost::decimal::to_bid(value)};

      std::cout << " Current value: " << std::dec << value << '\n'
                << "Value as bytes: " << std::hex << bid_value << "\n\n";

      file.write(reinterpret_cast<char*>(&bid_value), sizeof(bid_value));
   }
   file.close();
   if (!file)
   {
      std::cerr << "Failed to write the file" << std::endl;
      return 1;
   }

   // Now that we have written all the values to file we will read them in,
   // and then convert them back them to the decimal values using from_bid
   std::ifstream read_file(file_name, std::ios::binary);
   if (!read_file.is_open())
   {
      std::cerr << "Failed to open file for reading" << std::endl;
      return 1;
   }

   std::array<decimal32_t, 10> recovered_values;
   for (auto& value : recovered_values)
   {
      std::uint32_t bid_value {};
      read_file.read(reinterpret_cast<char*>(&bid_value), sizeof(bid_value));
      value = boost::decimal::from_bid(bid_value);
   }

   const bool read_ok {static_cast<bool>(read_file)};
   read_file.close();
   if (std::remove(file_name.c_str()))
   {
      std::cerr << "Failed to remove file" << std::endl;
   }

   // Verify that we read every value, and that we recovered the same values
   bool success {read_ok};
   for (std::size_t i {}; i < values.size(); ++i)
   {
      if (values[i] != recovered_values[i])
      {
         success = false;
         break;
      }
   }

   if (success)
   {
      std::cout << "Successfully recovered all values from file" << std::endl;
   }
   else
   {
      std::cout << "Error: Some values did not match after recovery" << std::endl;
      return 1;
   }

   return 0;
}
