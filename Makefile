# recommended: c++26, minimum: c++23
# if for some reason you need -ftrapping-math, add -DCONSTEXPR_NEXTAFTER_FALLBACK
CXXFLAGS = -std=c++26 -Wall -Wextra -O3 -march=native -I. -fno-trapping-math

ALL = BigNumTest.out
all: $(ALL)

BigNumTest.out: BigNumTest.o
	$(CXX) $(CXXFLAGS) -o $@ $<

BigNumTest.o: BigNumTest.cpp BigNum.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(ALL) *.o

.PHONY: all clean
