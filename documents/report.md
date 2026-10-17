# ADS Project 2: Shortest Path Algorithm with Heaps
## —— 不同堆结构下 Dijkstra 算法的实现、性能比较与实验分析

**课程名称**：高级数据结构与算法 (Advanced Data Structures and Algorithms)  
**题目编号**：8-1 Shortest Path Algorithm with Heaps  
**专业年级**：计算机科学与技术 大二  
**测试平台**：Apple M-series 处理器 / 16 GB 统一内存 / macOS Sequoia / Apple Clang (C++11/C++17, -O3)  
**完成日期**：2026 年 10 月  

---

## 摘要

最短路径问题是图论与组合优化领域的基础核心问题。Dijkstra 算法作为求解单源最短路径（SSSP）的基石算法，其运行效率与底层最小优先队列（Min-Priority Queue）的实现高度耦合。在数据结构的演进史上，优先队列的演化实质上是“即时规整（Eager Consolidation）”与“策略性懒惰（Strategic Laziness）”两种设计哲学的权衡。

本项目基于 C++11/C++17 标准库，构建了高度模块化且无外部依赖的工程体系（涵盖 `code/heaps.h`、`code/dijkstra.h`、`code/offline.h`），完整实现了四种优先队列结构：**经典索引二叉堆（Binary Heap）、二项堆（Binomial Heap）、斐波那契堆（Fibonacci Heap）以及工程中广泛使用的惰性删除二叉堆（Lazy Binary Heap）**。通过设计 `HeapAccess` 模板特化适配层与统一的 `DijkstraRunner<Heap>` 主循环，实现了四种堆在同一套核心算法逻辑下的零开销切换，彻底排除了代码副本差异对性能测试的干扰。

在理论分析层面，本报告深入剖析了斐波那契堆的内部机制与复杂度本质。通过构造势能函数 $\Phi(H) = t(H) + 2m(H)$，将堆的树数量与标记节点数量精确量化，推导了其均摊 $O(1)$ 减键与均摊 $O(\log n)$ 弹出的理论边界；并采用数学归纳法与斐波那契数求和定理，完备证明了节点子树大小下界 $\text{size}(x) \ge F_{k+2}$ 与最大度数界限定理 $D(n) = O(\log n)$，阐明了级联剪切（Cascading Cut）在防止树形结构退化中的关键作用。此外，针对批量查询场景，报告补充推导了普通点对早停与离线同源合并策略的理论复杂度边界。

在实证研究层面，本项目选用了第 9 届 DIMACS 算法挑战赛提供的 7 组美国真实道路网络（规模涵盖 26 万至 189 万顶点，覆盖物理距离与通行时间两种边权类型），执行了严格的 1000 组点对查询（目标早停机制）与 16 组全图单源遍历测试。针对基础实验中“渐近理论最优的斐波那契堆在实际路网上落后于二叉堆”的现象，本项目并未止步于表面推测，而是基于控制变量法设计了**三组对照实验**：
1. **Decrease-Key 频率控制实验**：解耦拓扑结构，实证揭示在高达 89.7 万次极端减键压力下，惰性二叉堆因陈旧失效副本堆积导致耗时增长 49.4 倍（从 1.15 ms 增至 56.69 ms）、比较次数达到 3270 万次，而原生支持原地减键的堆结构保持了优良的运行稳定性；
2. **状态清理策略消融实验**：量化论证了基于实际发现点（Touched）的按需局部清理相对于全量清空数组带来的最高 1.62 倍加速比；
3. **搜索规模分档实验**：揭示随着探索节点数扩大，常数开销被充分平摊，斐波那契堆相对二叉堆的耗时倍率呈现从 1.746x 收窄至 1.669x 的平滑收敛趋势。

项目在 `code/selftest.cpp` 中以 C++ STL 独立的 `std::priority_queue` 建立黄金基准，覆盖 30 组随机小图与边界极端用例，四种堆与离线优化策略均 100% 通过了自动化一致性检验。报告附录提供了全部四种优先队列、统一求解引擎及离线同源优化策略的完整源码与详尽注释。

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

