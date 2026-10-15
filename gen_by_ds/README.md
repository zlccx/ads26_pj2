# ADS Project 2 — Shortest Path Algorithm with Heaps

用 **二叉堆 / 二项堆 / 斐波那契堆** 三种最小优先队列实现 Dijkstra 最短路算法，
并在 [第 9 届 DIMACS 实现挑战赛（Challenge 9: Shortest Paths）](http://www.diag.uniroma1.it/challenge9/download.shtml)
的 USA 公路网基准上做大规模性能评测。

报告（中文，Typst）见 `report/report.pdf`，源码为 `report/report.typ`。

---

## 目录结构

```
ads26_pj2/
├─ src/
│  ├─ heaps.h        三种可索引最小优先队列（二叉堆 / 二项堆 / 斐波那契堆）+ 惰性二叉堆
│  ├─ dijkstra.h     Dijkstra 模板（可索引堆版 + 惰性删除版），含操作计数
│  ├─ dimacs.h       DIMACS .gr / .p2p 文件的两遍扫描快速解析 + CSR 构造
│  ├─ bench.cpp      性能测试程序
│  └─ selftest.cpp   正确性测试（手工用例 / 随机对拍 / SPFA 交叉验证）
├─ tools/
│  ├─ download.ps1       从 DIMACS 下载 USA 公路网数据
│  ├─ decompress.py      解压 .gr.gz
│  ├─ gen_queries.py     生成 1000 组随机点对查询
│  ├─ run_bench.py       并行跑各数据集的 bench
│  ├─ merge_results.py   合并 results/raw/*.csv -> results/results.csv
│  ├─ plot.py            生成报告用图
│  └─ make_report_tables.py  由 results.csv 生成 report/tables.typ
├─ data/                 图数据（不随仓库提交，见下方下载链接）
├─ results/              运行时间 CSV、逐查询明细、图
├─ report/               report.typ + report.pdf
└─ Makefile
```

## 编译

需要 g++（支持 C++17）。Windows 上用 MinGW：

```powershell
mingw32-make            # 或 mingw32-make all
mingw32-make test       # 编译并运行正确性测试
```

或直接调用编译器：

```powershell
g++ -O2 -std=c++17 -static -o build/selftest.exe src/selftest.cpp
g++ -O2 -std=c++17 -static -o build/bench.exe    src/bench.cpp
```

## 数据

数据来自 DIMACS Challenge 9，报告与本文档只列链接、不打包数据：

| 网络 | 结点数 | 弧数 | 下载链接 |
| --- | --- | --- | --- |
| NY  | 264,346   | 733,846   | http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.NY.gr.gz |
| BAY | 321,270   | 800,172   | http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.BAY.gr.gz |
| COL | 435,666   | 1,057,066 | http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.COL.gr.gz |
| FLA | 1,070,376 | 2,712,798 | http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.FLA.gr.gz |
| NW  | 1,207,945 | 2,840,208 | http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.NW.gr.gz |
| NE  | 1,524,453 | 3,897,636 | http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.NE.gr.gz |
| CAL | 1,890,815 | 4,657,742 | http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.CAL.gr.gz |
| 其余 | … | … | 见下载页（E / LKS / W / CTR / USA 等） |

完整的 12 个核心数据集及 `.co` 坐标文件、travel-time 版本都在上面的下载页里。

下载与预处理：

```powershell
powershell tools\download.ps1        # 下载 data/gz/*.gr.gz
python tools\decompress.py           # 解压成 data/*.gr
python tools\gen_queries.py          # 生成 data/queries/<NET>.p2p（每个 1000 组点对）
```

## 运行实验

单次运行（一种图、四种实现、点对 + 完整单源）：

```powershell
build\bench.exe --graph data\USA-road-d.NY.gr --net NY `
                --p2p data\queries\NY.p2p `
                --heaps bin,binom,fib,lazy --full-sources 16 `
                --out results\raw\NY.csv
```

批量跑所有已下载的数据集：

```powershell
python tools\run_bench.py -j 1 --full-sources 16    # -j 1 = 严格串行，计时最干净
python tools\verify_answers.py       # 逐查询交叉验证四种实现的答案
python tools\plot.py                 # 生成 results/figs/*
python tools\make_report_tables.py   # 生成 report/tables.typ
typst compile --root . report/report.typ report/report.pdf
```

一键复现（下载 → 实验 → 出图 → 编译报告）：`powershell tools\run_all.ps1`

> **计时注意**：`run_bench.py` 的 `-j` 控制并行度。并行跑多个数据集会争抢内存带宽，
> 使绝对耗时明显偏高（实测 `-j 4` 时单个数据集会慢 2–3 倍）。报告中的全部数字都是在
> `-j 1` 串行、且笔记本接交流电的状态下测得的，跨数据集的规模曲线才具有可比性。

`bench` 的参数：

| 参数 | 说明 |
| --- | --- |
| `--graph <file.gr>` | 图文件（DIMACS 格式，纯文本） |
| `--net NAME` | 结果里记录的数据集名字 |
| `--p2p <file>` / `--ss <file>` | 查询文件（`.p2p` 点对 / `.ss` 单源） |
| `--heaps bin,binom,fib,lazy` | 要测试的实现；`bin` 二叉堆、`binom` 二项堆、`fib` 斐波那契堆、`lazy` 惰性删除二叉堆 |
| `--full-sources K` | 额外跑 K 个随机源点的完整单源最短路 |
| `--out <csv>` | 追加写结果 CSV（自动写表头） |
| `--dump <prefix>` | 写出逐查询明细 `<prefix>.<heap>.csv` |
| `--seed S` | 生成完整单源随机源点用的种子 |

## 实验结果速览

7 个数据集（NY/BAY/COL/FLA/NW/NE/CAL，26 万 → 189 万结点），每个数据集
1000 组随机点对查询 + 16 个随机源点的完整单源最短路。

**平均单次点对查询耗时（ms）**

| 数据集 | 结点数 | 二叉堆 | 二项堆 | 斐波那契堆 | 二叉堆(惰性) |
| --- | ---: | ---: | ---: | ---: | ---: |
| NY  | 264,346   | 14.15 | 31.61 | 33.51 | 12.37 |
| BAY | 321,270   | 15.88 | 38.58 | 39.69 | 13.69 |
| COL | 435,666   | 24.25 | 51.54 | 54.04 | 19.00 |
| FLA | 1,070,376 | 82.35 | 138.99 | 136.13 | 53.81 |
| NW  | 1,207,945 | 91.38 | 155.88 | 156.27 | 59.86 |
| NE  | 1,524,453 | 128.02 | 205.49 | 216.24 | 86.16 |
| CAL | 1,890,815 | 152.60 | 242.15 | 257.37 | 103.88 |

* 三种可索引堆在同一批查询上完成的 `insert` / `decrease-key` / `extract-min` / 弧扫描次数
  **几乎逐位相同**（差异 ≤ 0.001%），因此耗时差异 100% 来自堆操作的常数因子。
* 实测排名：**二叉堆（惰性） < 二叉堆 < 二项堆 ≈ 斐波那契堆**，与
  $O(m + n log n) < O((n+m) log n)$ 的理论排名恰好相反。平均而言斐波那契堆是二叉堆的 1.98 倍，
  二项堆 1.91 倍，而惰性删除的二叉堆只要 0.74 倍。
* 每次 extract-min 的关键字比较次数：二叉堆 17.6 次，而二项堆 12.4 次、斐波那契堆 13.4 次
  ——**指针式堆少做 25% 以上的比较却更慢**，说明每次比较的访存代价（指针追逐 / cache miss）
  把渐进优势完全吃掉了。
* 原因：公路网稀疏（$m/n ≈ 2.5$），平均每 12 次 extract-min 才对应 1 次 decrease-key，
  斐波那契堆摊还 $O(1)$ 的 decrease-key 优势无从发挥。
* 完整单源（必须扫描整个连通分量）时差距缩小到 1.5–1.6 倍，与理论趋势一致。
* 全部 7000 组查询上四种实现返回的 $dist(t)$ **逐一相同**（`tools/verify_answers.py`）。

## 正确性

```powershell
build\selftest.exe
```

三层测试全部通过：固定序列手工用例、20 万次随机操作与「线性扫描」参考实现对拍、
以及随机图上与 SPFA（独立实现，不使用优先队列）的距离交叉验证；
`bench` 还对所有数据集校验「各实现距离之和」逐位一致。

## 计时说明

`bench` 只计时 Dijkstra 主循环（数据读入单独计时且不计入）。四种实现始终在**同一个进程**中
先后运行，使用**完全相同**的图、查询集与内存状态，因此它们之间的比值是可靠的。

跨数据集比较绝对耗时时，全部测量都采用了**串行 + 接交流电**的同一条件
（并行跑多个数据集会让绝对耗时虚高 2–3 倍；笔记本用电池时会因 CPU 降频慢 3 倍以上）。
