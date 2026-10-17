# ADS Project 2：使用不同堆实现 Dijkstra

> **此文档为详细说明，简单复现实验请参考 [GUIDE.md](GUIDE.md)**

实现二叉堆、二项堆、斐波那契堆和惰性删除二叉堆，使用真实 USA 路网比较性能

所有 C++ 程序采用 **C++11**，仅使用标准库；数据下载和解压脚本使用 Python 3 标准库
以下命令均在项目根目录运行

## 数据目录

```text
data/
  assets/d/USA-road-d.<网络名>.gr   # 物理距离边权
  assets/t/USA-road-t.<网络名>.gr   # 通行时间边权
  queries/d/<网络名>.in            # distance 点对查询
  queries/t/<网络名>.in            # travel time 点对查询
  gen_queries.cpp                 # 路网查询生成器
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

默认以 4 个并行任务处理 NY 到 CAL 的七张 distance 图，以及 NY、BAY、COL 的三张 travel time 图，共十个压缩包。使用 `python tmp/prepare_data.py -full` 才下载全部十二个区域的两个版本，共二十四张图。
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

默认只为七张 d 和三张 t 图生成查询；`./data/gen_queries.exe -full` 为全部二十四张生成查询。每张图生成 1000 对查询，写入 `data/queries/d/<网络名>.in` 或 `data/queries/t/<网络名>.in`，会覆盖已有查询。
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
- bench_common.h：命令行参数、统计输出和数据集调度
- dijkstra.h：图读取、公共统计结构、堆接口适配和模板 Dijkstra
- dijkstra_bench.cpp：统一性能测试程序
- selftest.cpp：以 STL priority_queue 为参照验证四种堆的最短路结果

## 编译程序

```sh
g++ -O3 -std=c++11 -Wall -Wextra -o code/dijkstra_bench.exe code/dijkstra_bench.cpp
g++ -O3 -std=c++11 -Wall -Wextra -o code/selftest.exe code/selftest.cpp
```

## 正确性自测

```sh
./code/selftest.exe
```

## 标准测试

```sh
./code/dijkstra_bench.exe
```

第一阶段测试 NY、BAY、COL 的 d/t 六图，输出 d/t 平均耗时、t÷d 倍率及堆间对比；第二阶段独立测试 NY、BAY、COL、FLA、NW、NE、CAL 的 d 七图，输出各堆耗时和相对二叉堆倍率。前三张 d 图会重新测试。默认仅输出到控制台，不自动保存文件。

可选模式如下，不能组合使用：

| 参数 | 测试内容 |
| --- | --- |
| 不带参数 | 小图 d/t 六图，再测 d 七图 |
| `-mini` | 仅小图 d/t 六图 |
| `-full` | 十二个区域的 d/t 共二十四图 |
| `-custom 0` | NY-d，5000 条查询 |
| `-custom 1` | 三层高减键人工图，默认 1000 条查询 |

各模式固定比较四种堆，仅输出到控制台。保存完整输出可以使用 shell 重定向，例如：

```sh
./code/dijkstra_bench.exe > fulltest.res 2>&1
./code/dijkstra_bench.exe -custom 0 > customtest.res 2>&1
./code/dijkstra_bench.exe -custom 1 >> customtest.res 2>&1
```

输入路径由模式固定选择，不提供单图路径、堆筛选、CSV 导出、逐查询导出或额外完整单源测试参数。`-full` 运行前需下载并生成全部二十四图的查询。

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

编译命令见上文。若 bench 正在运行，请将编译输出写入另一个文件名，或待该轮结束再编译。

## 实现特点

- `Args` 只保存一个 Mode；Dataset 保存内部图路径、查询路径和标签，统一调度与统计输出。
- `HeapAccess` 特化适配堆接口，`DijkstraRunner<Heap>` 让四种堆共享搜索逻辑，无需运行时虚函数派发。
- CSR 连续存储出边；整文件缓冲读取和整数扫描，输入解析不计入搜索耗时。
- `touched` 记录所有已发现顶点，仅恢复本轮状态，包括目标早停时仍留在队列中的点；堆清理也复用容量。
- 二项堆和斐波那契堆使用 `deque` 节点池，扩容保持节点地址稳定，查询间复用分配空间。
- 点对查询在目标有效出堆时早停；惰性队列通过距离和确定标记过滤失效条目。
- 同时统计入堆、出堆、成功距离改善、扫描边、确定点和键值比较，辅助解释耗时差异。
- 固定随机种子，同一区域 d/t 共享点对；正确性测试使用独立 STL priority_queue Dijkstra，覆盖边界图、三十组随机图、全点对和连续查询状态恢复。