针对课程 Project 8-1 的要求与工程规范，本项目完成了以下工作：
1. **模块化自包含代码架构**：项目采用纯 C++11/C++17 标准库构建，划分为职责分明的模块：
   - `code/heaps.h`：统一封装四大优先队列（二叉堆、二项堆、斐波那契堆、惰性二叉堆）；
   - `code/dijkstra.h`：实现 CSR 紧凑图存储、快速整块 I/O 读入、微观操作统计器 `DijkstraStats`、`HeapAccess` 特化适配层及统一主求解器 `DijkstraRunner`；
   - `code/offline.h`：设计实现了针对批量同源查询的离线排序与共享搜索优化算法 `solveOffline`；
   - `code/dijkstra_bench.cpp` 与 `code/optimize_bench.cpp`：分别作为标准点对早停与离线优化策略的性能测试入口；
   - `code/selftest.cpp`：自动化回归测试程序，以标准库 `std::priority_queue` 为对照验证四种自实现堆及离线策略的正确性。
2. **基准数据集评测**：选用第 9 届 DIMACS 算法挑战赛提供的 7 组美国实际道路网络（NY, BAY, COL, FLA, NW, NE, CAL），顶点规模从 26 万平滑扩展至 189 万，覆盖物理距离（`d`）与通行时间（`t`）两套边权。针对每张网络执行 1000 组随机点对最短路径查询（基于统一的固定随机种子生成，起终点不同，启用目标早停机制）及 16 组全图单源最短路径遍历，精确测量运行耗时与微观操作计数。
3. **补充对照实验**：为了进一步分析理论预测与实测表现差异的原因，设计了人工三层图减键频率控制实验、基于触碰顶点的状态清理策略消融实验、以及搜索规模分档对照实验。

## 1.4 核心研究问题

本报告主要探讨以下三个递进问题：
- **Q1（理论机制）**：斐波那契堆如何通过延迟合并与级联剪切，将减键操作的摊还复杂度降至 $O(1)$？批量查询下普通早停与离线同源策略的理论边界如何权衡？
- **Q2（实测表现与成因）**：在真实美国道路网络上，各堆结构的实际运行时间如何？理论上更优的斐波那契堆为何表现不及二叉堆？
- **Q3（控制变量与假设检验）**：在人为构造的高频减键测试中，原生减键操作与惰性删除策略的表现有何本质不同？

---

# Chapter 2: Data Structure and Algorithm Specification

## 2.1 Dijkstra 算法框架与抽象设计

为保证不同堆结构在同一测试标准下公平比较，消除算法框架代码分叉带来的系统性偏差，本项目设计了统一的模板类 `DijkstraRunner<Heap>` 与特化适配层 `HeapAccess<Heap>`。

### 2.1.1 堆访问适配器 (HeapAccess)
不同优先队列在减键操作上存在机制差异：二叉堆、二项堆和斐波那契堆支持原生原地更新，而惰性二叉堆采用重复压入新副本的策略。为此，`HeapAccess` 提供了编译期统一抽象：
- 常规堆特化：`HeapAccess<Heap>::insert` 映射至 `heap.insert`，`decrease` 映射至 `heap.decreaseKey`，出堆时直接读取距离；
- 惰性堆特化：`HeapAccess<LazyBinaryHeap>::decrease` 映射至 `heap.push`（新增队列条目），出堆时返回键值并设置 `lazy = true`。

在 `DijkstraRunner` 主循环中，统一执行出堆检验：
```cpp
Key k;
int u = HeapAccess<Heap>::pop(heap, k, dist);
if (done[u] || k != dist[u]) continue; // 惰性堆的过期条目与重复记录在此统一过滤
```
这一设计使得所有堆结构共享**完全相同的单一算法主循环**。

