// =============================================================================
//  report.typ —— ADS Project 2 报告（中文）
//  编译： typst compile --root . report/report.typ
// =============================================================================

#set page(paper: "a4", margin: (x: 2.1cm, top: 2.3cm, bottom: 2.2cm), numbering: "1")
#set text(font: ("Times New Roman", "Microsoft YaHei"), size: 10.5pt, lang: "zh")
#set par(justify: true, leading: 0.78em, first-line-indent: 2em)
#set heading(numbering: (..nums) => {
  let p = nums.pos()
  if p.len() == 1 { "Chapter " + str(p.at(0)) } else { p.map(str).join(".") }
})
#show heading.where(level: 1): set block(above: 1.3em, below: 0.8em)
#show heading.where(level: 1): set text(size: 14pt)
#show heading.where(level: 2): set text(size: 12pt)
#show raw.where(block: true): set text(size: 7.2pt, font: ("Consolas", "Microsoft YaHei"))
#show figure.caption: set text(size: 9pt)
#set figure(gap: 0.7em)
// 图号跟随章号：第一章的图编为图 3.1、图 3.2…，与正文引用一致
#set figure(numbering: n => {
  let levels = counter(heading).get()
  let chapter = if levels.len() > 0 { levels.at(0) } else { 0 }
  numbering("1.1", chapter, n)
})

#import "tables.typ": *

// 代码等宽块的统一样式
#let pseudo(s) = block(
  width: 100%, inset: 9pt, radius: 2pt, fill: luma(246), above: 0.7em, below: 0.7em,
  text(font: ("Consolas", "Microsoft YaHei"), size: 8.4pt, raw(s, block: true, tab-size: 2)),
)

#let codefile(path, lang: "cpp") = raw(read(path), lang: lang, block: true, tab-size: 4)

// 数据链路图（graphviz 生成的 PNG），带图题
#let fig(path, cap, w: 86%) = figure(image(path, width: w), caption: cap)

// =============================================================================
//  封面
// =============================================================================
#align(center)[
  #v(1.4cm)
  #text(size: 19pt, weight: "bold")[Shortest Path Algorithm with Heaps] \
  #v(0.35cm)
  #text(size: 12.5pt)[—— 基于斐波那契堆、二项堆与二叉堆的 Dijkstra 实现与实验评估 ——]
  #v(1.8cm)
  #text(size: 10.5pt)[
    课程：高级数据结构与算法（ADS）· Project 2 \
    题目：8-1 Shortest Path Algorithm with Heaps \
    姓名：【】　　学号：【】　　专业班级：【】 \
    日期：2026 年 10 月
  ]
  #v(1.6cm)
]
#align(center)[#text(size: 10pt)[测试平台：13th Gen Intel Core i9-13900HX（32 逻辑核）/ 16 GB RAM / Windows 11 + MinGW-w64 GCC 14]]
#v(1.2cm)

#heading(numbering: none, level: 1)[摘要]

本报告完成了题目 8-1 的全部要求：用 *二叉堆、二项堆和斐波那契堆* 三种最小优先队列分别实现
Dijkstra 最短路算法，在 DIMACS Challenge 9 的美国公路网基准上进行大规模实验（#n_datasets 个
数据集、每个数据集 #n_queries 组随机点对查询），并给出运行时间、堆操作次数与比较次数的完整统计。

报告第一部分（Chapter 2）重点介绍了 *斐波那契堆* 这一课外数据结构：它的结构、五种基本操作的
伪代码，以及用势能法对 extract-min 的 $O(log n)$ 和 decrease-key 的 $O(1)$ 摊还界的*完整证明*，
并给出三种堆的复杂度对比。第二部分（Chapter 3、4）给出全部实现、正确性测试与性能测试结果，
并分析了「理论最优 ≠ 实践最优」这一经典现象。

核心结论：三种堆在同一批查询上完成的*松弛次数完全相同*（差异 < 0.001%），因此耗时差异完全来自
堆本身。实测中 *斐波那契堆并没有获胜*——它比二叉堆慢 #ratio_fib 倍，二项堆慢 #ratio_binom 倍；
原因在于（i）公路网平均度很小、decrease-key 占比低，斐波那契堆摊还 $O(1)$ 的 decrease-key 优势
无从发挥；（ii）其 extract-min 需要 consolidate，常数因子大。而在最极端的「完整单源」实验中，
三者的相对差距缩小到 1.5 倍以内，与理论预测的趋势一致。

#pagebreak()
#outline(title: [目录], depth: 2)
#pagebreak()

= Introduction

== 问题描述

最短路径问题是组合优化中最基础的问题之一。给定带非负权重的有向图 $G = (V, E)$ 以及源点 $s$，
单源最短路（SSSP）要求出 $s$ 到所有顶点的最短距离；若只关心某个终点 $t$，则称为点到点最短路（P2P）。

Dijkstra 算法是求解该问题的经典算法，其正确性与效率*强依赖于最小优先队列（min-priority
queue）* 的实现。在最坏情形下，一趟 Dijkstra 需要执行 $|V|$ 次 extract-min、至多 $|E|$ 次
decrease-key，因此优先队列的单次操作代价直接决定整个算法的复杂度：

$ O((n + m) log n) quad "（二叉堆 / 二项堆）" quad "vs." quad O(m + n log n) quad "（斐波那契堆）" $

题目要求在*至少两种*堆结构上实现 Dijkstra（其中必须包含斐波那契堆），并在 DIMACS Challenge 9
的美国公路网基准上评测，目标是「找出最适合 Dijkstra 的数据结构」。

== 本报告的任务与目标

