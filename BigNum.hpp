/*
BigNum++: C++ port of https://github.com/Patashu/break_infinity.js
Slightly modified to increase maximum range by moving negative numbers outside
of the exponent's range (Emin is 0, therefore Emax is higher) Tradeoff: Cannot
store numbers smaller than regular doubles, but those aren't usually needed in
the types of games that would use this library. Numbers <0 with e=0 are valid,
e.g BigNum(0.1, 0)
*/

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cmath>
#include <compare>
#include <cstdint>
#include <format>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

// Define a macro for CPP26 and later for statements
#define CPP26 (__cplusplus >= 202600L)

// Constant precision for serializing
namespace BigNumber {
using namespace std::literals::string_literals;

static constexpr unsigned int SERIAL_PRECISION = 9;
static constexpr char DECIMAL_SEPARATOR = '.';
static constexpr char THOUSANDS_SEPARATOR = ',';

// Formatting context
struct BigNumContext {
  unsigned int max_digits = 10; // Up to how many "real" digits to display
                                // before using scientific notation
  unsigned int print_precision =
      3; // How many fractional digits to display on scientific notation
};
// Global "default" context when none is passed to functions
inline BigNumContext DefaultBigNumContext;

// Precompute powers-of-10 table for performance
static inline constexpr int Pow10TableOffset =
    std::numeric_limits<double>::max_exponent10;
static inline constexpr int Pow10TableSize = 2 * Pow10TableOffset + 1;
static inline constexpr std::array<double, Pow10TableSize>
Pow10_generate_table() {
  std::array<double, Pow10TableSize> table{};
  table[Pow10TableOffset] = 1.0;
  double pos = 1.0;
  for (int i = 1; i <= Pow10TableOffset; ++i) {
    pos *= 10.0;
    table[Pow10TableOffset + i] = pos;       // positive exponents: 10^i
    table[Pow10TableOffset - i] = 1.0 / pos; // negative exponents: 10^(-i)
  }
  return table;
};

class Pow10 {
private:
  Pow10() = delete;

public:
  static inline constexpr std::array<double, Pow10TableSize> Pow10Table =
      Pow10_generate_table();

  // e must be in the range [-offset, offset]
  static inline constexpr std::optional<double> get(int e) {
    if (e < -Pow10TableOffset || e > Pow10TableOffset) {
      return std::nullopt;
    }
    return Pow10Table[e + Pow10TableOffset];
  }
};

template <typename M, typename E> class BigNum_;

template <typename M, typename E>
std::ostream &operator<<(std::ostream &os, const BigNum_<M, E> &bn);

template <typename M, typename E>
std::istream &operator>>(std::istream &is, BigNum_<M, E> &bn);

template <typename M, typename E> class BigNum_ {
public:
  using man_t = M; // mantissa type
  using exp_t = E; // exponent type

  friend std::ostream &operator<< <>(std::ostream &os, const BigNum_<M, E> &bn);
  friend std::istream &operator>> <>(std::istream &is, BigNum_<M, E> &bn);

private:
  man_t m = 0; // mantissa
  exp_t e = 0; // exponent (base 10)
  static_assert(std::is_floating_point_v<man_t>,
                "mantissa must be a floating point type");
  static_assert(std::is_arithmetic_v<exp_t>,
                "exponent must be an arithmetic type");

  static inline constexpr exp_t MAX_DIV_DIFF = 308;
  // helper functions to convert strings to mantissa/exponent
  static inline constexpr man_t strtom(const std::string_view &sv) {
    man_t m;
    auto result = std::from_chars(sv.data(), sv.data() + sv.size(), m);
    if (result.ec != std::errc()) {
      throw std::invalid_argument("Failed to convert string to mantissa: " +
                                  std::string(sv));
    }
    return m;
  }
  static inline constexpr exp_t strtoe(const std::string_view &sv) {
    exp_t e;
    auto result = std::from_chars(sv.data(), sv.data() + sv.size(), e);
    if (result.ec != std::errc()) {
      throw std::invalid_argument("Failed to convert string to exponent: " +
                                  std::string(sv));
    }
    return e;
  }

  // convert the number to a full-precision string
  static constexpr std::string to_string_full(const man_t &value) {
    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<man_t>::digits10 + 1) << value;
    return out.str();
  }