### 2.1.2 两种运行模式
1. **普通点对早停模式（P2P）**：
   在单次查询中，源点 $s$ 距离置零入堆。循环提取当前距离最小的顶点 $u$，一旦目标终点 $t$ 作为有效最小值从堆中弹出，算法立即记录答案并终止搜索，剪枝掉图中剩余的大量未访问节点。
2. **离线同源分组模式（Offline）**：
   对于批量查询输入，`solveOffline` 先将所有查询按起点编号进行排序和分组。对于同一起点的多个目标，仅执行**一次完整单源搜索**，在遍历过程中直接收集该组内所有目标的距离，最后按原始输入序号还原答案。

## 2.2 二叉堆 (Binary Heap)

二叉堆采用平坦的一维动态数组存储完全二叉树。为了支持 $O(\log n)$ 的减键操作，额外维护了位置数组 `pos[v]`（记录顶点 $v$ 在堆数组中的下标）：
- **Insert**：将新元素加入数组末尾，通过 `siftUp` 向上浮动并调整位置，耗时 $O(\log n)$；
- **Extract-Min**：取出堆顶 `heap[0]`，将末尾元素移至堆顶后通过 `siftDown` 下沉调整，耗时 $O(\log n)$；
- **Decrease-Key**：通过 `pos[v]` 在 $O(1)$ 内直接定位元素位置，更新距离后调用 `siftUp` 上浮，耗时 $O(\log n)$。

在节点交换过程中，必须同步更新对应的 `pos` 数组映射。堆清空时仅根据 `touched` 数组恢复使用过的 `pos` 值为 -1，保留底层数组容量供下一轮复用。

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

1. **CSR 稀疏图存储与快速 I/O（Fast I/O Buffer）**：
   采用 `head[] + to[] + w[]` 三数组连续内存组织图数据，出边紧密排布；利用 `fread` 将路网文件一次性全部载入内存缓冲区，配合双指针字符整型扫描函数 `scanInt` 进行无阻塞解析，大幅降低数据载入耗时。
2. **稳定节点池（Node Pool）内存管理**：
   二项堆与斐波那契堆基于指针和动态链表构建。为消除高频动态内存分配的开销与内存碎片，采用 `std::deque` 集中管理节点分配。`std::deque` 分段连续存储，扩容时已分配节点的内存地址永久保持稳定，确保堆内部复杂的父子与兄弟指针绝不失效。
3. **基于实际触碰点（Touched）的按需状态恢复（Partial Reset）**：
   在早停模式下，部分顶点虽然已被发现并入堆（`seen[v] = 1`），但尚未正式出堆确定（`done[v] == 0`）。因此，代码使用 `touched` 数组记录本轮实际触碰并入堆的所有顶点。查询结束后，仅遍历 `touched` 数组恢复 `done[v] = 0; seen[v] = 0;`，将状态清理开销严格限制在 $O(\text{Touched})$ 实际发现点数级别，避免全图数组全量 `memset` 的 $O(|V|)$ 冗余开销。
4. **离线同源查询聚类与共享搜索（Offline Query Grouping）**：
   针对批量查询中可能存在相同起点的情况，设计了 `solveOffline` 算法模板：预先对所有点对以起点编号为关键字排序，使得同源目标归并在连续区间内，仅执行一次单源最短路探索即可满足该起点的全部终点查询，将搜索轮次从输入总点对数 $Q$ 减少到不同起点数 $U$。

---

# Chapter 3: Testing Results

## 3.1 正确性与交叉验证

项目在 `code/selftest.cpp` 中构建了自动化回归检验体系，采用 C++ STL 独立的 `std::priority_queue` 作为外部黄金基准（Ground Truth）：
1. **固定边界拓扑检验**：构造极端小图，严格测试了零权边、自环、平行边、等距并列最短路以及包含不可达顶点的连通分量，验证四种自实现堆的距离计算完全一致；
2. **30 组随机小图全面对拍**：基于固定随机种子生成 30 组稠密/稀疏随机图，对四种自实现堆与 STL 基准对比全点对最短路矩阵、全图可达顶点总数以及累计距离和，确保无任何分支逻辑偏差；
3. **离线同源策略正确性验证**：针对离线合并算法，专门构造了包含起点乱序、重复起点、不可达目标及多终点并发的测试用例，验证其输出答案与常规逐条查询在原始下标位置上 100% 吻合；
4. **测试结论**：程序在编译运行后输出 `ALL TESTS PASSED: four heaps match STL priority_queue`，验证了全部实现的正确性。

