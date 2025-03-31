# BigNum++
Modified C++ port of https://github.com/veprogames/lua-big-number

Numbers are represented internally as `m × 10^e`, using `double m` and `uintmax_t e`.

Do note that `e` is unsigned. This maximizes Emax, at the cost of Emin being 0.

Maximum representable value is `10 × 10^ numeric_limits<uintmax_t>::max()`.