  // convert double to string, rounding down and up to specific precision
  static constexpr std::string to_string_floor(const double &value,
                                               const int &precision) {
    // Assumes value is normalized to 1 digit before the decimal point
    // (|value| < 10)
    assert(value > -10 && value < 10 && "Value must be normalized");
    double scale = std::pow(10.0, precision);
    double truncated_value = std::floor(value * scale) / scale;

    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << truncated_value;
    std::string out_str = out.str();

    // If necessary, round down to always return 1 digit before the decimal
    // point This is to avoid rounding errors when the number is close to 10
    // Should always be correct given our assumption that |value| < 10
    if (out_str.substr(0, 3) == "10"s + DECIMAL_SEPARATOR) {
      out_str = "9"s + DECIMAL_SEPARATOR + std::string(precision, '9');
    } else if (out_str.substr(0, 4) == "-10"s + DECIMAL_SEPARATOR) {
      out_str = "-9"s + DECIMAL_SEPARATOR + std::string(precision, '9');
    }
    return out_str;
  }

// Fallback to std::log10 if not on C++26
#if !CPP26
  static int _log10(double x) {
    assert(x > 0.0 && "x must be positive for log10");
    int exponent = 0;
    while (x >= 10.0) {
      x /= 10.0;
      ++exponent;
    }
    while (x < 1.0) {
      x *= 10.0;
      --exponent;
    }
    return exponent;
  }
#endif

  constexpr BigNum_(const man_t mantissa, const exp_t exponent, bool normalize)
      : m(mantissa), e(exponent) {
    if (normalize)
      this->normalize();
  }

  constexpr void parseStr(const std::string_view &sv) {
    try {
      size_t pos = sv.find('e');
      if (pos != std::string::npos) {
        m = strtom(sv.substr(0, pos));
        e = strtoe(sv.substr(pos + 1));
      } else {
        m = strtom(sv);
        e = 0;
      }
      normalize();
    } catch (const std::invalid_argument &ex) {
      throw std::invalid_argument(std::string("Failed to parse number: ") +
                                  ex.what());
    }
  }

  constexpr void set(const BigNum_ &other) {
    m = other.m;
    e = other.e;
  }

public:
  static constexpr BigNum_ inf() {
    return BigNum_(std::numeric_limits<man_t>::infinity(), 0, false);
  }
  static constexpr BigNum_ nan() {
    return BigNum_(std::numeric_limits<man_t>::quiet_NaN(), 0, false);
  }
  static constexpr BigNum_ max() {
    return BigNum_(std::nextafter(10.0, 0.0), std::numeric_limits<exp_t>::max(),
                   false);
  }
  static constexpr BigNum_ min() {
    return BigNum_(std::nextafter(-10.0, 0.0),
                   std::numeric_limits<exp_t>::max(), false);
  }

  man_t getM() const { return m; }
  exp_t getE() const { return e; }

  constexpr BigNum_(const man_t mantissa, const exp_t exponent = 0) {
    m = mantissa;
    e = exponent;
    normalize();
  }

  constexpr BigNum_(const std::string_view &str) { parseStr(str); }

  // Default methods to satisfy concepts
  constexpr BigNum_() : m(0), e(0) { normalize(); } // Default constructor
  BigNum_(const BigNum_ &) = default;               // Copy constructor
  BigNum_ &operator=(const BigNum_ &) = default;    // Copy assignment
  BigNum_(BigNum_ &&) = default;                    // Move constructor
  BigNum_ &operator=(BigNum_ &&) = default;         // Move assignment

  ~BigNum_() = default; // Destructor

