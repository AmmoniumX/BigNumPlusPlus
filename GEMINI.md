This is a C++ project that implements a big number library.

## Building the project

To build the project, run the following command:

```bash
make
```

## Running the project

To run the project, run the following command:

```bash
./BigNum.out
```

## Project Overview

The project consists of the following files:

* `BigNum.hpp`: The header file for the BigNum library.
* `BigNumTest.cpp`: The main file for the BigNum project, which contains the tests.
* `Makefile`: The makefile for the project.
* `README.md`: The README file for the project.

The `BigNum` class represents a number as `m * 10^e`, where `m` is a `double` and `e` is a `uintmax_t`. The `e` is unsigned, which means that the minimum exponent is 0. This maximizes the maximum representable value, which is `10 * 10^numeric_limits<uintmax_t>::max()`.

### Implementation Details

#### `constexpr` and C++26

The `BigNum` class is designed to be `constexpr`-friendly, allowing for compile-time calculations where possible. However, some C++ standard library functions like `std::log10` and `std::pow` are only `constexpr` in C++26 and later. To handle this, the `CONSTEXPR_IF_CPP26` macro is used to conditionally enable `constexpr` for methods that rely on these functions.

For C++ versions prior to C++26, a tag dispatch mechanism (`NoNormalizeTag`) is used in the constructor to bypass the non-`constexpr` `normalize()` method when creating `constexpr` `BigNum` objects.

#### Preprocessor Macros

- `CONSTEXPR_IF_CPP26`: As described above, this macro enables `constexpr` for methods that use C++26 `constexpr` functions.
- `CONSTEXPR_NEXTAFTER_FALLBACK`: This macro provides a fallback for `std::nextafter` for compilers that do not support it in a `constexpr` context.

#### Tradeoffs and Quirks

- **Limited Range:** The exponent `e` is an unsigned integer (`uintmax_t`), which means that numbers between -1 and 1 (exclusive of 0) cannot be represented. This is a design tradeoff to maximize the upper range of representable numbers.
- **Normalization:** The `normalize()` method is crucial for keeping the mantissa within the range `[-10, 10)`. This ensures that comparisons and arithmetic operations are consistent.
- **Precision:** The `to_string` and `to_pretty_string` methods have a default precision that can be overridden. The serialization precision is fixed at 9 decimal places.
- **Special Values:** The class provides `inf()`, `nan()`, `max()`, and `min()` static methods to represent infinity, Not a Number, and the maximum and minimum representable values.