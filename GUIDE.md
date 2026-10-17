# 简单复现实验

需要 Python 3 和支持 **C++11** 的 g++。打开终端，进入项目根目录，按顺序执行下面的命令。

## 1. 准备路网

默认只准备 NY、BAY、COL、FLA、NW、NE、CAL 的 d 七图，以及 NY、BAY、COL 的 t 三图，共十张。加 `-full` 才下载全部二十四张。已有压缩包跳过下载。

```sh
python tmp/prepare_data.py
```

## 2. 编译程序

```sh
g++ -O3 -std=c++11 -Wall -Wextra -o data/gen_queries.exe data/gen_queries.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o code/selftest.exe code/selftest.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o code/dijkstra_bench.exe code/dijkstra_bench.cpp
```

## 3. 生成测试输入

先建目录，再为路网生成每张图 1000 对随机查询。

```sh
python -c "from pathlib import Path; [Path(p).mkdir(parents=True, exist_ok=True) for p in ['data/queries/d', 'data/queries/t', 'results/reproduce']]"
./data/gen_queries.exe
```

## 4. 检查正确性

看到 `ALL TESTS PASSED` 就表示四种堆的结果与 STL 参照一致。

```sh
./code/selftest.exe
```

## 5. 标准测试

```sh
./code/dijkstra_bench.exe
```

默认先测 NY、BAY、COL 的 d/t 六图，在控制台输出 d/t 平均耗时、t÷d 和堆间倍率；接着独立测试 NY 到 CAL 的 d 七图并输出堆间对比。前三张 d 图会重新测量。默认每张图使用生成的 1000 对查询，比较四种堆，不额外执行完整单源。

固定比较四种堆，仅输出到控制台。需要保存时使用 shell 重定向：`./code/dijkstra_bench.exe > fulltest.res 2>&1`。

## 6. 全量模式（可选）

```sh
python tmp/prepare_data.py -full
./data/gen_queries.exe -full
./code/dijkstra_bench.exe -full
```

全量模式测试全部十二个区域的 d/t 共二十四图；它与默认两阶段流程互斥。`-mini` 只执行第一阶段。各模式互斥，不提供单图路径和堆筛选参数。

## 专用测试

`data/gen_custom.cpp` 生成两个模式的独立输入，不覆盖标准查询：

```sh
g++ -O3 -std=c++11 -Wall -Wextra -o data/gen_custom.exe data/gen_custom.cpp
./data/gen_custom.exe
```

不带参数生成两组；也可用 `./data/gen_custom.exe -custom 0` 或 `-custom 1` 只生成对应组。

- `./code/dijkstra_bench.exe -custom 0`：仅跑 NY 的 d 图，读取 `data/queries/custom/NY_5000.in` 的 5000 条随机点对，固定种子，与标准查询分开存储。同一标准库环境下，前 1000 条与标准生成器一致。
- `./code/dijkstra_bench.exe -custom 1`：仅跑 `data/queries/custom/dense.in` 和 `dense_queries.in`。默认 k=r=3000，共 6001 个顶点、9,003,000 条边；1000 条查询均从源点 1 到目标层，目标循环选择。每次搜索有 8,997,000 次减键，重复查询仍独立运行，无同源合并。

人工图采用 A=r+1、B=k+1，中间点 i 的距离为 i，经它到目标 j 的候选距离为 B+(k-i)A+j；因此边权为 B+(k-i)A+j-i。每轮目标距离大幅下降，并倒序扫描目标编号，制造上浮压力；不预设斐波那契堆一定最快。

生成器支持 `--k N --r N --queries Q` 调整人工图，主要用于小规模校验，不影响模式 0 固定 5000 条。两个 bench 模式均固定比较四种堆，输出控制台汇总；不能与其他模式组合。

若 bench 正在运行，请将编译输出写入另一个文件名，或待该轮结束再编译。
