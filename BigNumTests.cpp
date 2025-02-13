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
}

void runMathTests() {
    BigNum v1("123456789e123456789");
    BigNum v2("-123456789e123456789");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    cout << "v1 + v2: " << v1 + v2 << endl;
    cout << "v2 + v3: " << v2 + v3 << endl;
    cout << "v3 + v4: " << v3 + v4 << endl;
    cout << "v4 + v5: " << v4 + v5 << endl;

    cout << "v1 - v2: " << v1 - v2 << endl;
    cout << "v2 - v3: " << v2 - v3 << endl;
    cout << "v3 - v4: " << v3 - v4 << endl;
    cout << "v4 - v5: " << v4 - v5 << endl;

    cout << "v1 * v2: " << v1 * v2 << endl;
    cout << "v2 * v3: " << v2 * v3 << endl;
    cout << "v3 * v4: " << v3 * v4 << endl;
    cout << "v4 * v5: " << v4 * v5 << endl;

    cout << "v1 / v2: " << v1 / v2 << endl;
    cout << "v2 / v3: " << v2 / v3 << endl;
    cout << "v3 / v4: " << v3 / v4 << endl;
    cout << "v4 / v5: " << v4 / v5 << endl;
}

void runComparisonTests() {
    BigNum v1("123456789e123456789");
    BigNum v2("-123456789e123456789");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    cout << "v1 < v1: " << (v1 < v1) << endl;
    cout << "v1 <= v1: " << (v1 <= v1) << endl;
    cout << "v1 > v1: " << (v1 > v1) << endl;
    cout << "v1 >= v1: " << (v1 >= v1) << endl;
    cout << "v1 == v1: " << (v1 == v1) << endl;
    cout << "v1 != v1: " << (v1 != v1) << endl;

    cout << "v1 < v2: " << (v1 < v2) << endl;
    cout << "v1 <= v2: " << (v1 <= v2) << endl;
    cout << "v1 > v2: " << (v1 > v2) << endl;
    cout << "v1 >= v2: " << (v1 >= v2) << endl;
    cout << "v1 == v2: " << (v1 == v2) << endl;
    cout << "v1 != v2: " << (v1 != v2) << endl;

    cout << "v1 < v3: " << (v1 < v3) << endl;
    cout << "v1 <= v3: " << (v1 <= v3) << endl;
    cout << "v1 > v3: " << (v1 > v3) << endl;
    cout << "v1 >= v3: " << (v1 >= v3) << endl;
    cout << "v1 == v3: " << (v1 == v3) << endl;
    cout << "v1 != v3: " << (v1 != v3) << endl;

    cout << "v1 < v4: " << (v1 < v4) << endl;
    cout << "v1 <= v4: " << (v1 <= v4) << endl;
    cout << "v1 > v4: " << (v1 > v4) << endl;
    cout << "v1 >= v4: " << (v1 >= v4) << endl;
    cout << "v1 == v4: " << (v1 == v4) << endl;
    cout << "v1 != v4: " << (v1 != v4) << endl;

    cout << "v1 < v5: " << (v1 < v5) << endl;
    cout << "v1 <= v5: " << (v1 <= v5) << endl;
    cout << "v1 > v5: " << (v1 > v5) << endl;
    cout << "v1 >= v5: " << (v1 >= v5) << endl;
    cout << "v1 == v5: " << (v1 == v5) << endl;
    cout << "v1 != v5: " << (v1 != v5) << endl;
}

void runAdvancedMathTests() {
    cout << "16^2: " << BigNum(16).pow(2) << endl;
    cout << "sqrt(64): " << BigNum(64).sqrt() << endl;
    cout << "10^6: " << BigNum(10).pow(6) << endl;
    cout << "sqrt(42^2): " << BigNum(42).pow(2).sqrt() << endl;
}

int main() {
    runInstantiationTests();
    runMathTests();
    runComparisonTests();
    runAdvancedMathTests();
    return 0;
}