#table(
  columns: (auto, 1fr),
  align: (left, left),
  stroke: 0.4pt + luma(150),
  column-gutter: 8pt,
  strong("题目要求"), strong("本报告的做法"),
  [至少两种堆、必须含斐波那契堆], [实现二叉堆、二项堆、斐波那契堆三种，接口完全一致，另加一种"惰性删除"写法作对照],
  [使用 USA 公路网评测（只列下载链接）], [使用 #n_datasets 个网络，共 #nodes_min – #nodes_max 个结点；链接见表 3.1],
  [至少 1000 组查询], [每个数据集 #n_queries 组随机点对，另在完整单源模式上再测 16 个源点],
  [性能测试程序], [`bench`：自动遍历所有堆、输出 CSV 时间表与逐查询明细],
  [运行时间表 + 规模曲线], [表 3.2 – 3.5 与图 3.1 – 3.5],
  [充分注释的代码], [Appendix A 全文收录，注释行占比 > 30%],
)

== 三种堆的背景

*二叉堆（binary heap）* 是最常见的优先队列实现：用数组存完全二叉树，父结点不大于子结点。
它缓存友好、常数极小，是工程实践中使用最广的选择；缺点是不支持高效的合并，且 decrease-key
需要 $O(log n)$。

*二项堆（binomial heap，Vuillemin 1978）* 是若干棵「二项树」$B_k$ 组成的森林，$B_k$ 有 $2^k$ 个
结点、度数为 $k$。二项堆第一次把 *合并（union）* 做到 $O(log n)$，并让 insert 的摊还代价降为
$O(1)$；但 extract-min 与 decrease-key 的最坏代价仍是 $O(log n)$。

*斐波那契堆（Fibonacci heap，Fredman & Tarjan 1987）* 进一步「偷懒」：insert、合并都只做 $O(1)$
的挂牌操作，把所有整理工作推迟到 extract-min；decrease-key 则通过*切断（cut）* 直接完成，
*摊还* 代价 $O(1)$。它使 Dijkstra 的复杂度从 $O(m log n)$ 降到 $O(m + n log n)$，是理论上的最优选择，
也是后续许多图算法（如 $O(m log n)$ 最小生成树、$O(n log n + m)$ 单源最短路）的基石。
斐波那契堆是本项目中*唯一的课外内容*，Chapter 2 将给出完整的原理与摊还分析证明。

== 报告组织

Chapter 2 给出图存储、Dijkstra 框架以及三种堆的规范与伪代码，并重点证明斐波那契堆的摊还复杂度；
Chapter 3 给出正确性测试、数据集说明与全部性能测试结果；Chapter 4 分析数据、解释理论与实测的差异
并讨论改进方向；Appendix A 收录全部源代码。

#pagebreak()

= 数据结构与算法规范

== 图的存储：CSR

实验中最大
的图有 #nodes_max 个结点、#arcs_max 条弧，必须使用紧凑且缓存友好的存储。本项目采用 *CSR
（Compressed Sparse Row）*，用三个数组表示整张图：

#pseudo("head[0 .. n]      // head[u]..head[u+1]-1 就是顶点 u 的出边区间
to[0 .. m-1]     // 弧头
w [0 .. m-1]     // 弧权")

顶点 $u$ 的全部出边是 `to[head[u] .. head[u+1]-1]`，是一段连续内存，因此遍历出边时
几乎没有额外指针跳转。读取 `.gr` 文本文件时采用「两遍扫描」：第一遍只统计每个顶点的出度，
前缀和后得到 `head`；第二遍才填充 `to`/`w`。这样无需缓存 $(u, v, w)$ 三元组，内存占用最小，
190 万结点的图载入时间约 0.3 s（不计入算法计时）。

== Dijkstra 算法框架

=== 可索引堆 + decrease-key（标准写法）

三种堆都实现为「可索引」（indexed）优先队列，即堆内部维护 顶点 → 堆结点 的映射，
从而直接支持 decrease-key。这样写既符合教材中三种堆的标准形式，也保证了公平比较。

#pseudo("DIJKSTRA(G, w, s, t)                     // 可索引堆版本
  for each v in V: dist[v] ← +∞, settled[v] ← false
  dist[s] ← 0 ; H.INSERT(s, 0)

  while not H.EMPTY():
      u ← H.EXTRACT-MIN()                     // u 的最短距离已确定
      settled[u] ← true
      if u = t: return dist[u]                // 点到点查询：提前终止
      for each arc (u, v, w) with u → v:
          if settled[v]: continue
          if dist[u] + w < dist[v]:
              dist[v] ← dist[u] + w
              if H.CONTAINS(v): H.DECREASE-KEY(v, dist[v])   // 已在堆中
              else:             H.INSERT(v, dist[v])         // 首次发现
  return dist[t]")

=== 惰性删除写法（附加对照，非题目要求）

工程上更常见的是不实现 decrease-key：松弛成功就再 `push` 一份副本，extract-min 时若发现关键字
已过时就丢弃。它把堆大小从 $O(n)$ 放大到 $O(m)$，理论复杂度略差，但省去了索引维护，常数更小。
把它加入对照是为了回答「为什么实践中大家用普通二叉堆」，详见 Chapter 4。

#pseudo("DIJKSTRA-LAZY(G, w, s, t)                // 惰性删除版本
  for each v in V: dist[v] ← +∞, settled[v] ← false
  dist[s] ← 0 ; H.PUSH(s, 0)

  while not H.EMPTY():
      (k, u) ← H.EXTRACT-MIN()
      if settled[u]: continue                 // 陈旧副本，丢弃
      settled[u] ← true
      if u = t: return k
      for each arc (u, v, w):
          if settled[v]: continue
          if k + w < dist[v]:                  // 松弛成功 → 直接再 push
              dist[v] ← k + w ; H.PUSH(v, dist[v])
  return +∞")

== 二叉堆（Binary Heap）

数组实现完全二叉树：`heap[i]` 的父结点为 `heap[(i-1)/2 向下取整]`，左右孩子为 `heap[2i+1]`、`heap[2i+2]`。
为了支持 decrease-key，额外维护 `pos[v]` 记录顶点 $v$ 在数组中的位置。

#table(
  columns: (auto, auto, 1fr),
  align: (left, center, left),
  stroke: 0.4pt + luma(150),
  strong("操作"), strong("代价"), strong("做法"),
  [insert], [$O(log n)$], [放到数组末尾，然后向上浮动（sift-up）],
  [extract-min], [$O(log n)$], [队首与队尾交换，删掉队尾，队首向下沉（sift-down）],
  [decrease-key], [$O(log n)$], [`pos[v]` $O(1)$ 定位，随后 sift-up],
  [find-min], [$O(1)$], [返回 `heap[0]`],
  [merge], [$O(n)$], [只能逐个重新插入；这正是二叉堆的短板],
)

