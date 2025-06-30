CXXFLAGS = -std=c++23 -Wall -Wextra -O3 -march=native -I.

ALL = BigNum.out
all: $(ALL)

BigNum.out: BigNum.o
	$(CXX) $(CXXFLAGS) -o $@ $<

BigNum.o: BigNumTests.cpp BigNum.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(ALL) *.o

.PHONY: all clean
