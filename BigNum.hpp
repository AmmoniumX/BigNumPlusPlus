/*
BigNum++: C++ port of https://github.com/Patashu/break_infinity.js
Slightly modified to increase maximum range by moving negative numbers outside
of the exponent's range (Emin is 0, therefore Emax is higher) Tradeoff: Cannot
store numbers smaller than regular doubles, but those aren't usually needed in
the types of games that would use this library
*/

#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <compare>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

/* Define a macro for deducing CONSTEXPR_NEXTAFTER_FALLBACK
 * We do this because std::nextafter is not properly constepxr in most compilers
 * yet, so if we conclude that it's not constexpr, we use our own fallback
 * implementation
 */
#if defined(__clang__)
// Clang does NOT support constexpr std::nextafter as of now
#define CONSTEXPR_NEXTAFTER_FALLBACK
#elif defined(__GNUC__)
// It's safe to use constexpr nextafter if we compile with -fno-trapping-math
#ifndef NO_TRAPPING_MATH
#define CONSTEXPR_NEXTAFTER_FALLBACK
#endif
#else
// For other compilers, be conservative
#define CONSTEXPR_NEXTAFTER_FALLBACK
#endif

// Define a macro for CPP26 and later for constexpr statements
#define CPP26 (__cplusplus >= 202600L)

// Constant precision for serializing
namespace BigNumber {
using namespace std::literals::string_literals;

static constexpr uint SERIAL_PRECISION = 9;
static constexpr char DECIMAL_SEPARATOR = '.';
static constexpr char THOUSANDS_SEPARATOR = ',';

// Formatting context
struct BigNumContext {
    uint max_digits = 10; // Up to how many "real" digits to display before
                          // using scientific notation
    uint print_precision =
        3; // How many fractional digits to display on scientific notation
};
// Global "default" context when none is passed to functions
inline BigNumContext DefaultBigNumContext;

// Precompute powers-of-10 table for performance
static constexpr int Pow10TableOffset =
    std::numeric_limits<double>::max_exponent10;
static constexpr int Pow10TableSize = 2 * Pow10TableOffset + 1;
static constexpr std::array<double, Pow10TableSize> Pow10_generate_table() {
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
    static constexpr std::array<double, Pow10TableSize> Pow10Table =
        Pow10_generate_table();

    // e must be in the range [-offset, offset]
    static constexpr std::optional<double> get(int e) {
        if (e < -Pow10TableOffset || e > Pow10TableOffset) {
            return std::nullopt;
        }
        return Pow10Table[e + Pow10TableOffset];
    }
};

class BigNum {
    using man_t = double;    // mantissa type
    using exp_t = uintmax_t; // exponent type

    friend std::ostream &operator<<(std::ostream &os, const BigNum &bn);
    friend std::istream &operator>>(std::istream &is, BigNum &bn);

  private:
    man_t m = 0; // mantissa
    exp_t e = 0; // exponent (base 10)
    static_assert(std::is_floating_point_v<man_t>,
                  "mantissa must be a floating point type");
    static_assert(std::is_arithmetic_v<exp_t>,
                  "exponent must be an arithmetic type");

    static constexpr exp_t MAX_DIV_DIFF = 308;
    // helper functions to convert strings to mantissa/exponent
    static inline man_t strtom(const std::string_view &sv) {
        man_t m;
        auto result = std::from_chars(sv.data(), sv.data() + sv.size(), m);
        if (result.ec != std::errc()) {
            throw std::invalid_argument(
                "Failed to convert string to mantissa: " + std::string(sv));
        }
        return m;
    }
    static inline exp_t strtoe(const std::string_view &sv) {
        exp_t e;
        auto result = std::from_chars(sv.data(), sv.data() + sv.size(), e);
        if (result.ec != std::errc()) {
            throw std::invalid_argument(
                "Failed to convert string to exponent: " + std::string(sv));
        }
        return e;
    }

    // convert the number to a full-precision string
    static std::string to_string_full(const man_t &value) {
        std::ostringstream out;
        out << std::setprecision(std::numeric_limits<man_t>::digits10 + 1)
            << value;
        return out.str();
    }

