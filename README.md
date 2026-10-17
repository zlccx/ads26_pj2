# ADS Project 2：使用不同堆实现 Dijkstra

> **此文档为详细说明，简单复现实验请参考 [GUIDE.md](GUIDE.md)**

实现二叉堆、二项堆、斐波那契堆和惰性删除二叉堆，使用真实 USA 路网与人工图比较性能

所有 C++ 程序采用 **C++11**，仅使用标准库；数据下载和解压脚本使用 Python 3 标准库
以下命令均在项目根目录运行

## 数据目录

```text
data/
  assets/d/USA-road-d.<网络名>.gr   # 物理距离边权
  assets/t/USA-road-t.<网络名>.gr   # 通行时间边权
  queries/d/<网络名>.in            # distance 点对查询
  queries/t/<网络名>.in            # travel time 点对查询
  queries/custom/                 # 人工图和对应点对查询
  gen_queries.cpp                 # 路网查询生成器
  gen_custom.cpp                  # 人工图生成器
tmp/
  prepare_data.py                  # 全量下载与解压脚本
  USA-road-*.gr.gz                 # 下载缓存，不随报告提交
```

网络顺序为 NY、BAY、COL、FLA、NW、NE、CAL、LKS、E、W、CTR、USA
大型原始文件只保存在本地；脚本、源码与说明可以提交，报告仅提供数据下载链接

## 下载并解压路网

```sh
python tmp/prepare_data.py
```

默认以 4 个并行任务处理官网全部 12 个网络的 distance、travel time 两个版本，共 24 个压缩包。
网络包括 NY、BAY、COL、FLA、NW、NE、CAL、LKS、E、W、CTR、USA。

- 压缩包保存在 `tmp/USA-road-d.<网络名>.gr.gz` 或 `tmp/USA-road-t.<网络名>.gr.gz`。
- 已有非空压缩包会跳过下载；下载失败最多尝试 3 次，未完成文件使用 `.part` 后缀。
- 每次运行均解压、校验 gzip CRC 并检查图头部，成功后才替换目标文件。
- 解压后的完整纯文本直接存为 `data/assets/d/USA-road-d.<网络名>.gr` 和 `data/assets/t/USA-road-t.<网络名>.gr`，无需手动移动。
- 路网保留官方 `.gr` 扩展名和原始 DIMACS 内容，不改变边权；查询文件使用 `.in`。
- 若已有压缩包损坏，删除报错的压缩包后重新运行。

可用 `--workers 2` 调整并行数；压缩包与全部解压文件会占用较多磁盘空间。脚本可从任意工作目录启动，默认路径相对于脚本位置确定。

