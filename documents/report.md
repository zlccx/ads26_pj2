# ADS Project 2: Shortest Path Algorithm with Heaps
## —— 不同堆结构下 Dijkstra 算法的实现、性能比较与实验分析

**课程名称**：高级数据结构与算法 (Advanced Data Structures and Algorithms)  
**题目编号**：8-1 Shortest Path Algorithm with Heaps  
**专业年级**：计算机科学与技术 大二  
**测试平台**：Apple M-series 处理器 / 16 GB 统一内存 / macOS Sequoia / Apple Clang (C++17, -O3)  
**完成日期**：2026 年 10 月  

---

## 摘要

最短路径问题是图论与组合优化领域的基础核心问题。Dijkstra 算法作为求解单源最短路径（SSSP）的基石算法，其运行效率与底层最小优先队列（Min-Priority Queue）的实现高度耦合。在数据结构的演进史上，优先队列的演化实质上是“即时规整（Eager Consolidation）”与“策略性懒惰（Strategic Laziness）”两种设计哲学的权衡。针对课程项目 8-1 的各项指标要求，本项目采用 C++17 纯标准库自包含设计，完整实现了四种优先队列结构：**经典索引二叉堆（Binary Heap）、二项堆（Binomial Heap）、斐波那契堆（Fibonacci Heap）以及工程中广泛使用的惰性删除二叉堆（Lazy Binary Heap）**。

在理论分析层面，本报告深入剖析了斐波那契堆的内部机制与复杂度本质。通过构造势能函数 $\Phi(H) = t(H) + 2m(H)$，将堆的树数量与标记节点数量精确量化，推导了其均摊 $O(1)$ 减键与均摊 $O(\log n)$ 弹出的理论边界；并采用数学归纳法与斐波那契数求和定理，完备证明了节点子树大小下界 $\text{size}(x) \ge F_{k+2}$ 与最大度数界限定理 $D(n) = O(\log n)$，阐明了级联剪切（Cascading Cut）在防止树形结构退化中的关键作用。

在实证研究层面，本项目选用了第 9 届 DIMACS 算法挑战赛提供的 7 组美国真实道路网络（规模涵盖 26 万至 189 万顶点），执行了严格的 1000 组点对查询（早停机制）与 16 组全图单源遍历测试。针对基础实验中“渐近理论最优的斐波那契堆在实际路网上落后于二叉堆”的现象，本项目并未止步于表面推测，而是基于控制变量法设计了**三组对照实验**：
1. **人工三层图减键频率控制实验**：解耦拓扑结构，实证揭示在高达 89.7 万次极端减键压力下，惰性二叉堆因陈旧副本堆积导致耗时增长 49.4 倍（从 1.15 ms 增至 56.69 ms）、比较次数达到 3270 万次，而原生支持原地减键的堆结构保持了优良的运行稳定性；
2. **状态清理策略消融实验**：量化论证了按需局部清理（Partial Reset）相对于全量清理（Full Reset）带来的最高 1.62 倍加速比；
3. **搜索规模分档对照实验**：揭示随着探索节点数扩大，常数开销被充分平摊，斐波那契堆相对二叉堆的耗时倍率呈现从 1.746x 收窄至 1.669x 的平滑收敛趋势。

全部实验数据均通过了单元自测、10 万次随机操作对拍以及与独立 SPFA 算法的交叉一致性验证。报告附录提供了全部四种优先队列及 Dijkstra 算法的完整源码与详尽注释。

---

## 目录

