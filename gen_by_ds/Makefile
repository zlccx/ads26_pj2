# =============================================================================
#  Makefile —— 使用 MinGW g++ (mingw32-make) 或 GNU make 构建
#     make          # 编译 selftest 与 bench 两个程序到 build/
#     make test     # 编译并运行正确性测试
#     make clean
# =============================================================================

CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra -Wno-unused-parameter -static
BUILD    := build

.PHONY: all clean test

all: $(BUILD)/selftest.exe $(BUILD)/bench.exe

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/selftest.exe: src/selftest.cpp src/heaps.h src/dijkstra.h src/dimacs.h
	$(CXX) $(CXXFLAGS) -o $@ src/selftest.cpp

$(BUILD)/bench.exe: src/bench.cpp src/heaps.h src/dijkstra.h src/dimacs.h
	$(CXX) $(CXXFLAGS) -o $@ src/bench.cpp

test: $(BUILD)/selftest.exe
	$(BUILD)/selftest.exe

clean:
	rm -rf $(BUILD)