== 二项堆（Binomial Heap）

*二项树* $B_k$ 递归定义：$B_0$ 是单结点；$B_k$ 由两棵 $B_(k-1)$ 相接而成（其中一棵的根成为另一棵
根的最左孩子）。因此 $B_k$ 有 $2^k$ 个结点、高度 $k$、根的孩子恰为 $B_0, B_1, ..., B_(k-1)$。
二项堆是*一组二项树*，满足：

+ 每棵树都满足小顶堆性质（父 ≤ 子）；
+ 根链表按度数递增排列；
+ *任一时刻每种度数至多存在一棵树*（这一点由 extract-min 中的合并维持）。

「每种度数至多一棵」是关键：任一度数 $k$ 要么没有，要么恰有一棵 $B_k$，于是把 $n$ 写成二进制，
堆中至多有 $floor(log_2 n) + 1$ 棵树——这正是 $O(log n)$ 的来源。

#pseudo("LINK(y, z)                                  // 让 y 成为 z 的孩子（要求 degree 相同）
  y.parent ← z
  y.sibling ← z.child
  z.child ← y
  z.degree ← z.degree + 1

MERGE-ROOTS(h1, h2)                         // 按度数递增归并两条根链表
  ...（标准归并，O(log n)）

UNION(h1, h2)                               // 归并 + 把同度数的相邻根两两链接
  h ← MERGE-ROOTS(h1, h2)
  prev ← NIL ; x ← h ; next ← x.sibling
  while next ≠ NIL:
      if x.degree ≠ next.degree or (next.sibling ≠ NIL and next.sibling.degree = x.degree):
          prev ← x ; x ← next                     // 情况 1：先跳过
      else if x.key ≤ next.key:
          x.sibling ← next.sibling ; LINK(next, x) // x 当父结点
      else:
          prev.sibling ← next ; LINK(x, next)      // next 当父结点
          x ← next
      next ← x.sibling
  return h

INSERT(v, k)      : 新建单结点树 B₀，head ← UNION(head, B₀)          O(log n)
MINIMUM(H)        : 扫描根链表取最小                                   O(log n)
EXTRACT-MIN(H)    : 摘掉最小根；其孩子链表逆序后接回根链表；再 UNION   O(log n)
DECREASE-KEY(v,k) : key 变小后沿父指针上浮（交换载荷 key 与顶点编号）   O(log n)")

== 斐波那契堆（Fibonacci Heap）

=== 结构

斐波那契堆与二项堆一样是「树的森林」，但*不再要求树的形状*，并且把「整理」推迟到不得不做的时候：

+ 每棵树满足小顶堆性质；
+ 所有根结点用*循环双向链表*串起来，`min` 指针指向关键字最小的根；
+ 每个结点的所有孩子也用*循环双向链表*连接；
+ 每个结点记录 `degree`（孩子数）与 `mark`（是否已经失去过一个孩子）；
+ *无序二项树性质*：度数为 $k$ 的结点 $x$，其孩子 $y_1, ..., y_k$ 按「被链接到 $x$ 的时间」排序后，
  有 $y_i."degree" ≥ i - 1$。这是因为 LINK 只在两棵度数相同的树之间发生，$y_i$ 被链接时 $x$ 已有
  $i-1$ 个孩子；而 $y_i$ 之后最多失去一个孩子（失去第二个时它自己就会被 cut 下来）。
  该性质是后面 $D(n) = O(log n)$ 证明的基石。

=== 五种操作

#pseudo("INSERT(v, k)                       // O(1)
  新建结点 x（key=k, degree=0, mark=false）
  把 x 挂到根链表（min 的右侧）
  if k < min.key: min ← x

MINIMUM(H)                         // O(1)：直接读 min 指针

UNION(H1, H2)                      // O(1)：把两条根链表首尾相接，取较小的 min

EXTRACT-MIN(H)                     // O(log n) 摊还
  z ← min
  for each child c of z:           // 把孩子全部挂到根链表（不排序、不合并）
      把 c 从 z 的孩子链表摘下，挂到根链表，c.parent ← NIL
  把 z 从根链表摘下
  if z 是根链表中唯一结点: min ← NIL
  else: min ← 任意剩余根 ; CONSOLIDATE()      // 这里才做整理

CONSOLIDATE()                      // 把「同度数」的根两两链接，使每种度数至多一个根
  A[0..D(n)] ← NIL
  for each root w in 根链表:
      x ← w ; d ← x.degree
      while A[d] ≠ NIL:
          y ← A[d]
          if y.key < x.key: swap(x, y)
          LINK(y, x)               // y 变成 x 的孩子；x.degree ← d+1
          A[d] ← NIL ; d ← d + 1
      A[d] ← x
  用 A[] 中非空的根重建根链表并更新 min            // 根数 ≤ D(n) + 1

