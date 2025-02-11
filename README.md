# BigNum++
Modified C++ port of https://github.com/veprogames/lua-big-number

Changed to increase maximum range by moving negative numbers outside of the exponent's range (Emin is 0, therefore Emax is higher)

Tradeoff: Cannot store numbers between (-1, 0) or (0, 1), i.e any `|x| < 1` except 0, but those aren't usually needed in the types of games that would use this library