  // Normalization: mantissa set in range (-10, 10) if abs() > 1, else e=0
  constexpr void normalize() {
    if (*this == max() || *this == min()) {
      return;
    }
    if (std::isnan(m)) {
      e = 0;
      return;
    }
    if (std::isinf(m)) {
      e = 0;
      return;
    }
    if (m == 0) {
      e = 0;
      return;
    }
    if (std::abs(m) < 1 && e == 0) {
      return;
    }

    // Start normalization
    int n_log;
#if !CPP26
    n_log = std::max(_log10(std::abs(m)), 0);
#else
    n_log = std::max(static_cast<int>(std::floor(std::log10(std::abs(m)))), 0);
#endif

    // if (n_log < 0) { n_log = 0; }
    m = m / (*Pow10::get(n_log));
    e += n_log;

    // Clamp between max and min
    if (*this > max()) {
      set(max());
    }
    if (*this < min()) {
      set(min());
    }

    // Disregard fractional part if exponent is under mantissa's max decimal
    // precision
    if (e < std::numeric_limits<man_t>::max_digits10) {
      double target_precision = Pow10::get(e).value_or(1.0);
      m = std::round(m * target_precision) / target_precision;
      // m = std::floor(m * target_precision) / target_precision;
    }
  }

  // Arithmetic operations
  constexpr BigNum_ add(const BigNum_ &b) const {

    // Handle special cases early
    auto m_inf = std::numeric_limits<man_t>::infinity();
    if (m == m_inf || b.m == m_inf) {
      return inf();
    }
    if (std::isnan(m) || std::isnan(b.m)) {
      return nan();
    }

    // Handle max and min cases early
    if (*this == max() && b.m > 0.0f) {
      return max();
    }
    if (m > 0.0f && b == max()) {
      return max();
    }
    if (*this == min() && b.m < 0.0f) {
      return min();
    }
    if (m < 0.0f && b == min()) {
      return min();
    }

    // Handle simple case: both exponents are zero
    if (e == 0 && b.e == 0) {
      return BigNum_(m + b.m, 0);
    }

    // Handle general case
    bool this_is_bigger = e > b.e;
    exp_t delta = this_is_bigger ? e - b.e : b.e - e;
    man_t m2;
    exp_t e2;
    if (delta > 14) {
      m2 = this_is_bigger ? m : b.m;
      e2 = this_is_bigger ? e : b.e;
    } else if (this_is_bigger) {
      m2 = m * (*Pow10::get(delta)) + b.m;
      e2 = b.e;
    } else {
      m2 = m + b.m * (*Pow10::get(delta));
      e2 = e;
    }

    return BigNum_(m2, e2);
  }

  constexpr BigNum_ sub(const BigNum_ &b) const {
    return add(BigNum_(b.m * -1, b.e));
  }

  constexpr BigNum_ mul(const BigNum_ &b) const {
    return BigNum_(m * b.m, e + b.e);
  }

  constexpr BigNum_ div(const BigNum_ &b) const {
    if (b.m == 0) {
      return nan();
    }

    if ((b.e > e) && (b.e - e >= MAX_DIV_DIFF)) {
      return BigNum_(static_cast<man_t>(0));
    }

    // Prevent underflow for unsigned exponent
    if (b.e > e) {
      // Result will be < 1, store as regular double with e=0
      man_t result_m = m / b.m;
      int exp_diff = static_cast<int>(b.e - e);

      auto pow_val = Pow10::get(-exp_diff);
      if (pow_val) {
        result_m *= *pow_val;
      } else {
        return BigNum_(static_cast<man_t>(0)); // Too small
      }

      return BigNum_(result_m, 0, false); // Don't normalize
    }

    return BigNum_(m / b.m, e - b.e);
  }

  constexpr BigNum_ abs() const { return BigNum_(std::abs(m), e); }

  constexpr BigNum_ negate() const {
    return mul(BigNum_(static_cast<man_t>(-1)));
  }

