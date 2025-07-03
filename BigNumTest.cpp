#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <string>
#include <string_view>
#include <sstream>
#include <optional>
#include <print>

#include "BigNum.hpp"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

// Helper to convert anything to a string for printing
template <typename T>
std::string to_string_test(const T& value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

// Overload for BigNum to use its own to_string
std::string to_string_test(const BigNum& value) {
    return value.to_string(9); // Use high precision for tests
}

// Overload for booleans
std::string to_string_test(const bool value) {
    return value ? "true" : "false";
}

template <typename T>
constexpr void assertEquals(const T& expected, const T& actual, const std::string_view& testName) {
    if (expected != actual) {
        std::println(stderr, "Test failed: {}", testName);
        std::println(stderr, "  Expected: {}", to_string_test(expected));
        std::println(stderr, "  Got:      {}", to_string_test(actual));
    }
}

constexpr void assertTrue(bool condition, const std::string_view& testName) {
    if (!condition) {
        std::println(stderr, "Test failed: {}", testName);
    }
}

void assertIsClose(double expected, double actual, const std::string_view& testName, double tolerance = 1e-5) {
    if (std::abs(expected - actual) > tolerance) {
        std::println(stderr, "Test failed: {}", testName);
        std::println(stderr, "  Expected: {}", expected);
        std::println(stderr, "  Got:      {}", actual);
    }
}


void runInstantiationTests() {
    BigNum v1("1.23456789e123456789");
    assertEquals("1.23e123456789"s, v1.to_string(2), "Instantiation from string (positive)"sv);
    
    BigNum v2("-1.23456789e123456789");
    assertEquals("-1.24e123456789"s, v2.to_string(2), "Instantiation from string (negative)"sv);

    BigNum v3("100");
    assertEquals("100"s, v3.to_string(), "Instantiation from string (small positive)"sv);

    BigNum v4("10");
    assertEquals("10"s, v4.to_string(), "Instantiation from string (small positive 2)"sv);

    BigNum v5("0");
    assertEquals("0"s, v5.to_string(), "Instantiation from string (zero)"sv);

    assertTrue(BigNum::inf().is_inf(), "BigNum::inf()"sv);
    assertTrue(BigNum::nan().is_nan(), "BigNum::nan()"sv);

    BigNum v6(123456789L);
    assertEquals("123456789"s, v6.to_string(), "Instantiation from integer"sv);
    assertEquals("123,456,789"s, v6.to_pretty_string(), "Pretty string formatting"sv);
}

void runMathTests() {
    BigNum v1("1.23e100");
    BigNum v2("-1.23e100");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    assertEquals(v5, v1 + v2, "Addition (positive + negative)"sv);
    assertEquals("-1.23e100"s, (v2 + v3).to_string(2), "Addition (negative + small positive)"sv);
    assertEquals(BigNum("110"), v3 + v4, "Addition (small positive + small positive)"sv);
    assertEquals(v4, v4 + v5, "Addition (small positive + zero)"sv);

    assertEquals("2.46e100"s, (v1 - v2).to_string(2), "Subtraction (positive - negative)"sv);
    assertEquals("-1.23e100"s, (v2 - v3).to_string(2), "Subtraction (negative - small positive)"sv);
    assertEquals(BigNum("90"), v3 - v4, "Subtraction (small positive - small positive)"sv);
    assertEquals(v4, v4 - v5, "Subtraction (small positive - zero)"sv);

    assertEquals(BigNum("1000"), v3 * v4, "Multiplication (small positive * small positive)"sv);
    assertEquals(v5, v4 * v5, "Multiplication (small positive * zero)"sv);

    assertEquals(v4, v3 / v4, "Division (small positive / small positive)"sv);
    assertTrue((v4 / v5).is_nan(), "Division (small positive / zero)"sv);
}

void runComparisonTests() {
    BigNum v1("1.23e100");
    BigNum v2("-1.23e100");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    assertTrue(!(v1 < v2), "Comparison (v1 < v2)"sv);
    assertTrue(v1 > v2, "Comparison (v1 > v2)"sv);
    assertTrue(v1 != v2, "Comparison (v1 != v2)"sv);
    assertTrue(v1 > v3, "Comparison (v1 > v3)"sv);
    assertTrue(v1 > v4, "Comparison (v1 > v4)"sv);
    assertTrue(v1 > v5, "Comparison (v1 > v5)"sv);

    assertTrue(v2 < v3, "Comparison (v2 < v3)"sv);
    assertTrue(v2 < v4, "Comparison (v2 < v4)"sv);
    assertTrue(v2 < v5, "Comparison (v2 < v5)"sv);

    assertTrue(v3 > v4, "Comparison (v3 > v4)"sv);
    assertTrue(v3 > v5, "Comparison (v3 > v5)"sv);
    
    assertTrue(v4 > v5, "Comparison (v4 > v5)"sv);
}

void runAdvancedMathTests() {
    assertEquals(BigNum(static_cast<intmax_t>(256)), BigNum(static_cast<intmax_t>(16)).pow(2.0), "pow(2.0)"sv);
    assertEquals(BigNum(static_cast<intmax_t>(8)), BigNum(static_cast<intmax_t>(64)).sqrt(), "sqrt()"sv);
    assertEquals(BigNum("1e6"), BigNum(static_cast<intmax_t>(10)).pow(6.0), "pow(6.0)"sv);
    assertEquals(BigNum(static_cast<intmax_t>(42)), BigNum(static_cast<intmax_t>(42)).pow(2.0).sqrt(), "sqrt(pow(2.0))"sv);
    assertEquals(BigNum(static_cast<intmax_t>(3)), BigNum(static_cast<intmax_t>(27)).root(3), "root(3)"sv);

    auto log1 = BigNum(static_cast<intmax_t>(12345)).log10();
    assertTrue(log1.has_value(), "log10(12345) exists"sv);
    assertIsClose(4.09149, *log1, "log10(12345) value"sv);

    auto log2 = BigNum("1.234e1000").log10();
    assertTrue(log2.has_value(), "log10(1.234e1000) exists"sv);
    assertIsClose(1000.0913, *log2, "log10(1.234e1000) value"sv, 1e-4);
}

void runSpecialCaseTests() {
    assertTrue(BigNum::inf().is_inf(), "BigNum::inf() is infinity"sv);
    assertTrue(BigNum::nan().is_nan(), "BigNum::nan() is NaN"sv);

    assertTrue((BigNum::inf() + BigNum::inf()) == BigNum::inf(), "BigNum::inf() + BigNum::inf() is still infinity"sv);
    assertTrue((BigNum::nan() + BigNum::nan()).is_nan(), "BigNum::nan() + BigNum::nan() is NaN"sv);
    assertTrue(!(BigNum::nan() == BigNum::nan()), "BigNum::nan() is not equal to itself"sv);

    assertTrue((BigNum::max() + 1) == BigNum::max(), "BigNum::max() + 1 is still max"sv);
    assertTrue((BigNum::min() - 1) == BigNum::min(), "BigNum::min() - 1 is still min"sv);
}

int main() {
    std::println("Running tests...");
    
    runInstantiationTests();
    runMathTests();
    runComparisonTests();
    runAdvancedMathTests();
    runSpecialCaseTests();

    std::println("Testing finished!");

    return 0;
}