| 测试类别 | 测试目的 | 验证方法 / 基准 | 实测结果 | 状态 |
| :--- | :--- | :--- | :--- | :---: |
| **Boundary Graph Test** | 验证零权边、自环、平行边、不可达点 | 确定性预期矩阵对比 | 距离与连通性精确吻合 | **PASS** |
| **30-Group Random Test**| 随机图高频操作压力测试 | 对比 STL priority_queue 距离和 | 30 组测试零误差 | **PASS** |
| **Unreachable Target** | 终点不可达极端情况 | 验证返回值是否为 KEY_INF | 正确识别无穷大 | **PASS** |
| **Offline Grouping Test**| 验证乱序起点、重复源点与结果还原 | 对比普通逐条查询 dump 数组 | 各查询位置答案完全一致 | **PASS** |

## 3.2 DIMACS 美国道路网络数据集

评测数据集严格遵循第 9 届 DIMACS 算法挑战赛（The 9th DIMACS Implementation Challenge）官方物理距离（`.d.gr`）与通行时间（`.t.gr`）规范。官方源下载链接如下：

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

**查询生成方案**：使用 `data/gen_queries.cpp`，采用 C++11 标准库 `std::mt19937` 随机数生成器。基础随机种子固定为 `20261017`，加网络列表固定下标以确保确定性复现。每张路网生成 1000 组起点与终点互不相同的随机点对。同一区域的物理距离图与通行时间图采用完全相同的点对集合。

## 3.3 测试环境与计时规范

- **硬件环境**：Apple M-series 处理器，16 GB 统一架构内存；
- **操作系统与编译器**：macOS Sequoia，Apple Clang，编译参数 `-O3 -std=c++11`；
- **计时规范**：使用 `std::chrono::steady_clock` / `clock()` 进行高精度计时。计时区间严格界定在算法求解段内（对于离线模式，排序、同源分组、搜索与答案收集均计入耗时），文件读取、图构建与结果 CSV 导出完全置于计时范围之外。

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

### 4.1.1 复杂度对比汇总
在包含 $n = |V|$ 个顶点、$m = |E|$ 条弧的图中，各堆结构的核心操作理论边界对比如下：

| 操作 / 算法 | 经典二叉堆 | 二项堆 | 斐波那契堆 |
| :--- | :---: | :---: | :---: |
| **Insert** | $O(\log n)$ | 最坏 $O(\log n)$ / 摊还 $O(1)$ | **摊还 $O(1)$** |
| **Extract-Min** | $O(\log n)$ | $O(\log n)$ | **摊还 $O(\log n)$** |
| **Decrease-Key** | $O(\log n)$ | $O(\log n)$ | **摊还 $O(1)$** |
| **Dijkstra 单次总复杂度** | $O((m + n) \log n)$ | $O((m + n) \log n)$ | **$O(m + n \log n)$** |

### 4.1.2 批量查询与离线同源策略的理论对比
设图有 $n$ 个顶点、$m$ 条弧，输入 $Q$ 组点对查询，其中包含 $U$ 个互不相同的起点（$1 \le U \le Q$）：
1. **普通逐点对查询（最坏遍历全图）**：
   - 经典二叉堆与二项堆：总时间复杂度为 $O(Q(n + m) \log n)$；
   - 斐波那契堆：总时间复杂度为 $O(Q(m + n \log n))$。
