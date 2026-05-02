
#include <boost/core/lightweight_test.hpp>

// This simulates upstream inclusion of Boost.Log. At the time of
// hash commit e9951622837f63cf0020b22834ad96b01293fd07, this failed.

// This odd-looking test in CI is intended to ensure that such failures
// no longer occur in the future and that the namespace resolution
// remains unequivocal.

namespace boost {
namespace log {

auto do_something() -> void
{
}

} // namespace log
} // namespace boost

#include <boost/decimal/cmath.hpp>

auto main() -> int;

auto main() -> int
{
  using local_decimal_type = boost::decimal::decimal64_t;

  const local_decimal_type arg_dec { 1.23456 };

  const local_decimal_type result1 { acosh(arg_dec) };
  const local_decimal_type result2 { lgamma(arg_dec) };

  BOOST_TEST(result1 > local_decimal_type { 0.0 });
  BOOST_TEST(result2 < local_decimal_type { 0.0 });

  return boost::report_errors();
}