    // convert double to string, rounding down and up to specific precision
    static std::string to_string_floor(const double &value,
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

// Fallback implemnetation in case of non-constexpr std::nextafter
#ifdef CONSTEXPR_NEXTAFTER_FALLBACK
    static constexpr double _prev_double(double x) {
        using uint = std::uint64_t;
        static_assert(sizeof(double) == sizeof(uint),
                      "Size of float and uint must be the same");

        if (x <= -std::numeric_limits<double>::infinity())
            return -std::numeric_limits<double>::infinity();
        if (x != x)
            return x; // NaN
        if (x == 0.0l)
            return -std::numeric_limits<double>::denorm_min();

        uint bits = std::bit_cast<uint>(x);
        bits = (x > 0.0f) ? (bits - 1) : (bits + 1);
        return std::bit_cast<double>(bits);
    }

    static constexpr double _next_double(double x) {
        using uint = std::uint64_t;
        static_assert(sizeof(double) == sizeof(uint),
                      "Size of float and uint must be the same");

        if (x >= std::numeric_limits<double>::infinity())
            return std::numeric_limits<double>::infinity();
        if (x != x)
            return x; // NaN
        if (x == 0.0l)
            return std::numeric_limits<double>::denorm_min();

        uint bits = std::bit_cast<uint>(x);
        bits = (x > 0.0f) ? (bits + 1) : (bits - 1);
        return std::bit_cast<double>(bits);
    }
#endif

// Fallback to constexpr std::log10 if not on C++26
#if !CPP26
    static constexpr int _log10(double x) {
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

    constexpr BigNum(const man_t mantissa, const exp_t exponent, bool normalize)
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
            throw std::invalid_argument(
                std::string("Failed to parse number: ") + ex.what());
        }
    }

    constexpr void set(const BigNum &other) {
        m = other.m;
        e = other.e;
    }

  public:
    static constexpr const BigNum &inf() {
        static constexpr BigNum inf_val(std::numeric_limits<man_t>::infinity(),
                                        0, false);

        return inf_val;
    }
    static constexpr const BigNum &nan() {
        static constexpr BigNum nan_val(std::numeric_limits<man_t>::quiet_NaN(),
                                        0, false);

        return nan_val;
    }
    static constexpr const BigNum &max() {
#ifdef CONSTEXPR_NEXTAFTER_FALLBACK
        static constexpr const BigNum max_val(
            _prev_double(10.0), std::numeric_limits<exp_t>::max(), false);
#else
        static constexpr const BigNum max_val(std::nextafter(10.0, 0.0),
                                              std::numeric_limits<exp_t>::max(),
                                              false);
#endif

        return max_val;
    }
    static constexpr const BigNum &min() {
#ifdef CONSTEXPR_NEXTAFTER_FALLBACK
        static constexpr const BigNum min_val(
            _next_double(-10.0), std::numeric_limits<exp_t>::max(), false);
#else
        static constexpr const BigNum min_val(std::nextafter(-10.0, 0.0),
                                              std::numeric_limits<exp_t>::max(),
                                              false);
#endif

        return min_val;
    }

    constexpr man_t getM() const { return m; }
    constexpr exp_t getE() const { return e; }

    constexpr BigNum(const man_t mantissa, const exp_t exponent = 0) {
        m = mantissa;
        e = exponent;
        normalize();
    }

    constexpr BigNum(const std::string_view &str) { parseStr(str); }

    // Default methods to satisfy concepts
    constexpr BigNum() : m(0), e(0) { normalize(); } // Default constructor
    constexpr BigNum(const BigNum &) = default;      // Copy constructor
    constexpr BigNum &operator=(const BigNum &) = default; // Copy assignment
    constexpr BigNum(BigNum &&) = default;                 // Move constructor
    constexpr BigNum &operator=(BigNum &&) = default;      // Move assignment

    ~BigNum() = default; // Destructor

    // Normalization: mantissa set in range (-10, 10)
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
        n_log =
            std::max(static_cast<int>(std::floor(std::log10(std::abs(m)))), 0);
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
    constexpr BigNum add(const BigNum &b) const {

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
            return BigNum(m + b.m, 0);
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

        return BigNum(m2, e2);
    }

    constexpr BigNum sub(const BigNum &b) const {
        return add(BigNum(b.m * -1, b.e));
    }

    constexpr BigNum mul(const BigNum &b) const {
        return BigNum(m * b.m, e + b.e);
    }

    constexpr BigNum div(const BigNum &b) const {
        // Division by zero, return NaN
        if (b.m == 0) {
            return nan();
        }

        // Divisor is significantly larger than dividend, result is 0
        if ((b.e > e) && (b.e - e >= MAX_DIV_DIFF)) {
            return BigNum(static_cast<man_t>(0));
        }

        // Perform division
        return BigNum(m / b.m, e - b.e);
    }

    constexpr BigNum abs() const { return BigNum(std::abs(m), e); }

    constexpr BigNum negate() const {
        return mul(BigNum(static_cast<man_t>(-1)));
    }

