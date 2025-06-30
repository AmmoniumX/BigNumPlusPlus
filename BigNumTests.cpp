#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <string>
#include <sstream>
#include <optional>

#include "BigNum.hpp"

using namespace std;

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
constexpr void assertEquals(const T& expected, const T& actual, const std::string& testName) {
    if (expected != actual) {
        std::cerr << "Test failed: " << testName << std::endl;
        std::cerr << "  Expected: " << expected << std::endl;
        std::cerr << "  Got:      " << actual << std::endl;
        exit(1);
    }
}

constexpr void assertTrue(bool condition, const std::string& testName) {
    if (!condition) {
        std::cerr << "Test failed: " << testName << std::endl;
        std::cerr << "  Expected: true" << std::endl;
        std::cerr << "  Got:      false" << std::endl;
        exit(1);
    }
}

void assertIsClose(double expected, double actual, const std::string& testName, double tolerance = 1e-5) {
    if (std::abs(expected - actual) > tolerance) {
        std::cerr << "Test failed: " << testName << std::endl;
        std::cerr << "  Expected: " << expected << std::endl;
        std::cerr << "  Got:      " << actual << std::endl;
        exit(1);
    }
}


void runInstantiationTests() {
    BigNum v1("1.23456789e123456789");
    assertEquals("1.23e123456789"s, v1.to_string(2), "Instantiation from string (positive)");
    
    BigNum v2("-1.23456789e123456789");
    assertEquals("-1.24e123456789"s, v2.to_string(2), "Instantiation from string (negative)");

    BigNum v3("100");
    assertEquals("100"s, v3.to_string(), "Instantiation from string (small positive)");

    BigNum v4("10");
    assertEquals("10"s, v4.to_string(), "Instantiation from string (small positive 2)");

    BigNum v5("0");
    assertEquals("0"s, v5.to_string(), "Instantiation from string (zero)");

    assertTrue(BigNum::inf().is_inf(), "BigNum::inf()");
    assertTrue(BigNum::nan().is_nan(), "BigNum::nan()");

    BigNum v6(123456789L);
    assertEquals("123456789"s, v6.to_string(), "Instantiation from integer");
    assertEquals("123,456,789"s, v6.to_pretty_string(), "Pretty string formatting");
}

void runMathTests() {
    BigNum v1("1.23e100");
    BigNum v2("-1.23e100");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    assertEquals(v5, v1 + v2, "Addition (positive + negative)");
    assertEquals("-1.23e100"s, (v2 + v3).to_string(2), "Addition (negative + small positive)");
    assertEquals(BigNum("110"), v3 + v4, "Addition (small positive + small positive)");
    assertEquals(v4, v4 + v5, "Addition (small positive + zero)");

    assertEquals("2.46e100"s, (v1 - v2).to_string(2), "Subtraction (positive - negative)");
    assertEquals("-1.23e100"s, (v2 - v3).to_string(2), "Subtraction (negative - small positive)");
    assertEquals(BigNum("90"), v3 - v4, "Subtraction (small positive - small positive)");
    assertEquals(v4, v4 - v5, "Subtraction (small positive - zero)");

    assertEquals(BigNum("1000"), v3 * v4, "Multiplication (small positive * small positive)");
    assertEquals(v5, v4 * v5, "Multiplication (small positive * zero)");

    assertEquals(v4, v3 / v4, "Division (small positive / small positive)");
    assertTrue((v4 / v5).is_nan(), "Division (small positive / zero)");
}

void runComparisonTests() {
    BigNum v1("1.23e100");
    BigNum v2("-1.23e100");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    assertTrue(!(v1 < v2), "Comparison (v1 < v2)");
    assertTrue(v1 > v2, "Comparison (v1 > v2)");
    assertTrue(v1 != v2, "Comparison (v1 != v2)");
    assertTrue(v1 > v3, "Comparison (v1 > v3)");
    assertTrue(v1 > v4, "Comparison (v1 > v4)");
    assertTrue(v1 > v5, "Comparison (v1 > v5)");

    assertTrue(v2 < v3, "Comparison (v2 < v3)");
    assertTrue(v2 < v4, "Comparison (v2 < v4)");
    assertTrue(v2 < v5, "Comparison (v2 < v5)");

    assertTrue(v3 > v4, "Comparison (v3 > v4)");
    assertTrue(v3 > v5, "Comparison (v3 > v5)");
    
    assertTrue(v4 > v5, "Comparison (v4 > v5)");
}

void runAdvancedMathTests() {
    assertEquals(BigNum(static_cast<intmax_t>(256)), BigNum(static_cast<intmax_t>(16)).pow(2.0), "pow(2.0)");
    assertEquals(BigNum(static_cast<intmax_t>(8)), BigNum(static_cast<intmax_t>(64)).sqrt(), "sqrt()");
    assertEquals(BigNum("1e6"), BigNum(static_cast<intmax_t>(10)).pow(6.0), "pow(6.0)");
    assertEquals(BigNum(static_cast<intmax_t>(42)), BigNum(static_cast<intmax_t>(42)).pow(2.0).sqrt(), "sqrt(pow(2.0))");
    assertEquals(BigNum(static_cast<intmax_t>(3)), BigNum(static_cast<intmax_t>(27)).root(3), "root(3)");

    auto log1 = BigNum(static_cast<intmax_t>(12345)).log10();
    assertTrue(log1.has_value(), "log10(12345) exists");
    assertIsClose(4.09149, *log1, "log10(12345) value");

    auto log2 = BigNum("1.234e1000").log10();
    assertTrue(log2.has_value(), "log10(1.234e1000) exists");
    assertIsClose(1000.0913, *log2, "log10(1.234e1000) value", 1e-4);
}

int main() {
    cout << "Running tests..." << endl;
    
    runInstantiationTests();
    runMathTests();
    runComparisonTests();
    runAdvancedMathTests();

    cout << "All tests passed!" << endl;

    return 0;
}
