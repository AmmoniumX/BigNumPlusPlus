/*
BigNum++: C++ port of https://github.com/veprogames/lua-big-number
Slightly modified to increase maximum range by moving negative numbers outside of the exponent's range (Emin is 0, therefore Emax is higher)
Tradeoff: Cannot store numbers between (-1, 0) or (0, 1), but those aren't usually needed in the types of games that would use this library
*/

#pragma once

#include <cmath>
#include <algorithm>
#include <string>
#include <optional>
#include <sstream>
#include <array>
#include <limits>
#include <cstdint>
#include <inttypes.h>
#include <cassert>
#include <iostream>
#include <iomanip>

struct BigNumContext {
    uint max_digits = 10; // Up to how many "real" digits to display before using scientific notation
    uint print_precision = 2; // How many fractional digits to display on scientific notation
};
inline BigNumContext DefaultBigNumContext;

static constexpr int Pow10TableOffset = std::numeric_limits<double>::max_exponent10;
static constexpr int Pow10TableSize = 2 * Pow10TableOffset + 1;
static constexpr std::array<double, Pow10TableSize> Pow10_generate_table() {
    std::array<double, Pow10TableSize> table{};
    table[Pow10TableOffset] = 1.0;
    double pos = 1.0;
    for (int i = 1; i <= Pow10TableOffset; ++i) {
        pos *= 10.0;
        table[Pow10TableOffset + i] = pos;      // positive exponents: 10^i
        table[Pow10TableOffset - i] = 1.0 / pos;  // negative exponents: 10^(-i)
    }
    return table;
};

class Pow10 {
private:
    Pow10() = delete;
public:
    static constexpr std::array<double, Pow10TableSize> Pow10Table = Pow10_generate_table();

    // e must be in the range [-offset, offset]
    static constexpr std::optional<double> get(int e) {
        if (e < -Pow10TableOffset || e > Pow10TableOffset) {
            return std::nullopt;
        }
        return Pow10Table[e + Pow10TableOffset];
    }
};

class BigNum {
    using man_t = double;
    using exp_t = uintmax_t;

    friend std::ostream& operator<<(std::ostream& os, const BigNum& bn);
    friend std::istream& operator>>(std::istream& is, BigNum& bn);

private:
    man_t m;
    exp_t e;
    static inline man_t strtom(const std::string str) { return std::stod(str); }
    static inline exp_t strtoe(const std::string str) { return strtoumax(str.c_str(), nullptr, 10); }
    static std::string to_string_full(double value) {
        std::ostringstream out;
        out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
        return out.str();
    }
    static std::string to_string_floor(double value, int precision) {
        // Assumes value is normalized to 1 digit before the decimal point (|value| < 10)
        assert(value > -10 && value < 10 && "Value must be normalized");
        double scale = std::pow(10.0, precision);
        double truncated_value = std::floor(value * scale) / scale;
    
        std::ostringstream out;
        out << std::fixed << std::setprecision(precision) << truncated_value;
        std::string out_str = out.str();

        // If necessary, round down to always return 1 digit before the decimal point
        // This is to avoid rounding errors when the number is close to 10
        // Should always be correct given our assumption that |value| < 10
        if (out_str.substr(0, 3) == "10.") {
            out_str = "9." + std::string(precision, '9');
        } else if (out_str.substr(0, 4) == "-10.") {
            out_str = "-9." + std::string(precision, '9');
        }
        return out_str;
    }
    
    BigNum(man_t mantissa, exp_t exponent, bool normalize) : m(mantissa), e(exponent) {
        if (normalize) this->normalize();
    }

    void parseStr(const std::string& str) {
        try {
            size_t pos = str.find('e');
            if (pos != std::string::npos) {
                m = strtom(str.substr(0, pos));
                e = strtoe(str.substr(pos + 1));
            } else {
                m = strtom(str);
                e = 0;
            }
            normalize();
        } catch (const std::invalid_argument& ex) {
            throw std::invalid_argument(std::string("Failed to parse number: ") + ex.what());
        }
    }

    void set(const BigNum& other) {
        m = other.m;
        e = other.e;
    }

public:
    static inline const BigNum& inf() { 
        static const BigNum inf_val(std::numeric_limits<double>::infinity(), 0, false); 
        return inf_val;
    }
    static inline const BigNum& nan() { 
        static const BigNum nan_val(std::numeric_limits<double>::quiet_NaN(), 0, false); 
        return nan_val;
    }
    static inline const BigNum& max() { 
        static const BigNum max_val(std::nextafter(10.0, 0), std::numeric_limits<exp_t>::max(), false); 
        return max_val;
    }
    static inline const BigNum& min() { 
        static const BigNum min_val(std::nextafter(-10.0, 0), std::numeric_limits<exp_t>::max(), false); 
        return min_val;
    }