  constexpr BigNum_ &operator+=(const BigNum_ &b) {
    bool this_is_bigger = e > b.e;
    exp_t delta = this_is_bigger ? e - b.e : b.e - e;
    static constexpr auto DELTA_MAX = 14;
    if (delta > DELTA_MAX) { // If the number is small enough to be considered
                             // insignificant (calculated to be about 1e14 times
                             // smaller), ignore it
      m = this_is_bigger ? m : b.m;
      e = this_is_bigger ? e : b.e;
    } else if (this_is_bigger) {
      m = m * (*Pow10::get(delta)) + b.m;
      e = b.e;
    } else {
      m = m + b.m * (*Pow10::get(delta));
      // e = e;
    }
    normalize();
    return *this;
  }

  constexpr BigNum_ &operator*=(const BigNum_ &b) {
    m *= b.m;
    e += b.e;
    normalize();
    return *this;
  }

  constexpr BigNum_ &operator/=(const BigNum_ &b) {
    if (b.m == 0) {
      m = nan().m;
      e = nan().e;
    } else if ((b.e > e) && (b.e - e >= MAX_DIV_DIFF)) {
      m = 0;
      e = 0;
    } else if (b.e > e) {
      // Result will be < 1
      m /= b.m;
      int exp_diff = static_cast<int>(b.e - e);
      auto pow_val = Pow10::get(-exp_diff);
      if (pow_val) {
        m *= *pow_val;
      } else {
        m = 0;
      }
      e = 0;
    } else {
      m /= b.m;
      e -= b.e;
    }
    normalize();
    return *this;
  }

  // Operator overloads
  constexpr BigNum_ operator+(const BigNum_ &other) const { return add(other); }
  constexpr BigNum_ operator+(const std::string_view &other) const {
    return add(BigNum_(other));
  }
  constexpr BigNum_ operator+(const man_t other) const {
    return add(BigNum_(other));
  }
  constexpr BigNum_ operator-(const BigNum_ &other) const { return sub(other); }
  constexpr BigNum_ operator-(const std::string_view &other) const {
    return sub(BigNum_(other));
  }
  constexpr BigNum_ operator-(const man_t other) const {
    return sub(BigNum_(other));
  }
  constexpr BigNum_ operator*(const BigNum_ &other) const { return mul(other); }
  constexpr BigNum_ operator*(const std::string_view &other) const {
    return mul(BigNum_(other));
  }
  constexpr BigNum_ operator*(const man_t other) const {
    return mul(BigNum_(other));
  }
  constexpr BigNum_ operator/(const BigNum_ &other) const { return div(other); }
  constexpr BigNum_ operator/(const std::string_view &other) const {
    return div(BigNum_(other));
  }
  constexpr BigNum_ operator/(const man_t other) const {
    return div(BigNum_(other));
  }
  constexpr BigNum_ operator-() const { return negate(); }
  constexpr BigNum_ &operator+=(const std::string_view &b) {
    return *this += BigNum_(b);
  }
  constexpr BigNum_ &operator+=(const man_t b) { return *this += BigNum_(b); }
  constexpr BigNum_ &operator-=(const BigNum_ &b) {
    return *this += BigNum_(b.m * -1, b.e);
  }
  constexpr BigNum_ &operator-=(const std::string_view &b) {
    return *this -= BigNum_(b);
  }
  constexpr BigNum_ &operator-=(const man_t b) { return *this -= BigNum_(b); }
  constexpr BigNum_ &operator*=(const std::string_view &b) {
    return *this *= BigNum_(b);
  }
  constexpr BigNum_ &operator*=(const man_t b) { return *this *= BigNum_(b); }
  constexpr BigNum_ &operator/=(const std::string_view &b) {
    return *this /= BigNum_(b);
  }
  constexpr BigNum_ &operator/=(const man_t b) { return *this /= BigNum_(b); }
  constexpr BigNum_ &operator++() {
    return *this += BigNum_(static_cast<man_t>(1));
  }
  constexpr BigNum_ operator++(int) {
    BigNum_ temp(*this);
    *this += BigNum_(static_cast<man_t>(1));
    return temp;
  }
  constexpr BigNum_ &operator--() {
    return *this -= BigNum_(static_cast<man_t>(1));
  }
  constexpr BigNum_ operator--(int) {
    BigNum_ temp(*this);
    *this -= BigNum_(static_cast<man_t>(1));
    return temp;
  }

