# =============================================================================
#  Root Makefile —— ADS Project 2 一键构建与测试
# =============================================================================

all:
	$(MAKE) -C code all

test:
	$(MAKE) -C code test

plot:
	python3 tools/analyze_groups.py
	python3 tools/plot.py

clean:
	$(MAKE) -C code clean

.PHONY: all test plot clean