    man_t getM() const { return m; }
    exp_t getE() const { return e; }

    BigNum(man_t mantissa = 0, exp_t exponent = 0) : m(mantissa), e(exponent) {
        normalize();
    }

    BigNum(const std::string& str) {
        parseStr(str);
    }

    BigNum(const BigNum& other) = default;

    BigNum& operator=(const BigNum& other) = default;

    ~BigNum() = default;

    // Normalization: mantissa set in range (-10, 1] and [1, 10)
    void normalize() {
        // std::cerr << "Normalizing m=" << m << ",e=" << e << std::endl;
        if (*this == max() || *this == min()) { return; }
        if (std::isnan(m)) { e = 0; return; }
        if (std::isinf(m)) { e = 0; return; }
        if (m == 0) { e = 0; return; }
        if (std::abs(m) < 1 && e == 0) { m = 0; return; } // Any number less than 1 is considered 0
        
        // Start normalization
        int n_log = static_cast<int>(std::floor(std::log10(std::abs(m))));
        // std::cerr << "n_log=" << n_log << std::endl;
        m = m / (*Pow10::get(n_log));
        e += n_log;

        // Any number less than 1 is considered 0
        m = (std::abs(m) < 1 && e == 0) ? 0 : m;

        // Clamp between max and min
        if (*this > max()) { set(max()); }
        if (*this < min()) { set(min()); }
        // std::cerr << "After normalization: m=" << m << ",e=" << e << std::endl;
    }

    // Arithmetic operations
    BigNum add(const BigNum& b) const {
        // std::cerr << "Adding m=" << m << ",e=" << e << " and m=" << b.m << ",e=" << b.e << std::endl;
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

        // std::cerr << "Result: " << *this << std::endl;
        return BigNum(m2, e2);
    }

    BigNum sub(const BigNum& b) const {
        return add(BigNum(b.m * -1, b.e));
    }

    BigNum mul(const BigNum& b) const {
        return BigNum(m * b.m, e + b.e);
    }

    BigNum div(const BigNum& b) const {
        // division by zero, return NaN
        if (b.m == 0) { return nan(); }
        // Divisor is larger than dividend, result is 0
        if (b.e > e) { return BigNum(0); }
        // Any number less than 1 is considered 0
        if (b.e == e && std::abs(m / b.m) < 1) { return BigNum(0); }
        // Perform division
        return BigNum(m / b.m, e - b.e);
    }

    BigNum abs() const {
        return BigNum(std::abs(m), e);
    }

    BigNum negate() const {
        return mul(BigNum(-1));
    }

    BigNum& operator+=(const BigNum& b) {
        // std::cerr << "Adding m=" << m << ",e=" << e << " and m=" << b.m << ",e=" << b.e << std::endl;
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
        // std::cerr << "Result: " << *this << std::endl;
        return *this;
    }

    BigNum& operator*=(const BigNum& b) {
        m *= b.m;
        e += b.e;
        normalize();
        return *this;
    }

    BigNum& operator/=(const BigNum& b) {
        if (b.m == 0) { 
            // division by zero, return NaN
            m = nan().m;
            e = nan().e;
        } else if (b.e > e) { 
            // Divisor is larger than dividend, result is 0
            m = 0;
            e = 0;
        } else if (b.e == e && std::abs(m / b.m) < 1) { 
            // Any number less than 1 is considered 0
            m = 0;
            e = 0;
        } else {
            // Perform division
            m /= b.m;
            e -= b.e;
        }
        return *this;
    }