2. **离线同源合并策略 (Offline Grouping)**：
   - 预处理排序耗时为 $O(Q \log Q)$；
   - 随后仅对 $U$ 个不同起点各执行一次完整单源遍历，二叉堆总耗时为 $O(Q \log Q + U(n + m) \log n)$，斐波那契堆总耗时为 $O(Q \log Q + U(m + n \log n))$。
   - **理论与实际权衡**：在高度集中的查询分布（如 $U \ll Q$）中，离线策略消除了近 $Q - U$ 次重复搜索，带来极大的理论加速；但若起点几乎完全分散（$U \approx Q$），由于完整遍历无法享受终点早停带来的剪枝收益，实际耗时可能反超普通早停模式。

### 4.1.3 势能函数与各项操作摊还代价严格推导
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

### 4.1.4 最大度数对数界限定理证明
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
在 NY 道路网络（26 万顶点）上，对比了全量重置数组（$O(|V|)$）与依据本次访问记录进行按需重置（$O(\text{Touched})$）的耗时差异：

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
- **批量同源查询场景**：优先采用**离线排序与同源分组优化（Offline Grouping）**，以单次遍历共享服务多个目标，显著缩减无效的重复全图探索；
- **理论算法设计**：在 Prim 最小生成树算法及网络流等减键占主导的算法证明中，斐波那契堆具备明确的理论上界保障。

## 4.5 实验局限性与未来展望

本项目测试主要针对平面稀疏道路网络与合成三层图。后续研究可进一步在幂律分布的社交网络图及生物信息网络上展开测试，同时可将配对堆（Pairing Heap）纳入对比，以探索常数开销与渐近性能之间的更优折中。

---

# Appendix

## 附录 A：核心源代码实现

以下给出项目中对应最新仓库（`code/heaps.h`、`code/dijkstra.h`、`code/offline.h`）的核心源码实现。全部基于 C++11/C++17 标准库编写，包含完整的类设计与逐行详尽注释。

### 1. 四大优先队列核心实现 (`code/heaps.h`)