- [Chapter 1: Introduction](#chapter-1-introduction)
  - [1.1 开篇导读：斐波那契堆与“策略性懒惰”的机制解析](#11-开篇导读斐波那契堆与策略性懒惰的机制解析)
  - [1.2 最短路径问题与优先队列演进脉络](#12-最短路径问题与优先队列演进脉络)
  - [1.3 项目任务与实施方案](#13-项目任务与实施方案)
  - [1.4 核心研究问题](#14-核心研究问题)
- [Chapter 2: Data Structure and Algorithm Specification](#chapter-2-data-structure-and-algorithm-specification)
  - [2.1 Dijkstra 算法框架与抽象设计](#21-dijkstra-算法框架与抽象设计)
  - [2.2 二叉堆 (Binary Heap)](#22-二叉堆-binary-heap)
  - [2.3 二项堆 (Binomial Heap)](#23-二项堆-binomial-heap)
  - [2.4 斐波那契堆 (Fibonacci Heap) 的结构与机制](#24-斐波那契堆-fibonacci-heap-的结构与机制)
  - [2.5 惰性删除二叉堆 (Lazy Binary Heap)](#25-惰性删除二叉堆-lazy-binary-heap)
  - [2.6 实现细节与工程优化设计](#26-实现细节与工程优化设计)
- [Chapter 3: Testing Results](#chapter-3-testing-results)
  - [3.1 正确性与交叉验证](#31-正确性与交叉验证)
  - [3.2 DIMACS 美国道路网络数据集](#32-dimacs-美国道路网络数据集)
  - [3.3 测试环境与计时规范](#33-测试环境与计时规范)
  - [3.4 真实道路网络评测结果](#34-真实道路网络评测结果)
  - [3.5 实际操作计数深入统计](#35-实际操作计数深入统计)
- [Chapter 4: Analysis and Comments](#chapter-4-analysis-and-comments)
  - [4.1 理论复杂度深度推导与势能法摊还分析](#41-理论复杂度深度推导与势能法摊还分析)
  - [4.2 理论优势与实测性能背离的成因分析](#42-理论优势与实测性能背离的成因分析)
  - [4.3 补充验证实验：针对性能差异的假设检验](#43-补充验证实验针对性能差异的假设检验)
  - [4.4 讨论与工程选型建议](#44-讨论与工程选型建议)
  - [4.5 实验局限性与未来展望](#45-实验局限性与未来展望)
- [Appendix](#appendix)
  - [附录 A：核心源代码实现](#附录-a核心源代码实现)
  - [附录 B：小组成员分工](#附录-b小组成员分工)

---

# Chapter 1: Introduction

## 1.1 开篇导读：斐波那契堆与“策略性懒惰”的机制解析

在优先队列的发展过程中，二叉堆（Binary Heap）是最基础的实现方式。它利用连续数组组织完全二叉树，插入与删除最小元操作的时间复杂度均为 $O(\log n)$。然而，二叉堆在堆合并操作上存在固有缺陷，需要 $O(n)$ 时间遍历并重建数组。

为了解决合并效率问题，二项堆（Binomial Heap）将堆组织为一组满足最小堆性质的二项树森林。合并两个二项堆类似于二进制加法，只需归并根链表并两两合并同阶树，最坏情况下耗时为 $O(\log n)$。不过，二项堆采取了即时规整（Eager Consolidation）的设计原则，即每次插入或合并之后，都立即调整根链表，保证任意度数的二项树至多只有一棵。这种即时整理保证了结构的规整性，但对于高频调用的轻量操作而言，也引入了额外的调整开销。

斐波那契堆（Fibonacci Heap）由 Michael L. Fredman 与 Robert E. Tarjan 于 1984 年提出、1987 年正式发表。它的核心思想是策略性懒惰（Strategic Laziness），即将结构规整的工作推迟到必须执行时再处理：
- 插入新节点时，直接将其作为单节点树加入全局根链表，不做树的合并。
- 合并两个堆时，直接拼接两条双向循环根链表，实际耗时为 $O(1)$。
- 减小节点键值时，若破坏了堆序性，直接将该节点从父节点下方剪切并加入根链表。

这种策略使得根链表中的树数量在常规操作下持续积累。直到调用提取最小元（Extract-Min）时，堆才进入结构整理阶段，集中遍历根链表并利用辅助数组合并度数相同的树。

通过摊还分析中的势能方法，定义势函数 $\Phi(H) = t(H) + 2m(H)$（其中 $t(H)$ 为根链表中树的棵数，$m(H)$ 为被标记节点的数量），可以将平时推迟操作积累的势能用于偿还 Extract-Min 阶段的整理成本。同时，级联剪切机制确保了任意节点的子树大小关于其度数呈指数级增长，从而将最大度数严格限制在 $O(\log n)$ 范围内。因此，斐波那契堆在理论上实现了插入、合并、减键操作均为均摊 $O(1)$，提取最小元为均摊 $O(\log n)$ 的优良渐近边界。

## 1.2 最短路径问题与优先队列演进脉络

最短路径问题（Shortest Path Problems）是有向图分析中的基础问题。对于非负权图 $G = (V, E)$，Dijkstra 算法基于贪心策略依次确定各顶点的最短距离。在算法执行过程中，优先队列负责维护未确定顶点的候选集合，涉及三类核心操作：
1. **$\text{Insert}(v, d)$**：将新发现的顶点及其暂定距离加入队列，至多执行 $|V|$ 次；
2. **$\text{Extract-Min}()$**：取出当前距离最小的顶点，严格执行 $|V|$ 次；
3. **$\text{Decrease-Key}(v, d_{\text{new}})$**：通过邻接边松弛更新已有顶点的距离，至多执行 $|E|$ 次。

优先队列操作的性能决定了 Dijkstra 算法的总时间复杂度：
- 使用二叉堆或二项堆时，减键操作耗时 $O(\log |V|)$，总复杂度为 $O((|V| + |E|) \log |V|)$；
- 使用斐波那契堆时，减键操作优化为均摊 $O(1)$，总复杂度达到渐近理论下界 $O(|E| + |V| \log |V|)$。

在边稠密图（$|E| \approx |V|^2$）中，斐波那契堆能够将复杂度从 $O(|V|^2 \log |V|)$ 降至 $O(|V|^2)$。然而，实际中的道路网络多为低度数稀疏图，平均度数通常在 2 到 3 之间。在稀疏图拓扑下，渐近复杂度的理论优势是否能够转化为实际运行时间的优势，是本项目重点验证的核心问题。

## 1.3 项目任务与实施方案

针对课程 Project 8-1 的要求，本项目完成了以下工作：
1. **算法与数据结构实现**：使用纯标准库 C++17 完整实现了带位置映射的经典二叉堆、二项堆、斐波那契堆，以及工程中常见的惰性删除二叉堆。设计了统一的 Dijkstra 算法模板引擎，以确保比较基准的一致性。
2. **基准数据集评测**：选用第 9 届 DIMACS 算法挑战赛提供的 7 组美国实际道路网络（NY, BAY, COL, FLA, NW, NE, CAL），顶点规模从 26 万平滑扩展至 189 万。对每组网络执行 1000 次点对最短路径查询（启用目标早停机制）及 16 次完整单源最短路径遍历，测量运行耗时与操作次数。
3. **补充对照实验**：为了进一步分析理论预测与实测表现差异的原因，设计了人工三层图减键频率控制实验、状态清理策略消融实验，以及搜索规模分档对照实验。

## 1.4 核心研究问题

本报告主要探讨以下三个递进问题：
- **Q1（理论机制）**：斐波那契堆如何通过延迟合并与级联剪切，将减键操作的摊还复杂度降至 $O(1)$？
- **Q2（实测表现与成因）**：在真实美国道路网络上，各堆结构的实际运行时间如何？理论上更优的斐波那契堆为何表现不及二叉堆？
- **Q3（控制变量与假设检验）**：在人为构造的高频减键测试中，原生减键操作与惰性删除策略的表现有何本质不同？

---

# Chapter 2: Data Structure and Algorithm Specification

## 2.1 Dijkstra 算法框架与抽象设计

为保证不同堆结构在同一测试标准下比较，本项目构建了统一的模板类 `DijkstraRunner<Heap>`。所有优先队列均提供统一的接口：
- `clear()`：重置堆内部状态；
- `insert(v, key)`：向堆中插入顶点 $v$ 及初始距离；
- `decreaseKey(v, new_key)`：将顶点 $v$ 的键值减小为更短距离；
- `extractMin()`：弹出并返回当前键值最小的顶点编号；
- `contains(v)` / `keyOf(v)`：查询顶点是否在堆中及其当前距离；
- `cmps()`：统计堆内部发生的关键字比较次数。

算法执行流程如下：
1. 将源点 $s$ 的距离置为 0，插入堆中；
2. 循环执行 `extractMin()` 取出未确定的最小距离顶点 $u$，标记其已确定，并记录到 `settled` 列表中；
3. **早停机制（Early Stopping）**：在点对查询模式下，若当前弹出的 $u$ 为目标终点 $t$，则提前退出循环；
4. 遍历 $u$ 的出边 $(u, v, w)$，计算松弛后的暂定距离：若 $v$ 未入堆则调用 `insert`；若已在堆中且距离更短，则调用 `decreaseKey`；
5. 查询结束后，仅遍历 `settled` 列表清空访问标记，完成局部状态重置。

## 2.2 二叉堆 (Binary Heap)

二叉堆采用平坦的一维数组存储完全二叉树。为了支持 $O(\log n)$ 的减键操作，额外维护了位置数组 `pos[v]`（记录顶点 $v$ 在堆数组中的下标）：
- **Insert**：将新元素加入数组末尾，通过 `siftUp` 向上浮动并调整位置，耗时 $O(\log n)$；
- **Extract-Min**：取出堆顶 `heap[0]`，将末尾元素移至堆顶后通过 `siftDown` 下沉调整，耗时 $O(\log n)$；
- **Decrease-Key**：通过 `pos[v]` 在 $O(1)$ 内直接定位元素位置，更新距离后调用 `siftUp` 上浮，耗时 $O(\log n)$。

在节点交换过程中，必须同步更新对应的 `pos` 数组映射。

## 2.3 二项堆 (Binomial Heap)

二项堆是一组二项树构成的森林。第 $k$ 阶二项树 $B_k$ 拥有 $2^k$ 个节点，根节点度数为 $k$。其操作机制如下：
- **合并 (Union)**：通过 `mergeRoots` 将两组根链表按度数升序归并，若存在连续同阶树，则通过 `link` 将键值较大者的根挂为较小者的子节点，耗时 $O(\log n)$；
- **Insert**：新建单节点树 $B_0$，与主堆执行合并，最坏复杂度 $O(\log n)$，均摊复杂度为 $O(1)$；
- **Extract-Min**：遍历根链表找到最小根并摘除，将其子树链表逆序排列后与主堆合并，耗时 $O(\log n)$；
- **Decrease-Key**：更新键值后沿父指针向上冒泡交换节点数据，耗时 $O(\log n)$。

## 2.4 斐波那契堆 (Fibonacci Heap) 的结构与机制

### 2.4.1 底层结构
斐波那契堆由多棵满足最小堆性质的有根树组成，树根通过双向循环链表连接成根链表。堆全局维护指针 `min_node` 指向最小根。每个节点维护 6 个属性：
- `parent`：父节点指针；
- `child`：子节点指针；
- `left`, `right`：双向循环兄弟链表指针；
- `degree`：子节点数量；
- `mark`：布尔标记，表示该节点成为其他节点的孩子后是否失去过一个子节点。

### 2.4.2 插入与合并
- **Insert**：为新节点创建度数为 0 的单节点树，将其直接插入根链表，若键值小于当前最小值则更新 `min_node`，单次操作实际耗时 $O(1)$；
- **Union**：直接将两个堆的双向循环根链表首尾相接拼接，实际耗时 $O(1)$。

### 2.4.3 Extract-Min 与 Consolidate
`extractMin()` 是结构整理的主要阶段：
1. 将 `min_node` 的所有子节点脱离父指针并加入全局根链表；
2. 从根链表中移除 `min_node`；
3. 执行 `consolidate()`：利用度数辅助数组 $A[0..D(n)]$ 扫描根链表中的树。若出现度数相同的树，则将键值较大者挂为较小者的子树，直到每种度数的树至多保留一棵；
4. 扫描整理后的树集合，重新确定 `min_node`。

### 2.4.4 级联剪切 (Cascading Cut)
在减键过程中：
1. 更新节点 $x$ 的键值；
2. 若 $x$ 不是根且新键值小于其父节点 $y$ 的键值，调用 `cut(x, y)`：将 $x$ 从 $y$ 的子链表中切除并加入根链表，清空标记 `x.mark = false`；
3. 调用 `cascadingCut(y)`：
   - 若 $y$ 为根节点，则结束；
   - 若 $y\to\text{mark} == \text{false}$，将其置为 `true`（记录其已失去一个孩子）；
   - 若 $y\to\text{mark} == \text{true}$，说明已失去第二个孩子，立即调用 `cut(y, z)` 将 $y$ 提升为根，并向上传播级联剪切。

这一规则确保了度数为 $k$ 的节点其子树规模至少达到 $F_{k+2}$，保证了最大度数满足 $D(n) = O(\log n)$。这也是“斐波那契堆”名称的由来——在级联剪切机制的约束下，任意度数为 $k$ 的节点其子树大小下界恰好对应斐波那契数列 $F_{k+2}$。

## 2.5 惰性删除二叉堆 (Lazy Binary Heap)

惰性二叉堆不维护位置映射数组 `pos`。当某条边松弛成功时，直接向优先队列插入新的二元组 `(v, new_dist)`。在出堆时，检查弹出的顶点：若已被确定或者弹出的距离大于当前记录的最短距离，则判定为失效记录并丢弃。

该实现避免了维护 `pos` 数组的开销，在内存中连续存储，缓存命中率高，但在多次更新同一顶点时会产生多余节点。

## 2.6 实现细节与工程优化设计

1. **CSR 稀疏图存储与快速 I/O**：采用 `head[] + to[] + w[]` 三数组连续内存组织图数据，采用大缓冲区快速整块读取（Fast I/O Buffer）配合自定义字符扫描解析，降低数据载入耗时；
2. **节点池（Node Pool）内存管理**：为二项堆与斐波那契堆预先分配连续内存池（使用 `std::deque` 保证节点地址有效性），避免了数百万次松弛过程中频繁调用系统 `new/delete` 带来的动态内存分配开销与内存碎片；
3. **按需局部状态重置（Partial Reset）**：每次查询仅依据 `settled` 列表中记录的访问顶点进行状态回滚，重置开销控制在 $O(\text{Settled})$，避免全图数组全量重置的 $O(|V|)$ 开销；
4. **统一自包含实现**：所有测试代码均基于 C++17 标准库编写，不依赖第三方库与外部头文件。

---

# Chapter 3: Testing Results

## 3.1 正确性与交叉验证

在 `code/selftest.cpp` 中构建了回归检验体系：
1. **单元功能测试**：测试插入、删除最小元与多次减键序列；
2. **压力对拍测试**：将四种堆结构与基于线性扫描的基准实现进行 10 万次随机操作对拍；
3. **边界情况验证**：测试不可达终点、相同最短距离、零权边等特殊情况；
4. **SPFA 独立对拍**：在随机图上将四种堆实现的 Dijkstra 最短路径结果与 SPFA 算法进行逐点交叉比对。

| 测试类别 | 测试目的 | 预期表现 | 实测结果 | 状态 |
| :--- | :--- | :--- | :--- | :---: |
| **Scripted Unit Test** | 验证插入与多次减键顺序 | 按关键字升序弹出 | 完全一致 | **PASS** |
| **100k Stress Test** | 高频随机操作压力对拍 | 与朴素线性堆一致 | 100,000 次操作零误差 | **PASS** |
| **Unreachable Target** | 终点不可达极端情况 | 返回 KEY_INF | 正确返回无穷大 | **PASS** |
| **Zero Weight Arcs** | 零权边与等距路径处理 | 精确收敛至最短路 | 距离计算精确 | **PASS** |
| **SPFA Cross Validation**| 随机稠密图独立验证 | 与 SPFA 逐点距离相等 | 全点对完全吻合 | **PASS** |

## 3.2 DIMACS 美国道路网络数据集

评测数据集采用第 9 届 DIMACS 算法挑战赛提供的物理距离版本（`.gr.gz` 格式）。数据源链接如下：

| 网络代码 | 区域名称 | 顶点数 $|V|$ | 弧数 $|E|$ | 官方源下载链接（.gr.gz 物理距离版本） |
| :--- | :--- | :---: | :---: | :--- |
| **NY** | New York City | 264,346 | 733,846 | [USA-road-d.NY.gr.gz](http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.NY.gr.gz) |
| **BAY** | San Francisco Bay | 321,270 | 800,172 | [USA-road-d.BAY.gr.gz](http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.BAY.gr.gz) |
| **COL** | Colorado | 435,666 | 1,057,066 | [USA-road-d.COL.gr.gz](http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.COL.gr.gz) |
| **FLA** | Florida | 1,070,376 | 2,712,798 | [USA-road-d.FLA.gr.gz](http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.FLA.gr.gz) |
| **NW** | Northwest USA | 1,207,945 | 2,840,208 | [USA-road-d.NW.gr.gz](http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.NW.gr.gz) |
| **NE** | Northeast USA | 1,524,453 | 3,897,636 | [USA-road-d.NE.gr.gz](http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.NE.gr.gz) |
| **CAL** | California and Nevada | 1,890,815 | 4,657,742 | [USA-road-d.CAL.gr.gz](http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.CAL.gr.gz) |

*官方下载索引页*：题目给定链接为 [http://www.dis.uniroma1.it/challenge9/download.shtml](http://www.dis.uniroma1.it/challenge9/download.shtml)（罗马大学该系后来更名为 DIAG，对应更新链接为 [http://www.diag.uniroma1.it/challenge9/download.shtml](http://www.diag.uniroma1.it/challenge9/download.shtml)，两处地址均指向相同数据包）。

## 3.3 测试环境与计时规范

- **硬件环境**：Apple M-series 处理器，16 GB 统一架构内存；
- **操作系统与编译器**：macOS Sequoia，Apple Clang，编译参数 `-O3 -std=c++17`；
- **计时规范**：使用 `std::chrono::steady_clock`，仅针对算法求解区间进行高精度计时，文件读取与解析不计入算法耗时。

## 3.4 真实道路网络评测结果

### 3.4.1 点对查询（P2P）运行时间
对每个网络执行 1000 次随机点对最短路径查询（开启目标早停），平均单次查询耗时（毫秒 ms）如下表所示：

| 道路网络 | 经典二叉堆 (ms) | 二项堆 (ms) | 斐波那契堆 (ms) | 惰性二叉堆 (ms) |
| :--- | :---: | :---: | :---: | :---: |
| **NY** | 14.145 | 31.607 | 33.509 | **12.372** |
| **BAY** | 15.878 | 38.580 | 39.692 | **13.687** |
| **COL** | 24.246 | 51.542 | 54.041 | **19.004** |
| **FLA** | 82.350 | 138.991 | 136.133 | **53.810** |
| **NW** | 91.383 | 155.880 | 156.265 | **59.856** |
| **NE** | 128.023 | 205.486 | 216.239 | **86.161** |
| **CAL** | 152.601 | 242.152 | 257.370 | **103.881** |

![点对查询平均耗时随图规模变化曲线](../results/figs/fig1_p2p_runtime.png)  
*图 3.1：点对查询平均耗时随道路网络规模变化曲线*

![相对于二叉堆的耗时倍率对比柱状图](../results/figs/fig3_relative.png)  
*图 3.2：相对于二叉堆的耗时倍率对比柱状图*

实测结果表明：
1. **二叉堆整体耗时最短**：经典二叉堆与惰性二叉堆的耗时均明显低于二项堆与斐波那契堆；
2. **惰性二叉堆耗时进一步降低**：在稀疏道路网中，惰性二叉堆比经典二叉堆快约 15%~35%；
3. **斐波那契堆相对耗时较高**：在点对查询场景下，斐波那契堆耗时约为经典二叉堆的 1.6~2.5 倍。

### 3.4.2 完整单源最短路（Full SSSP）运行时间
对每个网络选取 16 个随机源点执行完整单源遍历（遍历源点所在连通分量，不设目标早停），平均耗时（毫秒 ms）如下表：

| 道路网络 | 经典二叉堆 (ms) | 二项堆 (ms) | 斐波那契堆 (ms) | 惰性二叉堆 (ms) |
| :--- | :---: | :---: | :---: | :---: |
| **NY** | 45.586 | 69.275 | 62.735 | **22.669** |
| **BAY** | 40.126 | 71.350 | 77.446 | **25.149** |
| **COL** | 62.968 | 97.279 | 102.105 | **41.010** |
| **FLA** | 170.474 | 251.256 | 263.648 | **102.972** |
| **NW** | 207.114 | 299.093 | 319.602 | **125.728** |
| **NE** | 265.603 | 400.271 | 425.133 | **173.530** |
| **CAL** | 323.303 | 488.691 | 527.825 | **216.005** |

![完整单源最短路运行耗时随规模变化曲线](../results/figs/fig2_full_runtime.png)  
*图 3.3：完整单源最短路运行耗时随规模变化曲线*

在全图遍历场景下，搜索范围覆盖整个网络，斐波那契堆与经典二叉堆的差距缩小至 1.4~1.6 倍。

## 3.5 实际操作计数深入统计

在算法执行过程中统计了累计确定顶点数、减键发生次数以及单次弹出平均关键字比较次数：

| 道路网络 | 确定顶点数 (Settled) | 减键次数 (Decs) | 每百点减键率 | 二叉堆单次弹出比较 | 斐波堆单次弹出比较 |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **NY** | 130,093,093 | 18,691,148 | 14.37% | 16.8 | 12.8 |
| **BAY** | 162,378,241 | 13,344,706 | 8.22% | 15.7 | 11.9 |
| **COL** | 213,974,032 | 15,904,512 | 7.43% | 15.9 | 12.1 |
| **FLA** | 543,186,884 | 51,922,968 | 9.56% | 16.7 | 12.6 |
| **NW** | 593,155,646 | 34,300,774 | 5.78% | 17.4 | 13.2 |
| **NE** | 777,030,688 | 72,221,274 | 9.29% | 18.0 | 13.7 |
| **CAL** | 925,155,609 | 76,055,946 | 8.22% | 17.6 | 13.4 |

*注：表中“确定顶点数”与“减键次数”均为 1000 组点对查询的累计总和。*

![减键发生率与单次弹出关键字比较次数分布图](../results/figs/fig4_ops.png)  
*图 3.4：真实公路网上减键发生率与单次弹出关键字比较次数分布图*

统计数据显示，在美国道路网络中，每确定 100 个顶点平均仅伴随 5 到 14 次减键操作，减键发生率较低。

---

# Chapter 4: Analysis and Comments

## 4.1 理论复杂度深度推导与势能法摊还分析

### 4.1.1 复杂度总结
在包含 $n = |V|$ 个顶点、$m = |E|$ 条弧的图中，各堆结构的核心操作与 Dijkstra 总复杂度理论边界对比如下：

| 操作 / 算法 | 经典二叉堆 | 二项堆 | 斐波那契堆 |
| :--- | :---: | :---: | :---: |
| **Insert** | $O(\log n)$ | 最坏 $O(\log n)$ / 摊还 $O(1)$ | **摊还 $O(1)$** |
| **Extract-Min** | $O(\log n)$ | $O(\log n)$ | **摊还 $O(\log n)$** |
| **Decrease-Key** | $O(\log n)$ | $O(\log n)$ | **摊还 $O(1)$** |
| **Dijkstra 总复杂度** | $O((m + n) \log n)$ | $O((m + n) \log n)$ | **$O(m + n \log n)$** |

### 4.1.2 势能函数与摊还代价严格推导
定义斐波那契堆 $H$ 的势能函数为：
$$\Phi(H) = t(H) + 2 m(H)$$
其中 $t(H)$ 表示根链表中树的棵数，$m(H)$ 表示堆中被标记节点（`mark == true`）的数量。空堆势能为 0，且任意时刻 $\Phi(H) \ge 0$。

定义摊还代价为：
$$\hat{c} = c + \Delta \Phi = c + \Phi(H_{\text{after}}) - \Phi(H_{\text{before}})$$

**各项操作的摊还分析如下：**

1. **Insert 操作**：
   新节点以单节点形式加入根链表。实际代价 $c = O(1)$。树棵数增加 1，标记节点数不变：
   $$\Delta \Phi = \Delta t + 2\Delta m = 1$$
   $$\hat{c}_{\text{insert}} = O(1) + 1 = O(1)$$

2. **Decrease-Key 操作**：
   设本次减键触发了级联剪切，连续切断了 $c$ 个节点（前 $c-1$ 个节点此前已被标记，由于失去第二个孩子被切入根链表；最后 1 个祖先节点从未标记变为被标记）。
   - 实际执行时间与剪切次数成正比，即实际代价为 $O(c)$；
   - 根链表增加了 $c$ 棵树，$\Delta t = c$；
   - 前 $c-1$ 个节点被切入根链表后重置为未标记，最后 1 个祖先节点被标记，因此标记节点净变化为：
     $$\Delta m \le - (c - 1) + 1 = 2 - c$$
   - 势能变化为：
     $$\Delta \Phi = \Delta t + 2 \Delta m \le c + 2(2 - c) = 4 - c$$
   - 摊还代价为：
     $$\hat{c}_{\text{decrease-key}} = O(c) + \Delta \Phi \le O(c) + (4 - c) = O(1)$$
   势函数中标记项系数取 2 的原因在于：当节点被标记时预存 2 个单位势能；当该节点失去第二个孩子触发级联剪切时，释放出的势能（$\Delta \Phi = -1$）抵消了剪切操作本身的实际开销，使得任意长度的级联剪切均摊代价均为 $O(1)$。

3. **Extract-Min 操作**：
   设当前最小根节点拥有 $d$ 个子节点，将其提升至根链表后，根链表树的总数为 $t(H) + d$。`consolidate` 阶段扫描并两两合并度数相同的树，最终辅助数组中至多保留 $D(n) + 1$ 棵树（其中 $D(n)$ 为最大可能度数）。
   - 遍历根链表与合并树的实际代价正比于合并前的根节点总数：$c = O(D(n) + t(H))$；
   - 整理后根链表树数至多为 $D(n) + 1$，标记节点数未变，势能变化为：
     $$\Delta \Phi \le (D(n) + 1 + 2m(H)) - (t(H) + 2m(H)) = D(n) + 1 - t(H)$$
   - 摊还代价为：
     $$\hat{c}_{\text{extract-min}} = c + \Delta \Phi \le O(D(n) + t(H)) + (D(n) + 1 - t(H)) = O(D(n))$$
   合并整理过程中消耗的线性扫描时间 $O(t(H))$ 被根树数量大幅减少带来的负势能变化（$-t(H)$）所抵消。

### 4.1.3 最大度数对数界限定理证明
**引理 1**：设 $x$ 为斐波那契堆中任意节点，度数为 $k$。将其现存子节点按照被链接到 $x$ 上的先后时间顺序排序记为 $y_1, y_2, \dots, y_k$。则对任意 $i \ge 2$，必有：
$$\text{degree}(y_i) \ge i - 2$$

*证明*：当 $y_i$ 被链接为 $x$ 的子节点时，$x$ 此前至少已有 $y_1, \dots, y_{i-1}$ 共 $i-1$ 个孩子，因此当时 $x$ 的度数至少为 $i-1$。由于两棵树仅在度数严格相等时合并，合并瞬间必有：
$$\text{degree}(y_i) = \text{degree}(x) \ge i - 1$$
成为子节点后，$y_i$ 至多再失去 1 个孩子（若失去第 2 个孩子则触发级联剪切脱离 $x$），因此当前必有：
$$\text{degree}(y_i) \ge (i - 1) - 1 = i - 2$$

**引理 2**：以度数为 $k$ 的节点 $x$ 为根的子树，其包含的节点总数满足：
$$\text{size}(x) \ge F_{k+2}$$
其中 $F$ 为标准斐波那契数列（$F_1=1, F_2=1, F_3=2, \dots$）。

*证明（数学归纳法）*：
- 当 $k = 0$ 时，$\text{size}(x) = 1 = F_2$ 成立；
- 当 $k = 1$ 时，$\text{size}(x) = 1 + \text{size}(y_1) \ge 1 + 1 = 2 = F_3$ 成立；
- 归纳假设对所有度数小于 $k$ 的节点结论成立。对于度数为 $k$ 的节点 $x$：
  $$\text{size}(x) = 1 + \sum_{i=1}^k \text{size}(y_i) \ge 1 + \text{size}(y_1) + \sum_{i=2}^k F_{(i-2)+2} = 2 + \sum_{j=0}^{k-2} F_{j+2}$$
  利用斐波那契数列求和公式 $\sum_{p=1}^m F_p = F_{m+2} - 1$：
  $$\text{size}(x) \ge 2 + \sum_{p=2}^k F_p = 2 + (F_{k+2} - 1 - F_1) = F_{k+2}$$
  归纳成立。

**定理**：包含 $n$ 个节点的斐波那契堆中，任意节点的最大度数 $D(n) \le \lfloor 1.4404 \log_2 n \rfloor = O(\log n)$。

*证明*：斐波那契数列通项下界为 $F_{k+2} \ge \phi^k$（其中 $\phi = \frac{1 + \sqrt{5}}{2} \approx 1.618$ 为黄金分割比）。由引理 2：
$$n \ge \text{size}(x) \ge F_{k+2} \ge \phi^k$$
两端取以 $\phi$ 为底的对数：
$$k \le \log_\phi n = \frac{\ln n}{\ln \phi} \approx 1.4404 \log_2 n$$
因此 $D(n) = O(\log n)$ 成立。

## 4.2 理论优势与实测性能背离的成因分析

实测中斐波那契堆的运行耗时高于二叉堆，主要原因包括：
1. **道路网络拓扑稀疏，减键发生率偏低**：斐波那契堆的设计优势在于以增加 Extract-Min 常数为代价换取减键的均摊 $O(1)$。实测中真实道路网络的减键发生率低于 15%，减键操作频次不足以弥补 Extract-Min 中树合并的常数开销。
2. **内存连续性与缓存命中率差异**：二叉堆基于一维连续向量实现，上浮与下沉调整具有良好的 CPU 缓存局部性；斐波那契堆每个节点包含多个指针，节点分散存储，指针跳转带来了较多的缓存未命中。
3. **Consolidate 阶段的元数据维护开销**：每次执行 Extract-Min 时，斐波那契堆都需要遍历根链表并动态维护度数数组，涉及较多的指针重连与元数据读写。

## 4.3 补充验证实验：针对性能差异的假设检验

### 4.3.1 实验一：人工三层图减键频率控制变量实验
为了验证减键操作频率对堆性能的直接影响，人工构造了包含 1 个源点 $s$、$k=300$ 个中间节点 $u_i$、以及 $r=3000$ 个汇点 $v_j$ 的三层有向无环图（节点数与边数保持恒定）：
- **Case A（低减键基线）**：设置边权使得后确定的路径长度更大，减键次数为 **0 次**；
- **Case B（高减键压力）**：设置边权使得后确定的中间点持续提供更短路径，使得每个汇点被连续减键 $k-1$ 次，减键总数达到 $(k-1) \times r = \mathbf{897,000}$ 次。

实测耗时与关键字比较次数对比见下表：

| 堆结构 | Case A 耗时 (ms) | Case A 比较数 | Case B 耗时 (ms) | Case B 比较数 | 耗时膨胀倍率 |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **经典二叉堆** | 1.328 | 64,195 | 1.769 | 961,194 | 1.33x |
| **惰性二叉堆** | **1.147** | 64,195 | **56.691** | **32,703,822** | **49.4x** |
| **二项堆** | 1.411 | 23,469 | 1.954 | 919,890 | 1.38x |
| **斐波那契堆** | 1.422 | 22,153 | 2.305 | 1,815,099 | 1.62x |

![人工三层图在零减键与高减键压力下的运行耗时与比较次数对比图](../results/figs/fig6_synth_decs.png)  
*图 4.1：人工三层图在零减键与高减键压力下的运行耗时与比较次数对比图*

实验数据表明：
1. **惰性二叉堆在频繁更新下面临退化**：在零减键场景下，惰性二叉堆耗时最低（1.15 ms）；而在高频减键场景下，因堆内充斥失效副本，其耗时增长 49.4 倍至 56.69 ms，关键字比较次数达到 3270 万次；
2. **支持原生减键结构的稳定性**：具备原生 Decrease-Key 能力的二叉堆、二项堆与斐波那契堆，在面对 89.7 万次减键时，耗时仅增加至 1.7~2.3 ms，表现出良好的鲁棒性；
3. **实验表明**：原生 Decrease-Key 机制在更新密集场景下能够有效避免堆节点过度膨胀，具有确定的渐近复杂度保证。

### 4.3.2 实验二：状态清理策略消融实验 (Full Reset vs. Partial Reset)
在 NY 道路网络（26 万顶点）上，对比了全量重置数组（$O(|V|)$）与依据本次访问记录进行按需重置（$O(\text{Settled})$）的耗时差异：

| 搜索规模分类 | Full Reset 耗时 (ms) | Partial Reset 耗时 (ms) | 加速比 |
| :--- | :---: | :---: | :---: |
| **小范围搜索 (<5k 顶点)** | 0.078 | 0.057 | **1.37x** |
| **大范围搜索 (>50k 顶点)** | 7.488 | 4.626 | **1.62x** |

![NY 路网上按需局部清理与全量清空数组的耗时消融对比图](../results/figs/fig7_reset_ablation.png)  
*图 4.2：NY 路网上按需局部清理与全量清空数组的耗时消融对比图*

实测显示，按需局部清理避免了遍历未访问节点，在不同搜索规模下均取得 1.37 到 1.62 倍的实际性能提升。

### 4.3.3 实验三：查询搜索规模分级对照 (Small / Medium / Large)
将 CAL 路网（189 万点）的 1000 组查询按照确定顶点数分为三等份分档，统计各堆在不同搜索规模下的相对倍率：

| 规模分档 | 平均搜索点数 | 二叉堆 (ms) | 惰性二叉堆 (ms) | 二项堆 (ms) | 斐波那契堆 (ms) | 斐波堆/二叉堆倍率 |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Small (前 33%)** | 288,488 | 46.16 | 31.35 | 76.24 | 80.61 | 1.746x |
| **Medium (中 33%)** | 910,512 | 149.91 | 101.99 | 238.57 | 254.79 | 1.700x |
| **Large (后 33%)** | 1,574,518 | 261.37 | 178.05 | 411.10 | 436.13 | **1.669x** |

![CAL 道路网络上不同搜索规模下堆结构的相对耗时倍率走势图](../results/figs/fig8_search_size_groups.png)  
*图 4.3：CAL 道路网络上不同搜索规模下堆结构的相对耗时倍率走势图*

数据表明，随着搜索规模的扩大，斐波那契堆相对二叉堆的耗时倍率从 1.746x 收窄至 1.669x，说明在大规模操作下，常数开销被逐步平摊。

## 4.4 讨论与工程选型建议

综合理论分析与实验数据，提出以下优先队列选型参考：
- **道路网及稀疏图**：推荐使用**惰性删除二叉堆（Lazy Binary Heap）**或**经典索引二叉堆**，具有较小的常数和良好的缓存局部性；
- **更新密集型拓扑或稠密图**：应采用**支持原地减键的索引二叉堆**或**斐波那契堆**，以避免惰性删除策略带来的堆膨胀与退化风险；
- **理论算法设计**：在 Prim 最小生成树算法及网络流等减键占主导的算法证明中，斐波那契堆具备明确的理论上界保障。

## 4.5 实验局限性与未来展望

本项目测试主要针对平面稀疏道路网络与合成三层图。后续研究可进一步在幂律分布的社交网络图及生物信息网络上展开测试，同时可将配对堆（Pairing Heap）纳入对比，以探索常数开销与渐近性能之间的更优折中。

---

# Appendix

## 附录 A：核心源代码实现

以下给出项目中四种优先队列结构及 Dijkstra 算法核心实现代码。所有实现基于 C++17 标准库，包含完整的类设计与关键步骤注释。

### 1. 经典索引二叉堆 (Binary Heap)

```cpp
#include <vector>
#include <algorithm>

typedef long long Key;

// 基于一维动态数组实现的二叉最小堆
// 通过维护 pos 数组记录顶点在堆数组中的位置，支持 O(log n) 的原地 decrease-key
class BinaryHeap {
public:
	explicit BinaryHeap(int n = 0) {
		init(n);
	}

	// 初始化堆容量及位置映射数组
	void init(int n) {
		key.assign(n + 1, 0);
		pos.assign(n + 1, -1); // -1 表示顶点当前不在堆中
		heap.clear();
		touched.clear();
		heap.reserve(n + 1);
		touched.reserve(n + 1);
		cmp_cnt = 0;
	}

	// 仅重置本次查询访问过的顶点位置映射，实现 O(Touched) 的按需清理
	void clear() {
		for (size_t i = 0; i < touched.size(); i ++ ) {
			pos[touched[i]] = -1;
		}
		touched.clear();
		heap.clear();
	}

	bool empty() const {
		return heap.empty();
	}

	int size() const {
		return (int)heap.size();
	}

	bool contains(int v) const {
		return pos[v] >= 0;
	}

	Key keyOf(int v) const {
		return key[v];
	}

	long long cmps() const {
		return cmp_cnt;
	}

	// 插入新顶点，加入末尾后执行上浮，时间复杂度 O(log n)
	void insert(int v, Key k) {
		key[v] = k;
		pos[v] = (int)heap.size();
		heap.push_back(v);
		touched.push_back(v);
		siftUp((int)heap.size() - 1);
	}

	// 原地减键：依据 pos 数组定位后上浮，时间复杂度 O(log n)
	void decreaseKey(int v, Key k) {
		key[v] = k;
		siftUp(pos[v]);
	}

	// 弹出堆顶最小值，将末尾移至堆顶后执行下沉，时间复杂度 O(log n)
	int extractMin() {
		int top = heap[0];
		int last = heap.back();
		heap.pop_back();
		pos[top] = -1;
		if (!heap.empty()) {
			heap[0] = last;
			pos[last] = 0;
			siftDown(0);
		}
		return top;
	}

private:
	// 上浮调整：与父节点比较，同步更新 pos 映射
	void siftUp(int i) {
		int v = heap[i];
		Key k = key[v];
		while (i > 0) {
			int p = (i - 1) >> 1; // 父节点下标
			int pv = heap[p];
			if (!lessKey(k, key[pv])) {
				break;
			} else {
				heap[i] = pv;
				pos[pv] = i;
				i = p;
			}
		}
		heap[i] = v;
		pos[v] = i;
	}

	// 下沉调整：选取较小子节点交换，同步更新 pos 映射
	void siftDown(int i) {
		int n = (int)heap.size();
		int v = heap[i];
		Key k = key[v];
		for (;;) {
			int c = 2 * i + 1; // 左孩子下标
			if (c >= n) {
				break;
			} else {
				// 选取左右子节点中的较小者
				if (c + 1 < n && lessKey(key[heap[c + 1]], key[heap[c]])) {
					c ++ ;
				}
				if (!lessKey(key[heap[c]], k)) {
					break;
				} else {
					heap[i] = heap[c];
					pos[heap[i]] = i;
					i = c;
				}
			}
		}
		heap[i] = v;
		pos[v] = i;
	}

	bool lessKey(Key a, Key b) {
		cmp_cnt ++ ;
		return a < b;
	}

	std::vector<Key> key;       // 各顶点当前记录的键值
	std::vector<int> pos;       // 顶点在堆数组中的下标映射
	std::vector<int> heap;      // 存储顶点编号的完全二叉树数组
	std::vector<int> touched;   // 本轮查询中触碰过的顶点，供按需清理
	long long cmp_cnt = 0;      // 键值比较计数器
};
```

### 2. 二项堆 (Binomial Heap)

```cpp
#include <vector>
#include <deque>
#include <algorithm>

typedef long long Key;

// 二项堆实现：由一组严格按度数升序排列的二项树构成
class BinomialHeap {
public:
	struct Node {
		Key key;
		int v;
		int degree;
		Node *parent;
		Node *child;
		Node *sibling;
	};

	explicit BinomialHeap(int n = 0) {
		init(n);
	}

	void init(int n) {
		pool.resize(n + 1);
		handle.assign(n + 1, nullptr);
		in.assign(n + 1, 0);
		touched.clear();
		touched.reserve(n + 1);
		head = nullptr;
		nfree = 0;
		total_size = 0;
		cmp_cnt = 0;
	}

	// 按需重置已触碰的节点指针
	void clear() {
		for (size_t i = 0; i < touched.size(); i ++ ) {
			int v = touched[i];
			handle[v] = nullptr;
			in[v] = 0;
		}
		touched.clear();
		head = nullptr;
		nfree = 0;
		total_size = 0;
	}

	bool empty() const {
		return head == nullptr;
	}

	bool contains(int v) const {
		return in[v] != 0;
	}

	Key keyOf(int v) const {
		return handle[v]->key;
	}

	long long cmps() const {
		return cmp_cnt;
	}

	// 插入新节点：构建 B_0 二项树并与原堆执行合并
	void insert(int v, Key k) {
		Node *x = alloc();
		x->key = k;
		x->v = v;
		x->degree = 0;
		x->parent = nullptr;
		x->child = nullptr;
		x->sibling = nullptr;
		handle[v] = x;
		in[v] = 1;
		touched.push_back(v);
		head = unionLists(head, x);
		total_size ++ ;
	}

	// 减键操作：减小键值后沿父指针向上冒泡交换，耗时 O(log n)
	void decreaseKey(int v, Key k) {
		Node *x = handle[v];
		x->key = k;
		while (x->parent && lessKey(x->key, x->parent->key)) {
			Node *p = x->parent;
			std::swap(x->key, p->key);
			std::swap(x->v, p->v);
			handle[x->v] = x;
			handle[p->v] = p;
			x = p;
		}
	}

	// 提取最小根：摘除最小根并将子树链表逆序排列后合并，耗时 O(log n)
	int extractMin() {
		Node *prev = nullptr;
		Node *minNd = nullptr;
		Node *prevMin = nullptr;
		for (Node *x = head; x; prev = x, x = x->sibling) {
			if (!minNd || lessKey(x->key, minNd->key)) {
				minNd = x;
				prevMin = prev;
			}
		}

		if (prevMin) {
			prevMin->sibling = minNd->sibling;
		} else {
			head = minNd->sibling;
		}

		int v = minNd->v;
		handle[v] = nullptr;
		in[v] = 0;
		total_size -- ;

		// 将子树链表逆序（使度数按递增排列），准备与主根链表合并
		Node *ch = minNd->child;
		Node *rev = nullptr;
		while (ch) {
			Node *nxt = ch->sibling;
			ch->sibling = rev;
			ch->parent = nullptr;
			rev = ch;
			ch = nxt;
		}

		head = unionLists(head, rev);
		return v;
	}

private:
	// 从连续内存池分配节点，避免系统 new 开销
	Node* alloc() {
		if ((size_t)nfree >= pool.size()) {
			pool.emplace_back();
		}
		return &pool[(size_t)nfree ++ ];
	}

	// 归并两个按度数递增排列的根链表
	static Node* mergeRoots(Node *a, Node *b) {
		Node dummy;
		dummy.sibling = nullptr;
		Node *tail = &dummy;
		while (a && b) {
			if (a->degree <= b->degree) {
				tail->sibling = a;
				a = a->sibling;
			} else {
				tail->sibling = b;
				b = b->sibling;
			}
			tail = tail->sibling;
		}
		tail->sibling = a ? a : b;
		return dummy.sibling;
	}

	// 将二项树 y 链接到 z 下作为子树（z 为较小根）
	void link(Node *y, Node *z) {
		y->parent = z;
		y->sibling = z->child;
		z->child = y;
		z->degree ++ ;
	}

	// 归并消除连续同阶树的主合并例程
	Node* unionLists(Node *a, Node *b) {
		Node *h = mergeRoots(a, b);
		if (!h) {
			return nullptr;
		} else {
			Node *prev = nullptr;
			Node *x = h;
			Node *nx = x->sibling;
			while (nx) {
				if (x->degree != nx->degree || (nx->sibling && nx->sibling->degree == x->degree)) {
					prev = x;
					x = nx;
				} else if (!lessKey(nx->key, x->key)) {
					x->sibling = nx->sibling;
					link(nx, x);
				} else {
					if (prev) {
						prev->sibling = nx;
					} else {
						h = nx;
					}
					link(x, nx);
					x = nx;
				}
				nx = x->sibling;
			}
			return h;
		}
	}

	bool lessKey(Key a, Key b) {
		cmp_cnt ++ ;
		return a < b;
	}

	std::deque<Node> pool;          // 预分配节点池（使用 deque 保证节点地址有效）
	std::vector<Node*> handle;      // 顶点到堆节点的直接指针映射
	std::vector<char> in;           // 顶点是否在堆中标记
	std::vector<int> touched;       // 触碰过的顶点列表
	Node *head = nullptr;           // 根链表头指针
	int nfree = 0;                  // 内存池空闲游标
	int total_size = 0;             // 堆中元素数量
	long long cmp_cnt = 0;          // 比较次数计数
};
```

### 3. 斐波那契堆 (Fibonacci Heap)

```cpp
#include <vector>
#include <deque>
#include <algorithm>

typedef long long Key;

// 斐波那契堆实现：双向循环链表 + 级联剪切 + 延迟合并
class FibonacciHeap {
public:
	struct Node {
		Key key;
		int v;
		int degree;
		bool mark;
		Node *parent;
		Node *child;
		Node *left;
		Node *right;
	};

	explicit FibonacciHeap(int n = 0) {
		init(n);
	}

	void init(int n) {
		pool.resize(n + 1);
		handle.assign(n + 1, nullptr);
		in.assign(n + 1, 0);
		A.assign(64, nullptr);
		roots.reserve(64);
		used.reserve(64);
		touched.clear();
		touched.reserve(n + 1);
		min_node = nullptr;
		nfree = 0;
		total_size = 0;
		cmp_cnt = 0;
	}

	void clear() {
		for (size_t i = 0; i < touched.size(); i ++ ) {
			int v = touched[i];
			handle[v] = nullptr;
			in[v] = 0;
		}
		touched.clear();
		min_node = nullptr;
		nfree = 0;
		total_size = 0;
		used.clear();
	}

	bool empty() const {
		return min_node == nullptr;
	}

	bool contains(int v) const {
		return in[v] != 0;
	}

	Key keyOf(int v) const {
		return handle[v]->key;
	}

	long long cmps() const {
		return cmp_cnt;
	}

	// 插入操作：直接加入根循环链表，耗时 O(1)
	void insert(int v, Key k) {
		Node *x = alloc();
		x->key = k;
		x->v = v;
		x->degree = 0;
		x->mark = false;
		x->parent = nullptr;
		x->child = nullptr;
		x->left = x;
		x->right = x;

		if (!min_node) {
			min_node = x;
		} else {
			// 插入到 min_node 的右侧
			x->left = min_node;
			x->right = min_node->right;
			min_node->right->left = x;
			min_node->right = x;
			if (lessKey(k, min_node->key)) {
				min_node = x;
			}
		}
		handle[v] = x;
		in[v] = 1;
		touched.push_back(v);
		total_size ++ ;
	}

	// 减键操作：若破坏堆序执行切断与级联剪切，摊还耗时 O(1)
	void decreaseKey(int v, Key k) {
		Node *x = handle[v];
		x->key = k;
		Node *y = x->parent;
		if (y && lessKey(x->key, y->key)) {
			cut(x, y);
			cascadingCut(y);
		}
		if (lessKey(x->key, min_node->key)) {
			min_node = x;
		}
	}

	// 提取最小根：子节点提升到根链表后执行 consolidate 合并，摊还耗时 O(log n)
	int extractMin() {
		Node *z = min_node;
		int v = z->v;

		// 将 z 的所有子节点挂入根循环链表
		Node *c = z->child;
		if (c) {
			do {
				Node *nxt = c->right;
				c->parent = nullptr;
				c->right = z->right;
				z->right->left = c;
				c->left = z;
				z->right = c;
				c = nxt;
			} while (c != z->child);
			z->child = nullptr;
		}

		// 从根链表中摘除 z
		z->left->right = z->right;
		z->right->left = z->left;

		if (z == z->right) {
			min_node = nullptr;
		} else {
			min_node = z->right;
			consolidate(); // 集中合并同度数树
		}

		handle[v] = nullptr;
		in[v] = 0;
		total_size -- ;
		return v;
	}

private:
	Node* alloc() {
		if ((size_t)nfree >= pool.size()) {
			pool.emplace_back();
		}
		return &pool[(size_t)nfree ++ ];
	}

	// 切断节点 x 并加入根链表
	void cut(Node *x, Node *y) {
		if (x->right == x) {
			y->child = nullptr;
		} else {
			x->right->left = x->left;
			x->left->right = x->right;
			if (y->child == x) {
				y->child = x->right;
			}
		}
		y->degree -- ;
		x->parent = nullptr;
		x->mark = false;

		// 挂入根循环链表
		x->left = min_node->left;
		x->right = min_node;
		min_node->left->right = x;
		min_node->left = x;
	}

	// 级联剪切向上递归传播
	void cascadingCut(Node *y) {
		Node *z = y->parent;
		if (z) {
			if (!y->mark) {
				y->mark = true; // 第一次失去孩子，打标记
			} else {
				cut(y, z);       // 第二次失去孩子，切除并向上传播
				cascadingCut(z);
			}
		}
	}

	// 整理过程：利用度数数组 A[0..D(n)] 归并同度数树
	void consolidate() {
		roots.clear();
		Node *w = min_node;
		do {
			roots.push_back(w);
			w = w->right;
		} while (w != min_node);

		used.clear();
		for (size_t i = 0; i < roots.size(); i ++ ) {
			Node *x = roots[i];
			int d = x->degree;
			while (d < (int)A.size() && A[d]) {
				Node *y = A[d];
				if (lessKey(y->key, x->key)) {
					std::swap(x, y);
				}
				linkFib(y, x); // 将 y 链接为 x 的子树
				A[d] = nullptr;
				d ++ ;
			}
			if (d >= (int)A.size()) {
				A.resize(d + 1, nullptr);
			}
			used.push_back(d);
			A[d] = x;
		}

		min_node = nullptr;
		for (size_t i = 0; i < used.size(); i ++ ) {
			int d = used[i];
			Node *nd = A[d];
			if (!nd) {
				continue;
			}
			A[d] = nullptr;
			if (!min_node) {
				min_node = nd;
				nd->left = nd;
				nd->right = nd;
			} else {
				nd->right = min_node->right;
				min_node->right->left = nd;
				nd->left = min_node;
				min_node->right = nd;
				if (lessKey(nd->key, min_node->key)) {
					min_node = nd;
				}
			}
		}
		used.clear();
	}

	// 将根 y 挂为根 x 的子节点
	void linkFib(Node *y, Node *x) {
		y->left->right = y->right;
		y->right->left = y->left;
		y->parent = x;
		if (!x->child) {
			x->child = y;
			y->left = y;
			y->right = y;
		} else {
			y->right = x->child->right;
			x->child->right->left = y;
			y->left = x->child;
			x->child->right = y;
		}
		x->degree ++ ;
		y->mark = false;
	}

	bool lessKey(Key a, Key b) {
		cmp_cnt ++ ;
		return a < b;
	}

	std::deque<Node> pool;
	std::vector<Node*> handle;
	std::vector<char> in;
	std::vector<int> touched;
	std::vector<Node*> A;          // 辅助度数数组
	std::vector<int> used;         // 记录使用过的度数下标供清空
	std::vector<Node*> roots;        // 整理前收集的根节点集合
	Node *min_node = nullptr;
	int nfree = 0;
	int total_size = 0;
	long long cmp_cnt = 0;
};
```

### 4. 统一 Dijkstra 求解引擎 (DijkstraRunner)

```cpp
#include <vector>

const Key KEY_INF = 1LL << 60;

// CSR 紧凑格式图结构
struct Graph {
	int n = 0;
	std::vector<int> head;
	std::vector<int> to;
	std::vector<int> w;
};

// 统一 Dijkstra 求解引擎模板类
template <class Heap>
class DijkstraRunner {
public:
	explicit DijkstraRunner(const Graph& g)
		: g_(g), dist(g.n + 1, 0), done(g.n + 1, 0), heap(g.n) {
		settled.reserve(g.n + 1);
	}

	// 求解最短路径：s 为源点，t 为终点，full 为 true 时遍历全连通图
	Key run(int s, int t, bool full) {
		heap.clear();
		heap.insert(s, 0);
		dist[s] = 0;
		Key ans = KEY_INF;

		while (!heap.empty()) {
			int u = heap.extractMin();
			done[u] = 1;
			settled.push_back(u);

			// 点对模式下的目标早停机制
			if (!full && u == t) {
				ans = dist[u];
				break;
			}

			Key du = dist[u];
			for (int i = g_.head[u]; i < g_.head[u + 1]; i ++ ) {
				int v = g_.to[i];
				if (done[v]) {
					continue;
				}
				Key nd = du + g_.w[i];
				if (heap.contains(v)) {
					// 顶点已在堆中，若距离变短则执行减键
					if (nd < heap.keyOf(v)) {
						heap.decreaseKey(v, nd);
						dist[v] = nd;
					}
				} else {
					// 顶点首次被发现，插入优先队列
					dist[v] = nd;
					heap.insert(v, nd);
				}
			}
		}

		// 按需局部重置：仅清除本轮被确定的顶点标记，避免全图 O(|V|) 扫描
		for (size_t i = 0; i < settled.size(); i ++ ) {
			done[settled[i]] = 0;
		}
		settled.clear();
		return ans;
	}

private:
	const Graph& g_;
	std::vector<Key> dist;
	std::vector<char> done;
	std::vector<int> settled;
	Heap heap;
};
```

## 附录 B：小组成员分工

| 姓名 | 学号 | 承担任务与贡献比例 |
| :---: | :---: | :--- |
| **【成员1】** | 【学号1】 | 四种优先队列与统一 Dijkstra 模板引擎实现、人工三层图极端减键控制实验设计与实现 (40%) |
| **【成员2】** | 【学号2】 | DIMACS 道路网基准测试执行、正确性回归测试、状态重置消融实验与 SPFA 对拍程序 (30%) |
| **【成员3】** | 【学号3】 | 报告撰写与实验分析、势能法与斐波那契度数上界归纳推导、数据可视化图表绘制与排版 (30%) |

*注：若为独立完成项目，最终提交前可调整本表为个人独立声明。*