DECREASE-KEY(v, k)                 // O(1) 摊还
  x ← 顶点 v 对应的结点 ; x.key ← k
  if x.parent ≠ NIL and k < x.parent.key:
      CUT(x, x.parent)             // 把 x 从父结点上切下来，挂到根链表
      CASCADING-CUT(x.parent)      // 若父结点已被标记，继续向上切
  if k < min.key: min ← x

CUT(x, y)                          // O(1)：把 x 从 y 的孩子链表中摘下并挂到根链表
  y.degree ← y.degree - 1 ; x.parent ← NIL ; x.mark ← false
  把 x 插入根链表

CASCADING-CUT(y)                   // 级联切断
  z ← y.parent
  if z ≠ NIL:
      if y.mark = false: y.mark ← true    // 第一次失去孩子：只做标记
      else: CUT(y, z) ; CASCADING-CUT(z)  // 第二次：自己也切走，并继续向上")

=== 摊还分析（势能法）

取势能函数

$ Phi(H) = t(H) + 2 m(H) $

其中 $t(H)$ 是根链表中树的棵数，$m(H)$ 是*被标记*的结点数。直观上，$t$ 度量「EXTRACT-MIN 里
CONSOLIDATE 需要做的工作量」，$2m$ 则为「级联切断」预付了代价——每做一次 cut 就从 $m$ 中扣除 1 个
标记，恰好抵消。

*（1）insert / minimum / union 是 $O(1)$ 摊还。*
insert 实际代价 $O(1)$，只增加一根树，$Delta t = +1$；不改变标记数。故
摊还代价 $= O(1) + 1 = O(1)$。union 只是拼接两条根链表，$Delta Phi = 0$，摊还 $O(1)$。

*（2）extract-min 是 $O(log n)$ 摊还。*

先给出最大度数 $D(n)$ 的上界。

#text(weight: "bold")[引理 1（CLRS 19.1）.] 设 $x$ 是斐波那契堆中任一结点，$k = x."degree"$，
记 $F_1 = F_2 = 1, F_(i) = F_(i-1) + F_(i-2)$ 为斐波那契数列，$phi = (1 + sqrt(5))/2$ 为黄金比，则

$ "size"(x) ≥ F_(k+2) ≥ phi^k $

#text(weight: "bold")[证明.] 对 $k$ 归纳。$k = 0$ 时 $"size"(x) = 1 = F_2$ ✓。
设 $k > 0$，$y_1, ..., y_k$ 是 $x$ 的孩子，按*被链接到 $x$ 的时间先后*排列。
当 $y_i$ 被链接时，$x$ 已经有 $i - 1$ 个孩子，而 LINK 只发生在度数相同的两棵树之间，
故那时 $"degree"(y_i) ≥ i - 1$。此后 $y_i$ 最多失去一个孩子——若失去第二个，$y_i$ 自身就会被
CUT 下来而不再是 $x$ 的孩子。所以 $y_i$ 当前的度数 $≥ i - 1$，由归纳假设有
$ "size"(y_i) ≥ F_((i-1)+2) = F_(i+1) $。于是

$ "size"(x) = 1 + sum_(i=1)^k "size"(y_i) ≥ 1 + sum_(i=1)^k F_(i+1) = 1 + (F_(k+3) - 2) = F_(k+3) - 1 ≥ F_(k+2) $

再由 $F_(k+2) ≥ phi^k$（$F$ 的标准下界）得 $"size"(x) ≥ phi^k$。$square$

#text(weight: "bold")[推论.] 若堆中有 $n$ 个结点，则任一结点的度数满足 $k ≤ log_phi n$，
即 $D(n) = O(log n)$（准确地说 $D(n) ≤ floor(log_phi n)$）。

现在分析 EXTRACT-MIN。设整理前的根数为 $t$。

+ *实际代价*：把孩子挂到根链表 $O(D(n))$；CONSOLIDATE 中根链表被重建，每个根至多被链接一次，
  故 $O(D(n) + t)$。
+ *势能变化*：整理后每种度数至多一个根，故根数 $≤ D(n) + 1$，即 $Delta t ≤ D(n) + 1 - t$；
  标记数不增加，$Delta m ≤ 0$。于是 $Delta Phi ≤ D(n) + 1 - t$。
+ *摊还代价* $= O(D(n) + t) + D(n) + 1 - t = O(D(n)) = O(log n)$。$square$

*（3）decrease-key 是 $O(1)$ 摊还。*

设本次 decrease-key 触发了 $c$ 次级联切断（其中 1 次是初始的 CUT）。

+ *实际代价*：$O(1) + c$。
+ *势能变化*：每次 CUT 使根数增加 1，故 $Delta t = c$。对标记数：级联切断的过程是「把 $c-1$ 个
  带标记的结点变成不标记（并继续向上），最后可能把 1 个不带标记的结点变成带标记」，
  故 $Delta m ≤ 1 - (c - 1) = 2 - c$。
+ 于是 $Delta Phi = Delta t + 2 Delta m ≤ c + 2(2 - c) = 4 - c$，
  *摊还代价* $= (O(1) + c) + (4 - c) = O(1)$。$square$

*（4）回到 Dijkstra。* 一趟 Dijkstra 中：每个顶点至多 insert 一次、extract-min 一次，
每条边至多触发一次成功的 decrease-key。因此总代价为

$ underbrace(n · O(1))_"insert" + underbrace(n · O(log n))_"extract-min" + underbrace(m · O(1))_"decrease-key" = O(m + n log n) $

而用二叉堆或二项堆时为 $O(m log n + n log n) = O((n + m) log n)$。这就是斐波那契堆在
Dijkstra 上的理论优势。

== 三种堆的复杂度对比

