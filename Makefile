CXXFLAGS = -std=c++23 -Wall -Wextra -O3

ALL = BigNum.out
all: $(ALL)

BigNum.out: BigNum.o
	$(CXX) $(CXXFLAGS) -o $@ $<

BigNum.o: BigNumTests.cpp BigNum.hh
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(ALL) *.o

.PHONY: all clean