  // Comparison operations
  constexpr bool is_positive() const { return m >= 0; }
  constexpr bool is_negative() const { return m < 0; }
  constexpr bool is_inf() const { return std::isinf(m); }
  constexpr bool is_nan() const { return std::isnan(m); }
  static constexpr BigNum_ &max(BigNum_ &a, BigNum_ &b) {
    return a > b ? a : b;
  }
  static constexpr BigNum_ &min(BigNum_ &a, BigNum_ &b) {
    return a < b ? a : b;
  }

  constexpr std::partial_ordering operator<=>(const BigNum_ &b) const {
    if (is_nan() || b.is_nan())
      return std::partial_ordering::unordered;

    if (is_inf() && b.is_inf())
      return std::partial_ordering::equivalent;

    if (m == b.m && e == b.e)
      return std::partial_ordering::equivalent;

    const bool a_pos = is_positive();
    const bool b_pos = b.is_positive();

    if (a_pos && !b_pos)
      return std::partial_ordering::greater;
    if (!a_pos && b_pos)
      return std::partial_ordering::less;

    // At this point: both have the same sign (positive or negative)
    if (a_pos) {
      if (e > b.e)
        return std::partial_ordering::greater;
      if (e < b.e)
        return std::partial_ordering::less;
      if (m > b.m)
        return std::partial_ordering::greater;
      return std::partial_ordering::less; // m != b.m, and m < b.m
    } else {
      if (e > b.e)
        return std::partial_ordering::less;
      if (e < b.e)
        return std::partial_ordering::greater;
      if (m > b.m)
        return std::partial_ordering::less;
      return std::partial_ordering::greater; // m != b.m, and m < b.m
    }
  }

  // Exact bitwise comparison
  constexpr bool operator==(const BigNum_ &other) const {
    return (e == other.e) && (m == other.m);
  }

  // More "relaxed" equality comparison, assumes normalized
  constexpr bool approximately_equal(const BigNum_ &other,
                                     double tolerance = 1e-9) const {

    if (e != other.e) {
      return false;
    }
    if (m == other.m) {
      return true;
    }

    double diff = std::abs(m - other.m);
    double larger = std::max(std::abs(m), std::abs(other.m));

    // Use absolute tolerance for very small numbers
    const double abs_tol = 1e-15;
    return diff <= std::max(tolerance * larger, abs_tol);
  }

  constexpr std::partial_ordering
  operator<=>(const std::string_view &other) const {
    return *this <=> BigNum_(other);
  }
  constexpr std::partial_ordering operator<=>(const man_t other) const {
    return *this <=> BigNum_(other);
  }