#block(breakable: false, table(
  columns: (1fr, auto, auto, auto, auto),
  align: (left, center, center, center, center),
  stroke: 0.4pt + luma(150),
  table.header(
    strong("操作"), strong("二叉堆"), strong("二项堆"), strong("斐波那契堆"), strong("说明"),
  ),
  [find-min], [$O(1)$], [$O(log n)$], [$O(1)$], [斐波那契堆 / 二叉堆读指针即可],
  [insert], [$O(log n)$], [$O(log n)$ 摊还 $O(1)$], [$O(1)$ 摊还], [斐波那契堆只挂牌，不整理],
  [merge/union], [$O(n)$], [$O(log n)$], [$O(1)$], [斐波那契堆拼接两条链表],
  [extract-min], [$O(log n)$], [$O(log n)$], [$O(log n)$ 摊还], [三者同阶，但常数差别很大],
  [decrease-key], [$O(log n)$], [$O(log n)$], [$O(1)$ 摊还], [*斐波那契堆的核心优势*],
  [Dijkstra 总代价], [$O((n+m) log n)$], [$O((n+m) log n)$], [$O(m + n log n)$], [公路网 $m ≈ 3 n$，差别主要体现在 $log n$ 因子],
))

三个结论值得强调：

+ 斐波那契堆唯一「降阶」的操作是 *decrease-key*；insert/union 的改进只是常数与摊还意义上的。
+ 二项堆相对二叉堆的改进是 *合并* 与 *insert 的摊还代价*，而这两个操作在 Dijkstra 中占比很低，
  所以在 Dijkstra 里二项堆理论上*并不比二叉堆好*——这一点被实验完全证实（Chapter 4）。
+ 当 $m = O(n)$（公路网正属于此类）时，$O(m + n log n)$ 与 $O((n+m) log n)$ 的差别只有
  $log n$ 倍，而斐波那契堆的常数因子通常大 2–4 倍，*理论上限的优势很容易被常数吃掉*。

== 工程实现要点

为了让「堆的差异」在一亿次级别操作的尺度上仍然可靠地可测，实现上做了以下处理：

+ *可索引 + decrease-key*：三种堆都支持 decrease-key，避免用「惰性删除」让二叉堆占便宜；
+ *预分配结点池 + 跨查询复用*：堆对象一次构造、多次 `clear()`，`clear()` 只清理本次查询真正碰过的
  顶点（$O("工作量")$ 而非 $O(n)$），因此不会出现「重置数组」掩盖算法差异的情况；
+ *统一比较计数*：只有关键字比较被计数，三种堆一视同仁，用于分析常数因子（表 3.4）；
+ *手写快速解析器*：`.gr` 文件一次性读入内存后手工扫描，两遍得到 CSR，最大的数据集载入时间在 1 s 以内。

#pagebreak()

= 测试结果

== 正确性测试

正确性测试分三层，全部通过（完整输出见 Appendix B）：

#table(
  columns: (auto, 1fr, auto),
  align: (left, left, center),
  stroke: 0.4pt + luma(150),
  strong("层次"), strong("内容"), strong("结果"),
  [(A) 手工用例], [固定 insert / decrease-key / extract-min 序列，人工核对弹出顺序；CLRS 风格小图验证距离 {10, 9, 5}], [全部通过],
  [(B) 随机对拍], [各 20 万次随机 insert / decrease-key / extract-min，与「线性扫描找最小」的朴素实现逐条比对（三种堆 + 惰性堆）], [全部通过],
  [(C) 交叉验证], [随机图上 60 组查询 + 10 组完整单源，与独立实现的 SPFA（队列式 Bellman-Ford）逐点比对], [校验和一致],
  [(D) 大规模校验], [全部数据集 / 两种模式下，各实现的完整单源距离校验和逐位相同；点对模式下每次查询的 $"dist"(t)$ 逐一相同], [见 Appendix B],
)

其中 (B) 使用唯一关键字（`key = value·n + v`）以消除「并列最小时选择哪个顶点」的歧义，
使比较是严格的；20 万次操作中二叉堆 / 二项堆 / 斐波那契堆的比较次数分别为
1 003 882 / 632 602 / 603 009 次，与后文大规模实验的趋势一致。

在大规模实验上还有两重交叉验证：

+ *完整单源模式*：`bench` 输出的「所有被确定结点距离之和」校验和，在全部 #n_datasets 个
  数据集上四种实现*逐位相同*（见 Appendix B 的 verify 输出）。完整单源模式下这个量是
  与实现无关的，因此这同时证明了四种实现算出的整套距离数组完全一致。
+ *点对查询模式*：由于提前终止，若有结点与 $t$ 的最短距离并列，不同实现可能先弹出不同的结点，
  于是「被确定结点集合」会有极微小的差别（相对差异 $< 10^(-7)$），校验和不再逐位相等。
  因此这里改用*每次查询的答案 $"dist"(t)$* 作为判据：`tools/verify_answers.py` 逐一比对
  四种实现在同一批 #n_queries 组查询上的返回值，结果全部一致（Appendix B）。

因此可以确认：四种实现不仅在理论上、也在约 $10^9$ 次结点弹出、$7 · 10^9$ 次弧扫描的规模上
得到完全相同的最短路径答案。

== 实验环境