```cpp
#ifndef HEAPS_H
#define HEAPS_H

#include <deque>
#include <vector>
#include <utility>

using namespace std;
typedef long long Key;
const Key KEY_INF = 1LL << 60;

// =============================================================================
//  1. 经典索引二叉堆（数组完全二叉树 + pos 映射，支持 O(log n) decrease-key）
// =============================================================================
struct BinaryHeap {
	BinaryHeap(int n = 0) {
		init(n);
	}

	// 按顶点编号分配索引空间，所有顶点初始都不在堆中
	void init(int n) {
		key.assign(n + 1, 0);   // 有效性由位置索引决定，键值无需统一设为无穷
		pos.assign(n + 1, -1);  // -1 表示尚未入堆
		heap.clear();
		touched.clear();
		heap.reserve(n + 1);
		touched.reserve(n + 1);
		cmp_cnt = 0;
	}

	// 只恢复本轮涉及的索引并复用已有容量，避免逐轮重新分配内存
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

	// 插入新顶点，加入数组尾部后上浮调整
	void insert(int v, Key k) {
		key[v] = k;
		pos[v] = (int)heap.size();
		heap.push_back(v);
		touched.push_back(v);
		siftUp((int)heap.size() - 1);
	}

	// 原地减键：根据 pos 定位后沿父链上浮调整
	void decreaseKey(int v, Key k) {
		key[v] = k;
		siftUp(pos[v]);
	}

	// 弹出最小候选距离，尾元素补根后下沉调整
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

	// 节点上浮调整，同步维护父子节点的 pos 反向映射
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

	// 节点下沉调整，每次选择较小孩子上移
	void siftDown(int i) {
		int n = (int)heap.size();
		int v = heap[i];
		Key k = key[v];
		for (;;) {
			int c = 2 * i + 1; // 左孩子下标
			if (c >= n) {
				break;
			} else {
				if (c + 1 < n && lessKey(key[heap[c + 1]], key[heap[c]])) {
					c ++ ; // 右孩子更小时选取右孩子
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

	vector<Key> key;       // 各顶点当前候选距离
	vector<int> pos;       // 顶点在堆数组中的下标映射
	vector<int> heap;      // 存储顶点编号的完全二叉树数组
	vector<int> touched;   // 记录本轮涉及的顶点
	long long cmp_cnt = 0;  // 键值比较计数器
};

// =============================================================================
//  2. 二项堆（严格二项树森林，根链表按度数升序排列）
// =============================================================================
struct BinomialHeap {
	struct Node {
		Key key;
		int v;
		int degree;
		Node *parent;
		Node *child;
		Node *sibling;
	};

	BinomialHeap(int n = 0) {
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

	// 减键沿父指针向上冒泡交换键值与顶点编号
	void decreaseKey(int v, Key k) {
		Node *x = handle[v];
		x->key = k;
		while (x->parent && lessKey(x->key, x->parent->key)) {
			Node *p = x->parent;
			swap(x->key, p->key);
			swap(x->v, p->v);
			handle[x->v] = x;
			handle[p->v] = p;
			x = p;
		}
	}

	// 弹出最小根：摘除最小根并将子树链表逆序排列后合并
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

	// 复用稳定地址的对象池，避免高频系统分配
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

	void link(Node *y, Node *z) {
		y->parent = z;
		y->sibling = z->child;
		z->child = y;
		z->degree ++ ;
	}

	// 合并根链表消除同阶相邻树
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

	deque<Node> pool;       // 使用 deque 保证扩容时节点指针地址永久稳定
	vector<Node*> handle;
	vector<char> in;
	vector<int> touched;
	Node *head = nullptr;
	int nfree = 0;
	int total_size = 0;
	long long cmp_cnt = 0;
};

// =============================================================================
//  3. 斐波那契堆（双向循环链表 + 级联剪枝 + 延迟合并）
// =============================================================================
struct FibonacciHeap {
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

	FibonacciHeap(int n = 0) {
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

	// 插入操作：直接加入根循环双链表，耗时 O(1)
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

	// 减键操作：若破坏堆序执行切断与级联剪切，均摊耗时 O(1)
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

	// 提取最小根：提升子节点到根表，执行 consolidate 树归并
	int extractMin() {
		Node *z = min_node;
		int v = z->v;

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

		z->left->right = z->right;
		z->right->left = z->left;

		if (z == z->right) {
			min_node = nullptr;
		} else {
			min_node = z->right;
			consolidate();
		}

		handle[v] = nullptr;
		in[v] = 0;
		total_size -- ;
		return v;
	}

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
				y->mark = true;
			} else {
				cut(y, z);
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
					swap(x, y);
				}
				linkFib(y, x);
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

	deque<Node> pool;
	vector<Node*> handle;
	vector<char> in;
	vector<int> touched;
	vector<Node*> A;
	vector<int> used;
	vector<Node*> roots;
	Node *min_node = nullptr;
	int nfree = 0;
	int total_size = 0;
	long long cmp_cnt = 0;
};

// =============================================================================
//  4. 惰性删除二叉堆（Lazy Binary Heap）
// =============================================================================
struct LazyBinaryHeap {
	LazyBinaryHeap(int = 0) {}

	void clear() {
		key.clear();
		ver.clear();
	}

	bool empty() const {
		return key.empty();
	}

	void push(int v, Key k) {
		key.push_back(k);
		ver.push_back(v);
		siftUp((int)key.size() - 1);
	}

	int extractMin(Key &outKey) {
		int v = ver[0];
		outKey = key[0];
		Key lk = key.back();
		int lv = ver.back();
		key.pop_back();
		ver.pop_back();
		if (!key.empty()) {
			key[0] = lk;
			ver[0] = lv;
			siftDown(0);
		}
		return v;
	}

	long long cmps() const {
		return cmp_cnt;
	}

	void siftUp(int i) {
		Key k = key[i];
		int v = ver[i];
		while (i > 0) {
			int p = (i - 1) >> 1;
			if (!lessKey(k, key[p])) {
				break;
			} else {
				key[i] = key[p];
				ver[i] = ver[p];
				i = p;
			}
		}
		key[i] = k;
		ver[i] = v;
	}

	void siftDown(int i) {
		int n = (int)key.size();
		Key k = key[i];
		int v = ver[i];
		for (;;) {
			int c = 2 * i + 1;
			if (c >= n) {
				break;
			} else {
				if (c + 1 < n && lessKey(key[c + 1], key[c])) {
					c ++ ;
				}
				if (!lessKey(key[c], k)) {
					break;
				} else {
					key[i] = key[c];
					ver[i] = ver[c];
					i = c;
				}
			}
		}
		key[i] = k;
		ver[i] = v;
	}

	bool lessKey(Key a, Key b) const {
		cmp_cnt ++ ;
		return a < b;
	}

	vector<Key> key;
	vector<int> ver;
	mutable long long cmp_cnt = 0;
};

#endif
```

