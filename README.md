# BigNum++
Lightweight, high-performance C++ port of https://github.com/Patashu/break_infinity.js

Relies heavily on constexpr optimizations.

BigNums are represented internally as `m × 10^e`, using `double m` and `uintmax_t e`.

Note: `e` is unsigned. This means that BigNums can only store numbers as small as regular doubles, but the upper limit is higher.

Maximum representable value is `std::nextafter(10.0, 0.0) × 10^numeric_limits<uintmax_t>::max()`, effectively `1e(2^64)`.

## Build information
See the example `Makefile` for build requirements.