  // Conversion methods
  std::string to_string(const unsigned int &precision =
                            DefaultBigNumContext.print_precision) const {
    if (this->is_inf()) {
      return "inf";
    }
    if (this->is_nan()) {
      return "nan";
    }

    // Handle small numbers directly
    if (e == 0) {

      // Round down to specified precision
      double scale = *Pow10::get(precision);
      double rounded = std::floor(m * scale) / scale;

      // Use std::format for fixed precision and predictable output
      std::string str = std::format("{:.{}f}", rounded, precision);

      // Remove trailing zeroes and the decimal point if it's the last character
      size_t last_nonzero_pos = str.find_last_not_of('0');
      if (last_nonzero_pos != std::string::npos) {
        if (str[last_nonzero_pos] == '.') {
          // Remove the decimal point if it's the last character after trimming
          // zeros
          str.resize(last_nonzero_pos);
        } else {
          // Trim trailing zeros
          str.resize(last_nonzero_pos + 1);
        }
      }

      return str;
    }

    // Can this number be fully displayed as a string <= max_digits long?
    // Assumes m and e are already normalized
    unsigned int max_digits =
        std::max(precision + 1, DefaultBigNumContext.max_digits);
    if (this->e < max_digits - 1) {
      std::string str = to_string_full(m);

      // calculate new length based on
      exp_t newLen = std::min(static_cast<exp_t>(max_digits), e + 1) +
                     (str[0] == '-' ? 1 : 0);

      // Remove the decimal separator if it exists (and isn't small number
      // <1)
      str.erase(std::remove_if(str.begin(), str.end(),
                               [](char c) { return c == DECIMAL_SEPARATOR; }),
                str.end());

      // If the string is shorter than the desired length, pad with zeros
      if (str.length() < newLen) {
        str += std::string(newLen - str.length(), '0');
      } else {
        // If the string is longer than the desired length, truncate it,
        // and round the last digit if necessary
        bool round_up = false;

        int first_unused_digit_pos = (str.length() > newLen) ? newLen : -1;
        int first_unused_digit = (first_unused_digit_pos > 0)
                                     ? int(str[first_unused_digit_pos] - '0')
                                     : -1;

        if (first_unused_digit >= 5) {
          // Round up the last digit
          round_up = true;
        }
        str = str.substr(0, newLen);
        if (round_up) {
          assert(!str.empty() && "String should not be empty after truncation");
          str[str.length() - 1] += 1;
        }
      }
      return str;
    }

    // Otherwise, use scientific notation
    std::ostringstream out;
    // out << std::fixed << std::setprecision(precision) << m;

    // Handle max() and min() specifically due to rounding issues
    // std::string m_str;
    // if (*this == max()) { m_str = std::string("9.") +
    // std::string(precision, '9'); } else if (*this == min()) { m_str =
    // std::string("-9.") + std::string(precision, '9'); } else { m_str =
    // to_string_floor(m, precision); }

    // Trim the result to the desired precision
    // std::string m_str = to_string_floor(m, 2);
    // size_t dot_pos = m_str.find('.');
    // if (dot_pos != std::string::npos && dot_pos + precision + 1 <
    // m_str.size()) {
    //     m_str = m_str.substr(0, dot_pos + precision + 1);
    // }
    std::string m_str = to_string_floor(m, precision);
    out << m_str;

    if (e != 0) {
      out << "e" << e;
    }

    return out.str();
  }

  // Pretty string: 1234567 -> 1,234,567
  // Scientific notation is not affected
  std::string to_pretty_string(const unsigned int &precision =
                                   DefaultBigNumContext.print_precision) const {
    std::string str = to_string(precision);

    // Early exit if in scientific notation or if the number is too small
    if (str.contains('e') || str.contains(DECIMAL_SEPARATOR)) {
      return str;
    }
    if (str.length() < 4) {
      return str;
    }

    // Insert thousands separators
    for (size_t i = str.length() - 3; i > 0; i -= 3) {
      str.insert(i, 1, THOUSANDS_SEPARATOR);
    }
    return str;
  }

  // Standard methods for (de)serialization
  std::string serialize() const { return to_string(SERIAL_PRECISION); }

  static BigNum_ deserialize(const std::string_view &str) {
    return BigNum_(str);
  }

  // Returns number as intmax_t, or nullopt if the number is too large
  constexpr std::optional<intmax_t> to_number() const {
    int total_digits = e + std::log10(std::abs(m)) + 1;
    if (total_digits > std::numeric_limits<intmax_t>::digits10) {
      // std::cerr << "Number is too large to convert to intmax_t: " <<
      // this->to_string() << std::endl;
      return std::nullopt;
    }

    auto pow = Pow10::get(e);
    if (!pow) {
      // std::cerr << "Pow out of bounds" << std::endl;
      return std::nullopt;
    }
    return static_cast<intmax_t>(m * (*pow));
  }

  // Mathematical operations

  // Returns log10(num), or nullopt if the result would be too large
  constexpr std::optional<double> log10() const {
    if (std::numeric_limits<double>::max() - e < std::log10(m)) {
      return std::nullopt;
    }
    return e + std::log10(m);
  }