#table(
  columns: (auto, 1fr),
  align: (left, left),
  stroke: 0.4pt + luma(150),
  strong("项目"), strong("配置"),
  [CPU], [13th Gen Intel Core i9-13900HX；每种堆单线程运行，各数据集之间严格串行，同一时刻只有一个 `bench` 进程],
  [内存], [16 GB DDR5],
  [操作系统], [Windows 11 x64],
  [编译器], [MinGW-w64 GCC 14，`-O2 -std=c++17 -static`],
  [计时方式], [`std::chrono::steady_clock`，只统计 Dijkstra 主循环，不含数据读入],
  [运行条件], [全部数据集在同一会话、严格串行、且电源计划固定为「高性能」的条件下测完。这几点必须固定：实测并行运行多个数据集会使绝对耗时虚高 2–3 倍，而默认的「平衡」电源计划在电池供电下会让 CPU 降频到约 1/16 的吞吐，使跨数据集的规模曲线完全失真],
  [随机点对], [固定种子生成，同一数据集的所有堆使用*完全相同*的 #n_queries 组查询],
)

== 数据集

数据来自第 9 届 DIMACS 实现挑战赛（Challenge 9: Shortest Paths）的 USA 公路网基准，
均为*有向图*，弧权为两结点间的实际距离（单位：米）。按题目要求，此处只列出下载链接：

#align(center)[#tbl_links]
#align(center)[#text(size: 8.5pt)[表 3.1　实验中使用的 USA 公路网数据集。下载页：#link_note]]

每个数据集生成 #n_queries 组*均匀随机*点对 $(s, t)$（$s ≠ t$，种子由网络名派生，
因此查询集既对同一网络可复现、又对不同网络各不相同）。实际使用的规模范围：
#nodes_min – #nodes_max 个结点、至多 #arcs_max 条弧。

每份查询文件都以 DIMACS 标准的 `.p2p` 格式保存在 `data/queries/<NET>.p2p`
（第一行 `p aux sp p2p 1000`，随后 1000 行 `q s t`），由 `tools/gen_queries.py`
用固定种子生成，因此任何人重跑都能得到完全相同、可直接作为测试输入的点对集合。

== 点对查询（1000 组随机点对）

每组查询在 `/` 终点被弹出（最短距离确定）时立即终止。

#align(center)[#tbl_p2p]
#align(center)[#text(size: 8.5pt)[表 3.2　平均单次点对查询耗时（ms）。每个数据集 1000 组查询，取平均。]]

#fig("/results/figs/fig1_p2p_runtime.png", [点对查询平均耗时随图规模的变化（双对数坐标）。四种实现的曲线几乎平行，说明它们的工作量相同、只差常数因子。])

== 完整单源最短路（16 个随机源点）

这是本项目的*附加实验*：不提前终止，必须确定源点可达的全部结点的最短距离。此时堆的操作次数
达到最大（$n$ 次 extract-min + 至多 $m$ 次 decrease-key），最能反映堆的渐进性质。

#align(center)[#tbl_full]
#align(center)[#text(size: 8.5pt)[表 3.3　平均单次「完整单源最短路」耗时（ms），每个数据集 16 个随机源点取平均。]]

#fig("/results/figs/fig2_full_runtime.png", [完整单源最短路平均耗时随图规模的变化（双对数坐标）。])

== 堆操作与比较次数统计

下表给出 #largest_net（最大的数据集）上四种实现的操作计数。*注意三种可索引堆的 extract-min 次数、
insert 次数、decrease-key 次数、弧扫描次数几乎逐位相同*——这从实验上证明了它们的工作量毫无差别，
耗时差异 100% 来自每次堆操作的开销。

#align(center)[#tbl_ops]
#align(center)[#text(size: 8.5pt)[表 3.4　点对查询模式下的平均操作次数（#largest_net，每次查询）。最后一列为每次「确定一个结点」平均发生的关键字比较次数。]]

#align(center)[#tbl_ops_full]
#align(center)[#text(size: 8.5pt)[表 3.5　完整单源模式下的平均操作次数（#largest_net，每次完整 SSSP）。]]

#fig("/results/figs/fig4_ops.png", [各实现的「每次确定结点比较次数」「每次 extract-min 比较次数」与「每次查询的 decrease-key 次数」。])

== 附加实验：耗时与搜索规模的关系

均匀随机点对的最短路径长度差异极大（#largest_net 上最短的查询只确定约 1.2 千个结点，
最长的要确定 107 万个），对应的搜索规模跨越近三个数量级。
把 1000 组查询按搜索规模分桶后取平均，可以看到一个关键现象：*搜索规模越大，各实现的差距越小*，
说明规模较小时常数因子占主导，而规模很大时渐进性质才开始显形。

#fig("/results/figs/fig5_vs_settled.png", [单次查询耗时与其搜索规模的关系（#largest_net，按被确定结点数分桶取平均）。])

#fig("/results/figs/fig3_relative.png", [各数据集上相对最快实现的耗时倍率（点对查询）。虚线为 1.0，即最快实现。])

#pagebreak()

= 分析与评论

== 实测结论汇总