    // Operator overloads
    BigNum operator+(const BigNum& other) const { return add(other); }
    BigNum operator+(const std::string& other) const { return add(BigNum(other)); }
    BigNum operator+(const intmax_t other) const { return add(BigNum(other)); }
    BigNum operator-(const BigNum& other) const { return sub(other); }
    BigNum operator-(const std::string& other) const { return sub(BigNum(other)); }
    BigNum operator-(const intmax_t other) const { return sub(BigNum(other)); }
    BigNum operator*(const BigNum& other) const { return mul(other); }
    BigNum operator*(const std::string& other) const { return mul(BigNum(other)); }
    BigNum operator*(const intmax_t other) const { return mul(BigNum(other)); }
    BigNum operator/(const BigNum& other) const { return div(other); }
    BigNum operator/(const std::string& other) const { return div(BigNum(other)); }
    BigNum operator/(const intmax_t other) const { return div(BigNum(other)); }
    BigNum operator-() const { return negate(); }
    BigNum& operator+=(const std::string& b) { return *this += BigNum(b); }
    BigNum& operator+=(const intmax_t b) { return *this += BigNum(b); }
    BigNum& operator-=(const BigNum& b) { return *this += BigNum(b.m * -1, b.e); }
    BigNum& operator-=(const std::string& b) { return *this -= BigNum(b); }
    BigNum& operator-=(const intmax_t b) { return *this -= BigNum(b); }
    BigNum& operator*=(const std::string& b) { return *this *= BigNum(b); }
    BigNum& operator*=(const intmax_t b) { return *this *= BigNum(b); }
    BigNum& operator/=(const std::string& b) { return *this /= BigNum(b); }
    BigNum& operator/=(const intmax_t b) { return *this /= BigNum(b); }
    BigNum& operator++() { return *this += BigNum(1); }
    BigNum operator++(int) { BigNum temp(*this); *this += BigNum(1); return temp; }
    BigNum& operator--() { return *this -= BigNum(1); }
    BigNum operator--(int) { BigNum temp(*this); *this -= BigNum(1); return temp; }


    // Comparison operations
    bool is_positive() const { return m >= 0; }
    bool is_negative() const { return m < 0; }
    bool is_inf() const { return std::isinf(m); }
    bool is_nan() const { return std::isnan(m); }
    static inline BigNum& max(BigNum& a, BigNum& b) { return a > b ? a : b; }
    static inline BigNum& min(BigNum& a, BigNum& b) { return a < b ? a : b; }

    int compare(const BigNum& b) const {
        if (m == b.m && e == b.e) return 0;
        if (is_positive() && b.is_negative()) return 1;
        if (is_negative() && b.is_positive()) return -1;
        if (is_positive() && b.is_positive() && e > b.e) return 1;
        if (is_positive() && b.is_positive() && e < b.e) return -1;
        if (is_negative() && b.is_negative() && e > b.e) return -1;
        if (is_negative() && b.is_negative() && e < b.e) return 1;
        if (is_positive() && m > b.m) return 1;
        if (is_positive() && m < b.m) return -1;
        if (is_negative() && m > b.m) return -1;
        if (is_negative() && m < b.m) return 1;
        return 0;
    }

    // Comparison operator overloads
    bool operator<(const BigNum& other) const { return compare(other) < 0; }
    bool operator<(const std::string& other) const { return compare(BigNum(other)) < 0; }
    bool operator<(const intmax_t other) const { return compare(BigNum(other)) < 0; }
    bool operator<=(const BigNum& other) const { return compare(other) <= 0; }
    bool operator<=(const std::string& other) const { return compare(BigNum(other)) <= 0; }
    bool operator<=(const intmax_t other) const { return compare(BigNum(other)) <= 0; }
    bool operator>(const BigNum& other) const { return compare(other) > 0; }
    bool operator>(const std::string& other) const { return compare(BigNum(other)) > 0; }
    bool operator>(const intmax_t other) const { return compare(BigNum(other)) > 0; }
    bool operator>=(const BigNum& other) const { return compare(other) >= 0; }
    bool operator>=(const std::string& other) const { return compare(BigNum(other)) >= 0; }
    bool operator>=(const intmax_t other) const { return compare(BigNum(other)) >= 0; }
    bool operator==(const BigNum& other) const { return compare(other) == 0; }
    bool operator==(const std::string& other) const { return compare(BigNum(other)) == 0; }
    bool operator==(const intmax_t other) const { return compare(BigNum(other)) == 0; }
    bool operator!=(const BigNum& other) const { return compare(other) != 0; }
    bool operator!=(const std::string& other) const { return compare(BigNum(other)) != 0; }
    bool operator!=(const intmax_t other) const { return compare(BigNum(other)) != 0; }