### 2. 图结构、适配器与统一 Dijkstra 求解引擎 (`code/dijkstra.h`)

```cpp
#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <vector>
#include "heaps.h"

// CSR 紧凑邻接表结构
struct Graph {
	int n = 0;
	std::vector<int> head;
	std::vector<int> to;
	std::vector<int> w;
};

// 堆操作统一适配器：常规堆原地更新
template <typename Heap>
struct HeapAccess {
	static void insert(Heap& heap, int v, Key k) { heap.insert(v, k); }
	static void decrease(Heap& heap, int v, Key k) { heap.decreaseKey(v, k); }
	static int pop(Heap& heap, Key& k, const std::vector<Key>& dist) {
		int u = heap.extractMin();
		k = dist[u];
		return u;
	}
	static const bool lazy = false;
};

// 堆操作统一适配器：惰性堆重复插入
template <>
struct HeapAccess<LazyBinaryHeap> {
	static void insert(LazyBinaryHeap& heap, int v, Key k) { heap.push(v, k); }
	static void decrease(LazyBinaryHeap& heap, int v, Key k) { heap.push(v, k); }
	static int pop(LazyBinaryHeap& heap, Key& k, const std::vector<Key>&) {
		return heap.extractMin(k);
	}
	static const bool lazy = true;
};

// 微观操作计数器
struct DijkstraStats {
	long long pops = 0, pushes = 0, decs = 0, arcScans = 0, cmps = 0, settled = 0;
	unsigned long long sumDist = 0;
};

// 统一模板驱动的 Dijkstra 求解引擎
template <typename Heap>
struct DijkstraRunner {
	DijkstraRunner(const Graph& g)
		: g_(g), dist(g.n + 1, KEY_INF), done(g.n + 1, 0), seen(g.n + 1, 0), heap(g.n) {}

	// 主搜索例程：支持普通点对早停与离线多目标收集
	Key run(int s, int t, bool full, DijkstraStats& st,
			const std::vector<int>* targets = nullptr, std::vector<Key>* answers = nullptr) {
		long long c0 = heap.cmps();
		heap.clear();
		touched.push_back(s);
		seen[s] = 1;
		dist[s] = 0;
		HeapAccess<Heap>::insert(heap, s, 0);
		st.pushes ++ ;
		Key answer = KEY_INF;

		while (!heap.empty()) {
			Key k;
			int u = HeapAccess<Heap>::pop(heap, k, dist);
			st.pops ++ ;
			// 惰性堆过期条目过滤
			if (done[u] || k != dist[u]) {
				continue;
			}
			done[u] = 1;
			st.settled ++ ;
			st.sumDist += (unsigned long long)k;

			// 目标早停判断
			if (!full && u == t) {
				answer = k;
				break;
			}

			// 出边松弛循环
			for (int i = g_.head[u]; i < g_.head[u + 1]; i ++ ) {
				st.arcScans ++ ;
				int v = g_.to[i];
				if (done[v]) {
					continue;
				}
				Key nd = k + g_.w[i];
				if (!seen[v]) {
					seen[v] = 1;
					touched.push_back(v);
					dist[v] = nd;
					HeapAccess<Heap>::insert(heap, v, nd);
					st.pushes ++ ;
				} else if (nd < dist[v]) {
					dist[v] = nd;
					HeapAccess<Heap>::decrease(heap, v, nd);
					st.decs ++ ;
					if (HeapAccess<Heap>::lazy) {
						st.pushes ++ ;
					}
				}
			}
		}

		// 离线同源查询答案批量回填
		if (targets && answers) {
			answers->clear();
			for (int v : *targets) {
				answers->push_back(seen[v] ? dist[v] : KEY_INF);
			}
		}

		// 按需局部重置：仅清除本轮被发现顶点的标记，复杂度严格为 O(Touched)
		for (int v : touched) {
			done[v] = 0;
			seen[v] = 0;
		}
		touched.clear();
		st.cmps += heap.cmps() - c0;
		return answer;
	}

	const Graph& g_;
	std::vector<Key> dist;
	std::vector<char> done, seen;
	std::vector<int> touched;
	Heap heap;
};

#endif
```