    constexpr BigNum &operator+=(const BigNum &b) {
        bool this_is_bigger = e > b.e;
        exp_t delta = this_is_bigger ? e - b.e : b.e - e;
        if (delta > 14) {
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

    constexpr BigNum &operator*=(const BigNum &b) {
        m *= b.m;
        e += b.e;
        normalize();
        return *this;
    }

    constexpr BigNum &operator/=(const BigNum &b) {
        if (b.m == 0) {
            // Division by zero, return NaN
            m = nan().m;
            e = nan().e;
        } else if ((b.e > e) && (b.e - e >= MAX_DIV_DIFF)) {
            // Divisor is significantly larger than dividend, result is 0
            m = 0;
            e = 0;
        } else {
            // Perform division
            m /= b.m;
            e -= b.e;
        }
        normalize();
        return *this;
    }

    // Operator overloads
    constexpr BigNum operator+(const BigNum &other) const { return add(other); }
    constexpr BigNum operator+(const std::string_view &other) const {
        return add(BigNum(other));
    }
    constexpr BigNum operator+(const man_t other) const {
        return add(BigNum(other));
    }
    constexpr BigNum operator-(const BigNum &other) const { return sub(other); }
    constexpr BigNum operator-(const std::string_view &other) const {
        return sub(BigNum(other));
    }
    constexpr BigNum operator-(const man_t other) const {
        return sub(BigNum(other));
    }
    constexpr BigNum operator*(const BigNum &other) const { return mul(other); }
    constexpr BigNum operator*(const std::string_view &other) const {
        return mul(BigNum(other));
    }
    constexpr BigNum operator*(const man_t other) const {
        return mul(BigNum(other));
    }
    constexpr BigNum operator/(const BigNum &other) const { return div(other); }
    constexpr BigNum operator/(const std::string_view &other) const {
        return div(BigNum(other));
    }
    constexpr BigNum operator/(const man_t other) const {
        return div(BigNum(other));
    }
    constexpr BigNum operator-() const { return negate(); }
    constexpr BigNum &operator+=(const std::string_view &b) {
        return *this += BigNum(b);
    }
    constexpr BigNum &operator+=(const man_t b) { return *this += BigNum(b); }
    constexpr BigNum &operator-=(const BigNum &b) {
        return *this += BigNum(b.m * -1, b.e);
    }
    constexpr BigNum &operator-=(const std::string_view &b) {
        return *this -= BigNum(b);
    }
    constexpr BigNum &operator-=(const man_t b) { return *this -= BigNum(b); }
    constexpr BigNum &operator*=(const std::string_view &b) {
        return *this *= BigNum(b);
    }
    constexpr BigNum &operator*=(const man_t b) { return *this *= BigNum(b); }
    constexpr BigNum &operator/=(const std::string_view &b) {
        return *this /= BigNum(b);
    }
    constexpr BigNum &operator/=(const man_t b) { return *this /= BigNum(b); }
    constexpr BigNum &operator++() {
        return *this += BigNum(static_cast<man_t>(1));
    }
    constexpr BigNum operator++(int) {
        BigNum temp(*this);
        *this += BigNum(static_cast<man_t>(1));
        return temp;
    }
    constexpr BigNum &operator--() {
        return *this -= BigNum(static_cast<man_t>(1));
    }
    constexpr BigNum operator--(int) {
        BigNum temp(*this);
        *this -= BigNum(static_cast<man_t>(1));
        return temp;
    }

    // Comparison operations
    constexpr bool is_positive() const { return m >= 0; }
    constexpr bool is_negative() const { return m < 0; }
    constexpr bool is_inf() const { return std::isinf(m); }
    constexpr bool is_nan() const { return std::isnan(m); }
    static constexpr BigNum &max(BigNum &a, BigNum &b) { return a > b ? a : b; }
    static constexpr BigNum &min(BigNum &a, BigNum &b) { return a < b ? a : b; }

    constexpr std::partial_ordering operator<=>(const BigNum &b) const {
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
    // Equality operator (only use this under the assumption that the numbers
    // are already normalized)
    constexpr bool operator==(const BigNum &other) const = default;

    constexpr std::partial_ordering
    operator<=>(const std::string_view &other) const {
        return *this <=> BigNum(other);
    }
    constexpr std::partial_ordering operator<=>(const man_t other) const {
        return *this <=> BigNum(other);
    }

    // Conversion methods
    constexpr std::string to_string(
        const uint &precision = DefaultBigNumContext.print_precision) const {
        if (this->is_inf()) {
            return "inf";
        }
        if (this->is_nan()) {
            return "nan";
        }

        // Handle small numbers directly
        if (e == 0) {

            double scale = *Pow10::get(precision);
            man_t rounded = std::floor(m * scale) / scale;
            std::string str = std::to_string(rounded);

            // truncate string if it's bigger than precision (excluding leading
            // 0 and decimal point)
            if (str.length() - 2 > precision) {
                return str.substr(0, precision + 2);
            }

            return str;
        }

        // Can this number be fully displayed as a string <= max_digits long?
        // Assumes m and e are already normalized
        uint max_digits =
            std::max(precision + 1, DefaultBigNumContext.max_digits);
        if (this->e < max_digits - 1) {
            std::string str = to_string_full(m);

            // calculate new length based on
            exp_t newLen = std::min(static_cast<exp_t>(max_digits), e + 1) +
                           (str[0] == '-' ? 1 : 0);

            // Remove the decimal separator if it exists (and isn't small number
            // <1)
            str.erase(
                std::remove_if(str.begin(), str.end(),
                               [](char c) { return c == DECIMAL_SEPARATOR; }),
                str.end());

            // If the string is shorter than the desired length, pad with zeros
            if (str.length() < newLen) {
                str += std::string(newLen - str.length(), '0');
            } else {
                // If the string is longer than the desired length, truncate it,
                // and round the last digit if necessary
                bool round_up = false;

                int first_unused_digit_pos =
                    (str.length() > newLen) ? newLen : -1;
                int first_unused_digit =
                    (first_unused_digit_pos > 0)
                        ? int(str[first_unused_digit_pos] - '0')
                        : -1;

                if (first_unused_digit >= 5) {
                    // Round up the last digit
                    round_up = true;
                }
                str = str.substr(0, newLen);
                if (round_up) {
                    assert(!str.empty() &&
                           "String should not be empty after truncation");
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
    constexpr std::string to_pretty_string(
        const uint &precision = DefaultBigNumContext.print_precision) const {
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
    constexpr std::string serialize() const {
        return to_string(SERIAL_PRECISION);
    }

    static constexpr BigNum deserialize(const std::string_view &str) {
        return BigNum(str);
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
    constexpr BigNum pow(double power) const {
        // Special cases
        if (power == 0.0) {
            return BigNum(static_cast<man_t>(1));
        }
        if (m == 0) {
            if (power < 0) {
                throw std::domain_error("Cannot raise 0 to a negative power");
            }
            return BigNum(static_cast<man_t>(0));
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
                return BigNum(-m, e).pow(power); // Even power
            }
            return BigNum(-m, e).pow(power).negate(); // Odd power
        }

        // Calculate using logarithms
        auto log = log10();
        if (!log) {
            // std::cerr << "Logarithm out of bounds" << std::endl;
            return BigNum(static_cast<man_t>(0));
        }

        // Calculate new logarithm
        double new_log = static_cast<double>(*log) * power;

        // Check if result would be too small
        if (std::abs(new_log) < std::numeric_limits<double>::min_exponent10) {
            // std::cerr << "Result too small" << std::endl;
            return BigNum(static_cast<man_t>(0));
        }

        // Split into mantissa and exponent
        man_t m2 = static_cast<man_t>(std::pow(10, std::fmod(new_log, 1.0)));
        exp_t e2 = static_cast<exp_t>(std::floor(new_log));

        return BigNum(m2, e2);
    }

    // Integer power overload - just calls the double version
    constexpr BigNum pow(intmax_t power) const {
        return pow(static_cast<double>(power));
    }

    // Returns num^(1/n), aka the nth root
    constexpr BigNum root(intmax_t n) const {
        if (n == 0) {
            throw std::domain_error("Cannot take the zeroth root");
        } // Handle zero early
        if (m == 0) {
            return BigNum(static_cast<man_t>(0));
        }
        // Handle negative numbers: Only allow odd roots for negative bases
        bool is_negative = (m < 0);
        if (is_negative) {
            if (n % 2 == 0) {
                throw std::domain_error(
                    "Even root of a negative number is not defined");
            }
        }

        // Compute log10(|num|) = log10(|m|) + e
        double abs_log = std::log10(std::abs(m)) + e;
        // The new logarithm for the x-th root
        double new_log = abs_log / static_cast<double>(n);

        // Split new_log into its integer part (new exponent) and fractional
        // part
        exp_t new_e = static_cast<exp_t>(std::floor(new_log));
        double fractional = new_log - std::floor(new_log);
        // Compute the new mantissa from the fractional part
        man_t new_m = std::pow(10.0, fractional);

        // For negative bases with an odd root, the result should be negative
        if (is_negative) {
            new_m = -new_m;
        }

        return BigNum(new_m, new_e);
    }

    // Returns e^num
    static constexpr BigNum exp(exp_t n) {
        return BigNum(std::exp(1)).pow(static_cast<intmax_t>(n));
    }

    // Returns the square root of num
    constexpr BigNum sqrt() const { return root(2); }
};

inline std::ostream &operator<<(std::ostream &os, const BigNum &bn) {
    os << bn.to_string();
    return os;
}

inline std::istream &operator>>(std::istream &is, BigNum &bn) {
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