  // Returns num^power
  constexpr BigNum_ pow(double power) const {
    // Special cases
    if (power == 0.0) {
      return BigNum_(static_cast<man_t>(1));
    }
    if (m == 0) {
      if (power < 0) {
        throw std::domain_error("Cannot raise 0 to a negative power");
      }
      return BigNum_(static_cast<man_t>(0));
    }

    // When the mantissa is negative
    if (m < 0) {
      // Check if power is effectively an integer
      bool is_integer_power = std::abs(power - std::round(power)) < 1e-10;

      if (!is_integer_power) {
        throw std::domain_error("Non-integer powers of negative "
                                "numbers result in complex values");
      }

      // Handle integer powers of negative numbers
      if (std::fmod(std::round(power), 2.0) == 0.0) {
        return BigNum_(-m, e).pow(power); // Even power
      }
      return BigNum_(-m, e).pow(power).negate(); // Odd power
    }

    // Calculate using logarithms
    auto log = log10();
    if (!log) {
      // std::cerr << "Logarithm out of bounds" << std::endl;
      return BigNum_(static_cast<man_t>(0));
    }

    // Calculate new logarithm
    double new_log = static_cast<double>(*log) * power;

    // If result would be small (< 1), handle specially
    if (new_log < 0) {
      man_t result = static_cast<man_t>(std::pow(10.0, new_log));
      return BigNum_(result, 0, false); // Store as regular double
    }

    // Split into mantissa and exponent
    man_t m2 = static_cast<man_t>(std::pow(10, std::fmod(new_log, 1.0)));
    exp_t e2 = static_cast<exp_t>(std::floor(new_log));

    return BigNum_(m2, e2);
  }

  // Integer power overload - just calls the double version
  constexpr BigNum_ pow(intmax_t power) const {
    return pow(static_cast<double>(power));
  }

  // Returns num^(1/n), aka the nth root
  constexpr BigNum_ root(intmax_t n) const {
    if (n == 0) {
      throw std::domain_error("Cannot take the zeroth root");
    }
    if (m == 0) {
      return BigNum_(static_cast<man_t>(0));
    }

    bool is_negative = (m < 0);
    if (is_negative && n % 2 == 0) {
      throw std::domain_error("Even root of a negative number is not defined");
    }

    double abs_log = std::log10(std::abs(m)) + e;
    double new_log = abs_log / static_cast<double>(n);

    // Handle small results (new_log < 0 means result < 1)
    if (new_log < 0) {
      man_t result = std::pow(10.0, new_log);
      if (is_negative) {
        result = -result;
      }
      return BigNum_(result, 0, false);
    }

    exp_t new_e = static_cast<exp_t>(std::floor(new_log));
    double fractional = new_log - std::floor(new_log);
    man_t new_m = std::pow(10.0, fractional);

    if (is_negative) {
      new_m = -new_m;
    }

    return BigNum_(new_m, new_e);
  }

  // Returns e^num
  static constexpr BigNum_ exp(exp_t n) {
    return BigNum_(std::exp(1)).pow(static_cast<intmax_t>(n));
  }

  // Returns the square root of num
  constexpr BigNum_ sqrt() const { return root(2); }
};

using BigNum = BigNum_<double, uintmax_t>;

template <typename M, typename E>
inline std::ostream &operator<<(std::ostream &os, const BigNum_<M, E> &bn) {
  os << bn.to_string();
  return os;
}

template <typename M, typename E>
inline std::istream &operator>>(std::istream &is, BigNum_<M, E> &bn) {
  std::string input;
  is >> input;
  bn.parseStr(input);
  return is;
}

// Asserts
static_assert(std::equality_comparable<BigNum>);
static_assert(std::totally_ordered<BigNum>);
static_assert(std::movable<BigNum>);
static_assert(std::copyable<BigNum>);
static_assert(std::default_initializable<BigNum>);
static_assert(std::semiregular<BigNum>);
static_assert(std::regular<BigNum>);

} // namespace BigNumber

// Expose BigNum to the global namespace
using BigNumber::BigNum;