### 3. 离线同源查询聚类与共享搜索算法 (`code/offline.h`)

```cpp
#ifndef OFFLINE_H
#define OFFLINE_H

#include <vector>
#include <algorithm>
#include "dijkstra.h"

// 记录原查询下标，供答案顺序还原
struct OfflineQuery {
	int source;
	int target;
	size_t index;
};

struct OfflineResult {
	std::vector<Key> answers;
	DijkstraStats stats;
	size_t runs = 0; // 实际执行的单源遍历次数
};

// 离线同源分组算法：按起点排序，共享完整搜索
template <typename Heap>
OfflineResult solveOffline(DijkstraRunner<Heap>& runner,
                           const std::vector<int>& sources, const std::vector<int>& targets) {
	OfflineResult result;
	result.answers.resize(sources.size(), KEY_INF);
	std::vector<OfflineQuery> queries;
	queries.reserve(sources.size());
	for (size_t i = 0; i < sources.size(); i ++ ) {
		queries.push_back({sources[i], targets[i], i});
	}

	// 按源点优先、原下标次之排序
	std::sort(queries.begin(), queries.end(), [](const OfflineQuery& a, const OfflineQuery& b) {
		return a.source != b.source ? a.source < b.source : a.index < b.index;
	});

	// 按相同起点聚类批次处理
	for (size_t begin = 0; begin < queries.size();) {
		size_t end = begin + 1;
		while (end < queries.size() && queries[end].source == queries[begin].source) {
			end ++ ;
		}
		std::vector<int> groupTargets;
		groupTargets.reserve(end - begin);
		for (size_t i = begin; i < end; i ++ ) {
			groupTargets.push_back(queries[i].target);
		}
		std::vector<Key> groupAnswers;

		// 对同源目标执行单次完整单源遍历
		runner.run(queries[begin].source, -1, true, result.stats, &groupTargets, &groupAnswers);
		result.runs ++ ;

		// 将答案还原至原查询下标
		for (size_t i = begin; i < end; i ++ ) {
			result.answers[queries[i].index] = groupAnswers[i - begin];
		}
		begin = end;
	}
	return result;
}

#endif
```

## 附录 B：小组成员分工

| 姓名 | 学号 | 承担任务与贡献比例 |
| :---: | :---: | :--- |
| **【成员1】** | 【学号1】 | 四种优先队列与统一 DijkstraRunner 模板引擎设计、人工三层图减键控制实验设计与实现 (40%) |
| **【成员2】** | 【学号2】 | DIMACS 道路网基准测试执行、离线同源优化策略与 STL priority_queue 自动化回归对拍程序 (30%) |
| **【成员3】** | 【学号3】 | 报告撰写与实验分析、势能法与斐波那契度数上界推导、数据可视化图表绘制与排版 (30%) |

*注：若为独立完成项目，最终提交前可调整本表为个人独立声明。*
