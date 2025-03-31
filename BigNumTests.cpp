#include <iostream>
#include <cstdlib>

#include "BigNum.hh"

using namespace std;

void runInstantiationTests() {
    BigNum v1("123456789e123456789");
    cout << "123456789e123456789: " << v1 << endl;
    
    BigNum v2("-123456789e123456789");
    cout << "-123456789e123456789: " << v2 << endl;

    BigNum v3("100");
    cout << "100: " << v3 << endl;

    BigNum v4("10");
    cout << "10: " << v4 << endl;

    BigNum v5("0");
    cout << "0: " << v5 << endl;

    cout << "BigNum::inf(): " << BigNum::inf() << endl;
    cout << "BigNum::nan(): " << BigNum::nan() << endl;
    cout << "BigNum::max(): " << BigNum::max() << endl;
    cout << "BigNum::min(): " << BigNum::min() << endl;

    BigNum v6(123456789);
    cout << "123456789: " << v6 << endl;
    cout << "123456789 (pretty): " << v6.to_pretty_string() << endl;
}

void runMathTests() {
    BigNum v1("123456789e123456789");
    BigNum v2("-123456789e123456789");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    cout << v1 << " + " << v2 << " = " << v1 + v2 << endl;
    cout << v2 << " + " << v3 << " = " << v2 + v3 << endl;
    cout << v3 << " + " << v4 << " = " << v3 + v4 << endl;
    cout << v4 << " + " << v5 << " = " << v4 + v5 << endl;

    cout << v1 << " - " << v2 << " = " << v1 - v2 << endl;
    cout << v2 << " - " << v3 << " = " << v2 - v3 << endl;
    cout << v3 << " - " << v4 << " = " << v3 - v4 << endl;
    cout << v4 << " - " << v5 << " = " << v4 - v5 << endl;

    cout << v1 << " * " << v2 << " = " << v1 * v2 << endl;
    cout << v2 << " * " << v3 << " = " << v2 * v3 << endl;
    cout << v3 << " * " << v4 << " = " << v3 * v4 << endl;
    cout << v4 << " * " << v5 << " = " << v4 * v5 << endl;

    cout << v1 << " / " << v2 << " = " << v1 / v2 << endl;
    cout << v2 << " / " << v3 << " = " << v2 / v3 << endl;
    cout << v3 << " / " << v4 << " = " << v3 / v4 << endl;
    cout << v4 << " / " << v5 << " = " << v4 / v5 << endl;
}

void runComparisonTests() {
    BigNum v1("123456789e123456789");
    BigNum v2("-123456789e123456789");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    cout << v1 << " < " << v2 << ": " << (v1 < v2) << endl;
    cout << v1 << " <= " << v2 << ": " << (v1 <= v2) << endl;
    cout << v1 << " > " << v2 << ": " << (v1 > v2) << endl;
    cout << v1 << " >= " << v2 << ": " << (v1 >= v2) << endl;
    cout << v1 << " == " << v2 << ": " << (v1 == v2) << endl;
    cout << v1 << " != " << v2 << ": " << (v1 != v2) << endl;

    cout << v1 << " < " << v2 << ": " << (v1 < v2) << endl;
    cout << v1 << " <= " << v2 << ": " << (v1 <= v2) << endl;
    cout << v1 << " > " << v2 << ": " << (v1 > v2) << endl;
    cout << v1 << " >= " << v2 << ": " << (v1 >= v2) << endl;
    cout << v1 << " == " << v2 << ": " << (v1 == v2) << endl;
    cout << v1 << " != " << v2 << ": " << (v1 != v2) << endl;

    cout << v1 << " < " << v3 << ": " << (v1 < v3) << endl;
    cout << v1 << " <= " << v3 << ": " << (v1 <= v3) << endl;
    cout << v1 << " > " << v3 << ": " << (v1 > v3) << endl;
    cout << v1 << " >= " << v3 << ": " << (v1 >= v3) << endl;
    cout << v1 << " == " << v3 << ": " << (v1 == v3) << endl;
    cout << v1 << " != " << v3 << ": " << (v1 != v3) << endl;

    cout << v1 << " < " << v4 << ": " << (v1 < v4) << endl;
    cout << v1 << " <= " << v4 << ": " << (v1 <= v4) << endl;
    cout << v1 << " > " << v4 << ": " << (v1 > v4) << endl;
    cout << v1 << " >= " << v4 << ": " << (v1 >= v4) << endl;
    cout << v1 << " == " << v4 << ": " << (v1 == v4) << endl;
    cout << v1 << " != " << v4 << ": " << (v1 != v4) << endl;

    cout << v1 << " < " << v5 << ": " << (v1 < v5) << endl;
    cout << v1 << " <= " << v5 << ": " << (v1 <= v5) << endl;
    cout << v1 << " > " << v5 << ": " << (v1 > v5) << endl;
    cout << v1 << " >= " << v5 << ": " << (v1 >= v5) << endl;
    cout << v1 << " == " << v5 << ": " << (v1 == v5) << endl;
    cout << v1 << " != " << v5 << ": " << (v1 != v5) << endl;
}

void runAdvancedMathTests() {
    cout << "16^2: " << BigNum(16).pow(2.0f) << endl;
    cout << "sqrt(64): " << BigNum(64).sqrt() << endl;
    cout << "10^6: " << BigNum(10).pow(6.0f) << endl;
    cout << "sqrt(42^2): " << BigNum(42).pow(2.0f).sqrt() << endl;
    cout << "27.root(3): " << BigNum(27).root(3.0f) << endl;

    cout << "12345.log10(): ";
    auto v = BigNum(12345).log10();
    if (v) { cout << std::to_string(*v); } else { cout << "nullopt"; } 
    cout << endl;

    cout << "1.234e1000.log10(): ";
    auto v2 = BigNum("1.234e1000").log10();
    if (v2) { cout << std::to_string(*v2); } else { cout << "nullopt"; }
    cout << endl;
}

int main() {
    runInstantiationTests();
    runMathTests();
    runComparisonTests();
    runAdvancedMathTests();
    return 0;
}