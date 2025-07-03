# Default compiler
CXX ?= g++

# recommended: c++26, minimum: c++23
# Common flags
CXXFLAGS = -std=c++26 -Wall -Wextra -O3 -I.

# g++ specific flags
# if for some reason you need -ftrapping-math, add -DCONSTEXPR_NEXTAFTER_FALLBACK
ifeq ($(CXX),g++)
    CXXFLAGS += -march=native -fno-trapping-math -fsanitize=address,undefined
endif

# clang++ specific flags
ifeq ($(CXX),clang++)
    CXXFLAGS += -DCONSTEXPR_NEXTAFTER_FALLBACK
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