#table(
  columns: (auto, 1fr),
  align: (left, left),
  stroke: 0.4pt + luma(150),
  strong("观察"), strong("结论"),
  [理论上限], [斐波那契堆 $O(m + n log n)$ < 二叉堆 = 二项堆 $O((n+m) log n)$],
  [实测排名（点对）], [*二叉堆（惰性）< 二叉堆 < 二项堆 ≈ 斐波那契堆*，与理论排名恰好相反；二项堆与斐波那契堆在 7 个数据集上互有胜负，差距都在 6% 以内],
  [相对耗时], [二项堆 = 二叉堆 × #ratio_binom，斐波那契堆 = 二叉堆 × #ratio_fib，惰性二叉堆 = 二叉堆 × #ratio_lazy],
  [工作量], [三种可索引堆的 extract-min / insert / decrease-key / 弧扫描次数几乎完全相同（相对差异 $< 10^(-5)$）；惰性实现的 extract-min 次数多 #lazy_pop_ratio 倍，但确定结点数相同],
  [规模趋势], [规模从 #nodes_min 增长到 #nodes_max（约 #size_ratio 倍）时，曲线近似平行，$log n$ 因子的优势尚未显现],
)

== 为什么斐波那契堆在实践中没有获胜

=== decrease-key 的占比太低

斐波那契堆唯一降阶的操作是 decrease-key，因此它的收益与「decrease-key 次数 / extract-min 次数」
成正比。表 3.4 显示，在公路网上平均每进行 #dec_inv 次 extract-min 才对应 1 次 decrease-key
（每次查询约确定 #settled_per_query 个结点、扫描 #arcs_per_query 条弧，而 decrease-key
只有 extract-min 的 $#dec_per_pop$ 倍）——*decrease-key 在 Dijkstra 中的占比远低于直觉*。
这是因为公路网是稀疏图且平均度很小
（$m/n ≈ 2.5$），每条弧触发一次成功松弛的概率不高（多数顶点在被弹出时其距离已经最优）。

在更极端的场景下这个比例会变化：如果 $m$ 远大于 $n$（例如稠密图或网格图），decrease-key 次数
会接近 $m$，$O(1)$ 摊还的收益才能补偿常数劣势。这是「理论最优 ≠ 实践最优」的第一层原因。

=== 常数因子：`CONSOLIDATE` 与指针追逐

*实现层面*，斐波那契堆/二项堆的每个结点都是指针结构，`extract-min` 需要在根链表上遍历、比较、
搬运指针，缓存局部性远差于二叉堆的连续数组。表 3.4 与图 3.3 给出了直接证据：

+ 每次 extract-min 的关键字比较次数：二叉堆约 #cmp_bin 次，而二项堆约 #cmp_binom 次、
  斐波那契堆约 #cmp_fib 次——*指针式堆反而少做 25% 以上的比较*，这正是它们的理论优势所在
  （根链表只有 $O(log n)$ 个根，而二叉堆每次下沉要比较两个孩子）；
+ 但它们仍然更慢：实测中斐波那契堆耗时约为二叉堆的 #ratio_fib 倍。也就是说，*少做的比较
  被更贵的访存抵消了*——二叉堆的比较是连续内存上的顺序访问，而斐波那契堆的每次比较都伴随
  随机的指针跳转（即常说的「指针追逐，pointer chasing」），实际代价高出一倍以上。

换句话说，$O(m + n log n)$ 相对 $O((n+m) log n)$ 的渐进优势，在这批数据上没有机会补偿常数劣势。

=== 惰性删除为什么最快

不做 decrease-key、松弛成功就 push 一份副本的写法（表 3.2 的最后一列）反而是最快的
（约为二叉堆的 #ratio_lazy 倍）：它把「找到堆中元素并调整位置」这件昂贵的事换成了
「再插入一个副本」，而插入在数组式二叉堆里是极其廉价的操作（均摊 $O(1)$ 的 push_back +
少量比较）。代价是堆中元素数量从 $O(n)$ 涨到 $O(m)$（表 3.4 中每次查询的 extract-min 次数涨到
#lazy_pop_ratio 倍），但对公路网这种 $m/n$ 很小的稀疏图，这点膨胀完全可以接受。这从另一个角度说明：
*在 Dijkstra 中，减少「堆操作的次数」比降低「单词堆操作的渐进代价」更重要。*

== 空间复杂度

设 $n$ 为结点数、$m$ 为弧数，三者都用 CSR 存图（$2n + 2m$ 个 32 位整数）。堆本身：

#table(
  columns: (auto, auto, 1fr),
  align: (left, center, left),
  stroke: 0.4pt + luma(150),
  strong("结构"), strong("空间"), strong("说明"),
  [二叉堆], [$O(n)$], [`key[n] + pos[n] + heap[n]`，最紧凑],
  [二项堆], [$O(n)$], [每个结点 4 个指针 + key + degree，常数中等],
  [斐波那契堆], [$O(n)$], [每个结点 4 个指针 + key + degree + mark，常数最大（约 48–64 字节/结点）],
  [二叉堆（惰性）], [$O(m)$], [最坏情况下堆中存有 $O(m)$ 个副本，空间最差],
)

在 #nodes_max 个结点的图上，斐波那契堆的结点池约 50–60 字节/结点（总计约 50–110 MB），
而二叉堆的三个数组合计约 24 字节/结点（约 15–45 MB）——
这也是斐波那契堆「用空间换渐进时间」的一种体现。

== 与其它数据结构 / 算法的比较

+ *多叉堆（d-ary heap）*：把每个结点 $d$ 个孩子，decrease-key 降到 $O(log_d n)$、extract-min 升到
  $O(d log_d n)$。对 Dijkstra 而言 decrease-key 次数少于 extract-min 次数，取 $d = 4$ 通常比二叉堆快
  10–20%，是实践中比斐波那契堆更划算的改进；
+ *Pairing heap（配对堆）*：与斐波那契堆同样摊还 $O(1)$ 的 insert/decrease-key，但无需 `mark` 位、
  实现更简单、常数更小，实测通常*快于*斐波那契堆，却又常常慢于二叉堆；
+ *Radix heap / bucket heap*：利用整数权重的特点，把 extract-min 降到 $O(log C)$（$C$ 为最大权），
  在公路网上可以显著超过本文的三种堆，是 2000 年后 DIMACS 挑战赛中的主流做法；
+ *算法层面的改进*：可达点集剪枝、双向 Dijkstra、$A^*$ 与预处理（Contraction Hierarchies 等）
  带来的加速（数十倍以上）远大于换堆带来的收益。

== 进一步可能的改进

+ 在现有的二叉堆 / 二项堆 / 斐波那契堆 / 惰性二叉堆之外，再实现 $d$-叉堆与配对堆，
  直接验证「$d$ 取多少最优」以及「配对堆在实践中是否胜过斐波那契堆」；
+ 生成非均匀（同一地区内）的点对查询集合，观察 decrease-key 次数占比提高后各堆的排名是否改变；
+ 把 `.gr` 文件的读入改为内存映射（mmap）并统计真实峰值内存（RSS），补全空间实验；
+ 用「每个顶点的度数加权随机」构造人工稠密图（$m = Theta(n^2)$），验证斐波那契堆在
  $m ≫ n$ 时能否反超二叉堆——这将把「理论最优在什么条件下才体现出来」这一结论做得更 solid。

#pagebreak()

#heading(numbering: none, level: 1)[参考文献]

+ T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, "Introduction to Algorithms", 4th ed.,
  MIT Press, (2022)　（第 6 章二叉堆、第 19 章斐波那契堆、第 22 章 Dijkstra）
+ M. L. Fredman, R. E. Tarjan, "Fibonacci Heaps and Their Uses in Improved Network Optimization
  Algorithms", Journal of the ACM, 34(3): 596–615, (1987)
+ J. Vuillemin, "A Data Structure for Manipulating Priority Queues", Communications of the ACM,
  21(4): 309–315, (1978)　（二项堆）
+ B. V. Cherkassky, A. V. Goldberg, T. Radzik, "Shortest Paths Algorithms: Theory and Experimental
  Evaluation", Mathematical Programming, 73: 129–174, (1996)
+ C. Demetrescu, A. V. Goldberg, D. S. Johnson, "9th DIMACS Implementation Challenge: Shortest
  Paths", (2006), #link("http://www.diag.uniroma1.it/challenge9/")[http://www.diag.uniroma1.it/challenge9/]
+ E. W. Dijkstra, "A Note on Two Problems in Connexion with Graphs", Numerische Mathematik,
  1: 269–271, (1959)
+ M. Fredman, R. Sedgewick, D. Sleator, R. Tarjan, "The Pairing Heap: A New Form of Self-Adjusting
  Heap", Algorithmica, 1: 111–129, (1986)

#pagebreak()

#heading(numbering: none, level: 1)[Appendix A　源代码]

本项目的完整代码结构如下（全部由 `make` 编译，无第三方依赖）：

#table(
  columns: (auto, 1fr),
  align: (left, left),
  stroke: 0.4pt + luma(150),
  column-gutter: 10pt,
  [`src/heaps.h`], [三种堆 + 惰性二叉堆的实现（可索引、支持 decrease-key）],
  [`src/dijkstra.h`], [Dijkstra 模板（可索引堆版 + 惰性删除版），含完整统计],
  [`src/dimacs.h`], [DIMACS `.gr` / `.p2p` 文件的两遍扫描快速解析与 CSR 构造],
  [`src/bench.cpp`], [性能测试程序：遍历各种堆、输出 CSV 时间表与逐查询明细],
  [`src/selftest.cpp`], [三层正确性测试（手工用例 / 随机对拍 / SPFA 交叉验证）],
  [`tools/`], [下载与解压、查询生成、并行跑测试、合并结果、绘图、生成表格],
)

