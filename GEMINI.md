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

* `BigNum.hh`: The header file for the BigNum library.
* `BigNumTests.cpp`: The main file for the BigNum project, which contains the tests.
* `Makefile`: The makefile for the project.
* `README.md`: The README file for the project.

The `BigNum` class represents a number as `m * 10^e`, where `m` is a `double` and `e` is a `uintmax_t`. The `e` is unsigned, which means that the minimum exponent is 0. This maximizes the maximum representable value, which is `10 * 10^numeric_limits<uintmax_t>::max()`.
