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

static struct {
    uint max_digits = 10;
} BigNumContext;

static constexpr int Pow10TableSize = std::numeric_limits<double>::max_digits10;
static constexpr std::array<double, Pow10TableSize> Pow10_generate_table() {
    std::array<double, Pow10TableSize> table{};
    double value = 1.0;
    for (int i = 0; i < Pow10TableSize; ++i) {
        table[i] = value;
        value *= 10.0;
    }
    return table;
};

class Pow10 {
private:
    Pow10() = delete;
public:
    static constexpr std::array<double, Pow10TableSize> lookup_table = Pow10_generate_table();

    static constexpr double get(int e) {
        if (e >= Pow10TableSize) {
            throw std::overflow_error("Pow10 double overflow");
        }
        if (e > 0) { return lookup_table[e]; }
        return 1.0 / lookup_table[-e];
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

public:
    static inline BigNum inf() { return BigNum(std::numeric_limits<double>::infinity(), 0); }
    static inline BigNum nan() { return BigNum(std::numeric_limits<double>::quiet_NaN(), 0); }
    static inline BigNum max() { return BigNum(std::numeric_limits<man_t>::max(), std::numeric_limits<exp_t>::max(), false); }
    static inline BigNum min() { return BigNum(0); }

    man_t getM() const { return m; }
    exp_t getE() const { return e; }

    // Constructors
    BigNum(man_t mantissa = 0, exp_t exponent = 0) : m(mantissa), e(exponent) {
        normalize();
    }

    BigNum(const std::string& str) {
        parseStr(str);
    }

    BigNum(const BigNum& other) : m(other.m), e(other.e) {}

    BigNum& operator=(const BigNum& other) {
        m = other.m;
        e = other.e;
        return *this;
    }

    // Normalization (mantissa set to range [1, 10) )
    void normalize() {
        // std::cerr << "Normalizing m=" << m << ",e=" << e << std::endl;
        if (std::isnan(m)) { e = 0; return; }
        if (std::isinf(m)) { e = 0; return; }

        if (m == 0) { e = 0; return; }
        
        int n_log = static_cast<int>(std::floor(std::log10(std::abs(m))));
        // std::cerr << "n_log=" << n_log << std::endl;
        m = m / Pow10::get(n_log);
        e += n_log;

        // Any number less than 1 is considered 0
        m = (std::abs(m) < 1 && e == 0) ? 0 : m;
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
            m2 = m * Pow10::get(delta) + b.m;
            e2 = b.e;
        } else {
            m2 = m + b.m * Pow10::get(delta);
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
            m = m * Pow10::get(delta) + b.m;
            e = b.e;
        } else {
            m = m + b.m * Pow10::get(delta);
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
    BigNum operator+(const intmax_t& other) const { return add(BigNum(other)); }
    BigNum operator-(const BigNum& other) const { return sub(other); }
    BigNum operator-(const std::string& other) const { return sub(BigNum(other)); }
    BigNum operator-(const intmax_t& other) const { return sub(BigNum(other)); }
    BigNum operator*(const BigNum& other) const { return mul(other); }
    BigNum operator*(const std::string& other) const { return mul(BigNum(other)); }
    BigNum operator*(const intmax_t& other) const { return mul(BigNum(other)); }
    BigNum operator/(const BigNum& other) const { return div(other); }
    BigNum operator/(const std::string& other) const { return div(BigNum(other)); }
    BigNum operator/(const intmax_t& other) const { return div(BigNum(other)); }
    BigNum operator-() const { return negate(); }
    BigNum& operator+=(const std::string& b) { return *this += BigNum(b); }
    BigNum& operator+=(const intmax_t& b) { return *this += BigNum(b); }
    BigNum& operator-=(const BigNum& b) { return *this += BigNum(b.m * -1, b.e); }
    BigNum& operator-=(const std::string& b) { return *this -= BigNum(b); }
    BigNum& operator-=(const intmax_t& b) { return *this -= BigNum(b); }
    BigNum& operator*=(const std::string& b) { return *this *= BigNum(b); }
    BigNum& operator*=(const intmax_t& b) { return *this *= BigNum(b); }
    BigNum& operator/=(const std::string& b) { return *this /= BigNum(b); }
    BigNum& operator/=(const intmax_t& b) { return *this /= BigNum(b); }
    BigNum& operator++() { return *this += BigNum(1); }
    BigNum operator++(int) { BigNum temp(*this); *this += BigNum(1); return temp; }
    BigNum& operator--() { return *this -= BigNum(1); }
    BigNum operator--(int) { BigNum temp(*this); *this -= BigNum(1); return temp; }


    // Comparison operations
    bool is_positive() const { return m >= 0; }
    bool is_negative() const { return m < 0; }
    bool is_inf() const { return std::isinf(m); }
    bool is_nan() const { return std::isnan(m); }

    int compare(const BigNum& b) const {
        if (m == b.m && e == b.e) return 0;
        if (is_positive() && b.is_negative()) return 1;
        if (is_negative() && b.is_positive()) return -1;
        if (e > b.e) return 1;
        if (e < b.e) return -1;
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
    std::string to_string() const {
        if (this->is_inf()) { return "inf"; }
        if (this->is_nan()) { return "nan"; }

        // Can this number be fully represented as a string <= max_digits long?
        if (this->e < BigNumContext.max_digits - 1) {
            std::string str = std::to_string(m);
            exp_t newLen = std::min(static_cast<exp_t>(BigNumContext.max_digits), e + 1 ) + (str[0] == '-' ? 1 : 0);
            str.erase(std::remove_if(str.begin(), str.end(), [](char c) { return c == '.'; }), str.end());
            if (str.length() < newLen) {
                str += std::string(newLen - str.length(), '0');
            } else {
                str = str.substr(0, newLen);
            }
            return str;
        }

        // Otherwise, use scientific notation
        std::ostringstream oss;
        oss << m;
        if (e != 0) { oss << "e" << e; }

        return oss.str();
    }

    std::optional<intmax_t> to_number() const {
        int total_digits = e + std::log10(std::abs(m)) + 1;
        if (std::numeric_limits<intmax_t>::max_digits10 < total_digits) { return std::nullopt; }
        return m * Pow10::get(e);
    }

    // Mathematical operations
    std::optional<exp_t> log10() const {
        if (std::numeric_limits<exp_t>::max() - e < std::log10(m)) { return std::nullopt; }
        return e + std::log10(m);
    }

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
        
        // Handle negative powers
        bool is_negative = power < 0;
        intmax_t abs_power = std::abs(power);
        
        // Calculate new logarithm
        double new_log = static_cast<double>(*log) * abs_power;
        
        // If power was negative, invert the result
        if (is_negative) { new_log = -new_log; }

        // Check if result would be too small
        if (std::abs(new_log) < std::numeric_limits<double>::min_exponent10) {
            return BigNum(0, 0);
        }

        // Split into mantissa and exponent
        man_t mantissa = static_cast<man_t>(Pow10::get(static_cast<uint>(std::fmod(new_log, 1.0))));
        exp_t exponent = static_cast<exp_t>(std::floor(new_log));

        return BigNum(mantissa, exponent);
    }

    static BigNum exp(exp_t n) {
        return BigNum(std::exp(1)).pow(n);
    }

    BigNum sqrt() const { return pow(0.5); }
    
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
