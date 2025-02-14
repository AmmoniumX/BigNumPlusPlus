# BigNum++
Modified C++ port of https://github.com/veprogames/lua-big-number

Numbers are represented internally as `m × 10^e`.

Do note that the type of `e` is `uintmax_t`. This maximizes Emax, at the cost of Emin being 0.

Tradeoff: Cannot store numbers between (-1, 0) or (0, 1), i.e any `|x| < 1` except 0, but for the scope of this library, it is assumed that these are not needed (the user will only use large whole numbers).

Maximum representable value is `10 × 10^ numeric_limits<uintmax_t>::max()`.