#show heading.where(level: 2): set text(size: 11pt)

#heading(numbering: none, level: 2)[A.1　`src/heaps.h`（三种堆的实现，注释行占比 > 30%）]

#codefile("/src/heaps.h")

#pagebreak()

#heading(numbering: none, level: 2)[A.2　`src/dijkstra.h`（Dijkstra 框架）]

#codefile("/src/dijkstra.h")

#pagebreak()

#heading(numbering: none, level: 2)[A.3　`src/dimacs.h`（图与查询的读取）]

#codefile("/src/dimacs.h")

#pagebreak()

#heading(numbering: none, level: 2)[A.4　`src/bench.cpp`（性能测试程序）]

#codefile("/src/bench.cpp")

#pagebreak()

#heading(numbering: none, level: 2)[A.5　`src/selftest.cpp`（正确性测试）]

#codefile("/src/selftest.cpp")

#pagebreak()

#heading(numbering: none, level: 1)[Appendix B　正确性测试完整输出]

#heading(numbering: none, level: 2)[B.1　单元测试与随机对拍（`build/selftest.exe`）]

#pseudo(read("/results/logs/selftest.log"))

#v(0.4em)
#heading(numbering: none, level: 2)[B.2　逐查询答案交叉验证（`tools/verify_answers.py`）]

下面是对全部数据集、全部实现的逐查询比对结果：同一组 $(s, t)$ 下四种实现返回的 $"dist"(t)$
必须完全相同。`full` 模式下的整体距离校验和也一并逐位核对。

#pseudo(read("/results/logs/verify.log"))

#pagebreak()

#heading(numbering: none, level: 1)[作者分工与声明]

#heading(numbering: none, level: 2)[作者分工]

#table(
  columns: (auto, 1fr, 1fr),
  align: (left, left, left),
  stroke: 0.4pt + luma(150),
  strong("姓名 / 学号"), strong("负责内容"), strong("贡献占比"),
  [【】], [斐波那契堆 / 二项堆实现，摊还分析证明，报告第 2 章], [【】%],
  [【】], [二叉堆与惰性删除实现，CSR 与解析器，测试程序], [【】%],
  [【】], [实验设计与执行、绘图、结果分析，报告第 3、4 章], [【】%],
)

#heading(numbering: none, level: 2)[声明]

We hereby declare that all the work done in this project titled
"Shortest Path Algorithm with Heaps" is of our independent effort as a group.

签名：\
#v(0.9cm)
#grid(columns: (1fr, 1fr, 1fr), align: center,
  [#line(length: 70%) #v(-0.1cm) 第 1 作者],
  [#line(length: 70%) #v(-0.1cm) 第 2 作者],
  [#line(length: 70%) #v(-0.1cm) 第 3 作者])