    // Conversion methods
    std::string to_string(uint precision=DefaultBigNumContext.print_precision) const {
        if (this->is_inf()) { return "inf"; }
        if (this->is_nan()) { return "nan"; }

        // Can this number be fully displayed as a string <= max_digits long?
        // Assumes m and e are already normalized
        uint max_digits = std::max(precision+1, DefaultBigNumContext.max_digits);
        if (this->e < max_digits - 1) {
            std::string str = std::to_string(m);
            exp_t newLen = std::min(static_cast<exp_t>(max_digits), e + 1 ) + (str[0] == '-' ? 1 : 0);
            str.erase(std::remove_if(str.begin(), str.end(), [](char c) { return c == '.'; }), str.end());
            if (str.length() < newLen) {
                str += std::string(newLen - str.length(), '0');
            } else {
                str = str.substr(0, newLen);
            }
            return str;
        }

        // Otherwise, use scientific notation
        std::ostringstream out;
        // out << std::fixed << std::setprecision(precision) << m;
        
        // Handle max() and min() specifically due to rounding issues
        // std::string m_str;
        // if (*this == max()) { m_str = std::string("9.") + std::string(precision, '9'); }
        // else if (*this == min()) { m_str = std::string("-9.") + std::string(precision, '9'); }
        // else { m_str = to_string_floor(m, precision); }
        // std::cerr << "m_str: " << m_str << std::endl;
        
        // Trim the result to the desired precision
        // std::string m_str = to_string_floor(m, 2);
        // size_t dot_pos = m_str.find('.');
        // if (dot_pos != std::string::npos && dot_pos + precision + 1 < m_str.size()) {
            //     m_str = m_str.substr(0, dot_pos + precision + 1);
            // }
        std::string m_str = to_string_floor(m, precision);
        out << m_str;

        if (e != 0) { out << "e" << e; }

        return out.str();
    }

    // Serialize with constant precision
    std::string serialize() const {
        return to_string(9);
    }

    // Returns number as intmax_t, or nullopt if the number is too large
    std::optional<intmax_t> to_number() const {
        int total_digits = e + std::log10(std::abs(m)) + 1;
        if (total_digits > std::numeric_limits<intmax_t>::digits10) { 
            // std::cerr << "Number is too large to convert to intmax_t: " << this->to_string() << std::endl;
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
    std::optional<double> log10() const {
        if (std::numeric_limits<double>::max() - e < std::log10(m)) { return std::nullopt; }
        return e + std::log10(m);
    }

    // Returns num^power
    BigNum pow(intmax_t power) const {
        // Special cases
        if (power == 0) { return BigNum(1); }
        if (m == 0) {
            if (power < 0) { throw std::domain_error("Cannot raise 0 to a negative power"); }
            return BigNum(0);
        }

        // When the mantissa is negative
        if (m < 0) {
            if (power % 2 == 0) { return BigNum(-m, e).pow(power); } // Even power of negative number is positive
            return BigNum(-m, e).pow(power).negate(); // Odd power of negative number is negative
        }

        // Calculate using logarithms
        auto log = log10();
        if (!log) { return BigNum(0); }
        // std::cerr << "log: " << std::to_string(*log) << std::endl;
        
        // Handle negative powers
        bool is_negative = power < 0;
        intmax_t abs_power = std::abs(power);
        
        // Calculate new logarithm
        double new_log = static_cast<double>(*log) * abs_power;
        // std::cerr << "new_log: " << std::to_string(new_log) << std::endl;
        
        // If power was negative, invert the result
        if (is_negative) { new_log = -new_log; }

        // Check if result would be too small
        if (std::abs(new_log) < std::numeric_limits<double>::min_exponent10) {
            return BigNum(0);
        }

        // Split into mantissa and exponent
        man_t m2 = static_cast<man_t>(std::pow(10, std::fmod(new_log, 1.0)));
        exp_t e2 = static_cast<exp_t>(std::floor(new_log));

        // std::cerr << "m=" << std::to_string(m2) << ",e=" << std::to_string(e2) << std::endl;
        return BigNum(m2, e2);
    }

    // Returns num^(1/n), aka the nth root
    BigNum root(intmax_t n) const { 
        if (n == 0) { throw std::domain_error("Cannot take the zeroth root"); } // Handle zero early 
        if (m == 0) { return BigNum(0); }
        // Handle negative numbers: Only allow odd roots for negative bases
        bool is_negative = (m < 0);
        if (is_negative) {
            if (n % 2 == 0) {
                throw std::domain_error("Even root of a negative number is not defined");
            }
        }

        // Compute log10(|num|) = log10(|m|) + e
        double abs_log = std::log10(std::abs(m)) + e;
        // The new logarithm for the x-th root
        double new_log = abs_log / static_cast<double>(n);

        // Split new_log into its integer part (new exponent) and fractional part
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
    static BigNum exp(exp_t n) {
        return BigNum(std::exp(1)).pow(n);
    }

    // Returns the square root of num
    BigNum sqrt() const { return root(2); }
    
};

inline std::ostream& operator<<(std::ostream& os, const BigNum& bn) {
    os << bn.to_string();
    return os;
}

inline std::istream& operator>>(std::istream& is, BigNum& bn) {
    std::string input;
    is >> input;
    bn.parseStr(input);
    return is;
}
