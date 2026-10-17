# 简单复现实验

需要 Python 3 和支持 **C++11** 的 g++。打开终端，进入项目根目录，按顺序执行下面的命令。

## 1. 准备路网

下载全部 12 个区域的距离和通行时间版本，并直接解压到 `data/assets/d/`、`data/assets/t/`。已经下载的压缩包会跳过下载；如果这两个目录里的 24 份 `.gr` 已齐全，可以跳过这一步。

```sh
python tmp/prepare_data.py
```

## 2. 编译程序

```sh
g++ -O3 -std=c++11 -Wall -Wextra -o data/gen_queries.exe data/gen_queries.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o data/gen_custom.exe data/gen_custom.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o code/selftest.exe code/selftest.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o code/dijkstra_bench.exe code/dijkstra_bench.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o code/optimize_bench.exe code/optimize_bench.cpp
```

## 3. 生成测试输入

先建目录，再为路网生成每张图 1000 对随机查询，同时生成两组人工图输入。

```sh
python -c "from pathlib import Path; [Path(p).mkdir(parents=True, exist_ok=True) for p in ['data/queries/d', 'data/queries/t', 'data/queries/custom', 'results/reproduce']]"
./data/gen_queries.exe
./data/gen_custom.exe
```

## 4. 检查正确性

看到 `ALL TESTS PASSED` 就表示四种堆的结果与 STL 参照一致。

```sh
./code/selftest.exe
```

## 5. 跑小规模路网实验

只跑最小的 NY、BAY、COL，每个区域分别测试距离和通行时间，共 6 张图，不跑全部 24 张图。

```sh
./code/dijkstra_bench.exe -mini --out results/reproduce/mini.csv
```

## 6. 跑补充实验

人工图分别测试低减键和高减键场景。

```sh
./code/dijkstra_bench.exe --data 1 --graph data/queries/custom/low.in --p2p data/queries/custom/low_queries.in --net custom-low --out results/reproduce/custom_low.csv
./code/dijkstra_bench.exe --data 1 --out results/reproduce/custom_high.csv
```

再测试按起点合并查询的优化版，同样只跑 6 张路网。

```sh
./code/optimize_bench.exe -mini --out results/reproduce/optimize_mini.csv
```

结果都在 `results/reproduce/`。CSV 会追加写入，重复实验前请把上一次的结果移走或换一个输出文件名。