来源：[DIMACS Challenge 9 官方下载页](http://www.diag.uniroma1.it/challenge9/download.shtml)。报告只列数据下载链接，不附带大型路网文件。

## 生成路网查询

```sh
g++ -O3 -std=c++11 -Wall -Wextra -o data/gen_queries.exe data/gen_queries.cpp
```

先确保 `data/queries/d/` 和 `data/queries/t/` 存在，然后运行生成器：

```sh
python -c "from pathlib import Path; Path('data/queries/d').mkdir(parents=True, exist_ok=True); Path('data/queries/t').mkdir(parents=True, exist_ok=True)"
./data/gen_queries.exe
```

每张图生成 1000 对查询，写入 `data/queries/d/<网络名>.in` 或 `data/queries/t/<网络名>.in`，会覆盖已有查询。
基础种子固定为 `20261017`，加网络在生成器列表中的固定下标；同一路网两个权重版本使用相同种子。
同一标准库环境中结果可复现；各堆应读取同一份查询文件。

查询起终点在 `[1,n]` 内独立均匀抽样，保证二者不同，允许重复点对，不保证可达；
生成器只读取纯文本头部，不负责下载、解压或创建输出目录；
相同路网两种权重的顶点数相同时，使用相同种子生成相同点对；
也可以只生成一张图的查询，此模式种子固定为 `20261017`：

```sh
./data/gen_queries.exe data/assets/d/USA-road-d.NY.gr data/queries/d/NY.in
```

## 文件说明

- heaps.h：四种堆的公共实现
- bench_common.h：两个 bench 共用的命令行、统计输出和 mini 数据调度
- offline.h：同源离线查询分组与答案还原
- optimize_bench.cpp：局部状态清理与同源查询合并的优化测试入口
- dijkstra.h：图读取、公共统计结构、堆接口适配和模板 Dijkstra
- dijkstra_bench.cpp：统一性能测试入口
- selftest.cpp：以 STL priority_queue 为参照验证四种堆的最短路结果

## 编译程序

```sh
g++ -O3 -std=c++11 -Wall -Wextra -o code/dijkstra_bench.exe code/dijkstra_bench.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o code/selftest.exe code/selftest.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o code/optimize_bench.exe code/optimize_bench.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o data/gen_custom.exe data/gen_custom.cpp
```

## 正确性自测

```sh
./code/selftest.exe
```

## Mini 路网测试

```sh
./code/dijkstra_bench.exe -mini --out results/mini.csv
```

仅依次测试 NY、BAY、COL 的 d/t 版本，共六张图
仍使用各图完整的 1000 对查询，默认不做完整单源测试
可用 --heaps "bin,fib" 指定堆类型
-mini 仅适用于 --data 0，此模式自动选择六张图及查询，忽略单图路径与网络名
六张图结果追加到同一个 CSV，通过 net 列的 NY-d、NY-t 等标识区分

## 指定路网测试

```sh
./code/dijkstra_bench.exe --data 0 --graph data/assets/d/USA-road-d.NY.gr --net NY-d --p2p data/queries/d/NY.in --heaps "bin,binom,fib,lazy" --full-sources 16 --out results/distance_results.csv
./code/dijkstra_bench.exe --data 0 --graph data/assets/t/USA-road-t.NY.gr --net NY-t --p2p data/queries/t/NY.in --heaps "bin,binom,fib,lazy" --full-sources 16 --out results/travel_time_results.csv
```

可将 NY 换为其他网络名

## 人工数据生成与测试

先创建人工输入目录：

```sh
python -c "from pathlib import Path; Path('data/queries/custom').mkdir(parents=True, exist_ok=True)"
```

然后执行

```sh
./data/gen_custom.exe
```

可选规模参数：./data/gen_custom.exe 300 3000
生成 low.in、high.in 两张图及各自的 low_queries.in、high_queries.in
每组固定 1000 对源点到目标层的查询，这组查询专门控制减键频率，并非均匀随机点对

```sh
./code/dijkstra_bench.exe --data 1 --out results/custom_high.csv
./code/dijkstra_bench.exe --data 1 --graph data/queries/custom/low.in --p2p data/queries/custom/low_queries.in --net custom-low --out results/custom_low.csv
```

## 参数与结果说明

- --data 0：默认读取 distance NY 路网与查询
- --data 1：默认读取人工 high 图与查询
- 两者共用模板测试逻辑，显式 --graph 和 --p2p 可以覆盖默认路径
- --full-sources N：额外随机选择 N 个源点做完整单源测试，默认不开启
- --dump 前缀：为每种堆额外输出逐查询数据
- 结果 CSV 采用追加模式，重复实验请使用新的输出文件或先备份旧结果
- 各种堆应使用同一图、同一批查询和相同编译参数
- 人工高减键图会使惰性堆产生大量重复元素，运行时间和内存开销可能明显增大

- 原始结果来自重构前的程序，不自动替换，新的性能数据应重新测量
- Linux/macOS 可在编译和运行命令中同时去掉 .exe 后缀


## 离线同源优化测试

```sh
./code/optimize_bench.exe -mini --out results/optimize_mini.csv
```

参数与普通 bench 相同，也支持 `--data 1`、`--heaps`、`--graph`、`--p2p` 和 `--dump`
离线模式只接受点对查询，按起点排序后，每个不同起点只执行一次完整 Dijkstra，再按输入顺序还原所有答案
只清理本轮访问点的状态，不逐轮全数组 memset；普通模板 Dijkstra 也已采用这种局部清理方式
离线排序、分组和答案收集计入查询耗时，图和查询文件读取、CSV 导出不计入

CSV 的 mode 为 offline，runs 仍表示输入点对数，终端另外显示实际的 dijkstra_runs
操作计数对应实际执行的各次完整单源搜索，平均耗时按原始查询数计算
离线 checksum 累计查询答案，与普通模式累计搜索过程距离的 checksum 含义不同，不应跨模式直接比较
`--dump` 导出 idx、src、dst、dist，按原始查询顺序排列，不给共享搜索虚构单条查询耗时
显式 `--full-sources` 仍保留额外完整单源测试的原有行为，与同源查询分组分开统计

起点重复较多时可以减少搜索次数；起点几乎各不相同时，完整搜索可能比原来的目标早停更慢。