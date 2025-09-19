#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <optional>
#include <string>
#include <string_view>

#include "BigNum.hpp"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

namespace doctest {
template <> struct StringMaker<BigNum> {
    static String convert(const BigNum &value) {
        return value.to_string(9).c_str();
    }
};
} // namespace doctest

TEST_SUITE("Instantiation Tests") {
    TEST_CASE("Instantiation from string") {
        BigNum v1("1.23456789e123456789");
        CHECK_EQ("1.23e123456789"s, v1.to_string(2));

        BigNum v2("-1.23456789e123456789");
        CHECK_EQ("-1.24e123456789"s, v2.to_string(2));

        BigNum v3("100");
        CHECK_EQ("100"s, v3.to_string());

        BigNum v4("10");
        CHECK_EQ("10"s, v4.to_string());

        BigNum v5("0");
        CHECK_EQ("0"s, v5.to_string());
    }

    TEST_CASE("Special values") {
        CHECK(BigNum::inf().is_inf());
        CHECK(BigNum::nan().is_nan());
    }

    TEST_CASE("Instantiation from integer") {
        BigNum v6(123456789L);
        CHECK_EQ("123456789"s, v6.to_string());
        CHECK_EQ("123,456,789"s, v6.to_pretty_string());
    }
}

TEST_SUITE("Math Tests") {
    BigNum v1("1.23e100");
    BigNum v2("-1.23e100");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    TEST_CASE("Addition") {
        CHECK((v1 + v2).abs() < 1e-9);
        CHECK_EQ("-1.23e100"s, (v2 + v3).to_string(2));
        CHECK_EQ(BigNum("110"), v3 + v4);
        CHECK_EQ(v4, v4 + v5);
    }

    TEST_CASE("Subtraction") {
        CHECK_EQ("2.46e100"s, (v1 - v2).to_string(2));
        CHECK_EQ("-1.23e100"s, (v2 - v3).to_string(2));
        CHECK_EQ(BigNum("90"), v3 - v4);
        CHECK_EQ(v4, v4 - v5);
    }

    TEST_CASE("Multiplication") {
        CHECK_EQ(BigNum("1000"), v3 * v4);
        CHECK_EQ(v5, v4 * v5);
    }

    TEST_CASE("Division") {
        CHECK_EQ(v4, v3 / v4);
        CHECK((v4 / v5).is_nan());
    }
}

TEST_SUITE("Comparison Tests") {
    BigNum v1("1.23e100");
    BigNum v2("-1.23e100");
    BigNum v3("100");
    BigNum v4("10");
    BigNum v5("0");

    TEST_CASE("v1 comparisons") {
        CHECK_FALSE(v1 < v2);
        CHECK(v1 > v2);
        CHECK(v1 != v2);
        CHECK(v1 > v3);
        CHECK(v1 > v4);
        CHECK(v1 > v5);
    }

    TEST_CASE("v2 comparisons") {
        CHECK(v2 < v3);
        CHECK(v2 < v4);
        CHECK(v2 < v5);
    }

    TEST_CASE("v3 comparisons") {
        CHECK(v3 > v4);
        CHECK(v3 > v5);
    }

    TEST_CASE("v4 comparisons") {
        CHECK(v4 > v5);
    }
}

TEST_SUITE("Advanced Math Tests") {
    TEST_CASE("Power and square root") {
        CHECK((BigNum(static_cast<intmax_t>(16)).pow(2.0) - BigNum(static_cast<intmax_t>(256))).abs() < 1e-9);
        CHECK((BigNum(static_cast<intmax_t>(64)).sqrt() - BigNum(static_cast<intmax_t>(8))).abs() < 1e-9);
        CHECK((BigNum(static_cast<intmax_t>(10)).pow(6.0) - BigNum("1e6")).abs() < 1e-9);
        CHECK((BigNum(static_cast<intmax_t>(42)).pow(2.0).sqrt() - BigNum(static_cast<intmax_t>(42))).abs() < 1e-9);
        CHECK((BigNum(static_cast<intmax_t>(27)).root(3) - BigNum(static_cast<intmax_t>(3))).abs() < 1e-9);
    }

    TEST_CASE("Logarithm") {
        auto log1 = BigNum(static_cast<intmax_t>(12345)).log10();
        CHECK(log1.has_value());
        CHECK(*log1 == doctest::Approx(4.09149));

        auto log2 = BigNum("1.234e1000").log10();
        CHECK(log2.has_value());
        CHECK(*log2 == doctest::Approx(1000.0913).epsilon(1e-4));
    }
}

TEST_SUITE("Special Case Tests") {
    TEST_CASE("Infinity and NaN") {
        CHECK(BigNum::inf().is_inf());
        CHECK(BigNum::nan().is_nan());
        CHECK((BigNum::inf() + BigNum::inf()) == BigNum::inf());
        CHECK((BigNum::nan() + BigNum::nan()).is_nan());
        CHECK_FALSE(BigNum::nan() == BigNum::nan());
    }

    TEST_CASE("Max and Min") {
        CHECK((BigNum::max() + 1) == BigNum::max());
        CHECK((BigNum::min() - 1) == BigNum::min());
    }
}

TEST_SUITE("Small Number Tests") {
    TEST_CASE("Small number operations") {
        BigNum res1 = BigNum(1) / BigNum(2);
        CHECK((res1 - BigNum(0.5)).abs() < 1e-5);

        BigNum res2 = BigNum(1) - BigNum(0.1);
        CHECK((res2 - BigNum(0.9)).abs() < 1e-5);

        BigNum res3 = BigNum(0.1) + BigNum(0.2);
        CHECK((res3 - BigNum(0.3)).abs() < 1e-5);

        BigNum res4 = BigNum(0.5) * BigNum(0.25);
        CHECK((res4 - BigNum(0.125)).abs() < 1e-5);
    }
}
