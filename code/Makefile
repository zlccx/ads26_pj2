# =============================================================================
#  Makefile —— ADS Project 2 编译脚本（全自包含，不依赖外部 .h，纯标准库）
# =============================================================================

CXX := g++
CXXFLAGS := -O3 -std=c++17 -Wall -Wextra

TARGETS := dijkstra_bench selftest synth_bench reset_bench

all: $(TARGETS)

dijkstra_bench: dijkstra_bench.cpp
	$(CXX) $(CXXFLAGS) -o $@ dijkstra_bench.cpp

selftest: selftest.cpp
	$(CXX) $(CXXFLAGS) -o $@ selftest.cpp

synth_bench: synth_bench.cpp
	$(CXX) $(CXXFLAGS) -o $@ synth_bench.cpp

reset_bench: reset_bench.cpp
	$(CXX) $(CXXFLAGS) -o $@ reset_bench.cpp

test: selftest
	./selftest

clean:
	rm -f $(TARGETS)

.PHONY: all test clean
