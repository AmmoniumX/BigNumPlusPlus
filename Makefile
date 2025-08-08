# Default compiler
CXX ?= g++

# recommended: gnu++26, minimum: c++23
# Common flags
CXXFLAGS =-std=gnu++26 -Wall -Wextra -O3 -I. -march=native
CXXFLAGS +=-fno-trapping-math -DNO_TRAPPING_MATH # add this to ensure constexpr maths
# if for some reason you need -ftrapping-math, add -DCONSTEXPR_NEXTAFTER_FALLBACK

# g++ specific flags
ifeq ($(CXX),g++)
    CXXFLAGS +=-fsanitize=address,undefined
endif

# clang++ specific flags
ifeq ($(CXX),clang++)
	CXXFLAGS +=-DCONSTEXPR_NEXTAFTER_FALLBACK # clang is currently behind on constexpr <cmath> :(
endif

ALL = BigNumTest.out
all: $(ALL)

BigNumTest.out: BigNumTest.o
	$(CXX) $(CXXFLAGS) -o $@ $<

BigNumTest.o: BigNumTest.cpp BigNum.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(ALL) *.o

.PHONY: all clean
