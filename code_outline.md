# 代码实现要点

## 二项堆插入只处理连续进位

插入单节点树时，只与根链表开头同阶的树合并，每次合并后阶数加一；遇到不同阶的根就停止，将剩余根链表直接接回。一次插入最坏为 O(log N)，以根数作为势能可得均摊 O(1)。取出最小元仍使用通用根链表合并。

我们按顶点编号开了一个数组：二叉堆保存堆下标，二项堆和斐波那契堆保存节点指针。减键时查数组即可直接找到对应节点。

## 1. Args 统一管理参数与测试模式

`code/bench_common.h` 用一个枚举表示互斥的五种测试模式。Args 只保存用户选择的模式；Dataset 保存内部确定的图路径、查询路径和标签。固定比较四种堆并输出控制台，不提供路径覆盖、堆筛选或文件导出参数。

```cpp
enum class Mode { Standard, Mini, Full, Custom0, Custom1 };
struct Args {
    Mode mode = Mode::Standard;
};
struct Dataset {
    string graph, queries, net;
};

Args parseArgs(int argc, char** argv) { ... }
void runDataset(const Dataset& data, vector<BenchSummary>& summaries) { ... }
int runStage(int count, bool bothWeights, const char* title,
             bool customQueries = false) { ... }

int runSelected(const Args& a) {
    if (a.mode == Mode::Custom0) return runStage(1, false, "custom_5000", true);
    if (a.mode == Mode::Custom1) { ... }
    if (a.mode == Mode::Full) return runStage(12, true, "full");
    runStage(3, true, "small_dt");
    if (a.mode == Mode::Mini) return 0;
    return runStage(7, false, "distance_7");
}
```

## 2. 独立参照与边界用例验证正确性

`code/selftest.cpp` 使用独立编写的 STL `priority_queue` Dijkstra 作为参照，不复用自实现堆或模板搜索循环。对固定边界图和三十组固定种子随机图，逐项比较四种堆的全点对距离，并检查完整单源的可达点数和距离和。

用例覆盖零权、自环、平行边、并列距离和不可达点；同一 runner 连续回答多个查询，还能检查早停后的状态恢复。另外，二项堆有独立的操作测试：连续插入跨越二的幂边界，再混合插入、减键和出堆，用数组保存预期键值并核对最小值，同时检查树结构、堆序和节点指针数组。最短路参照使用 STL Dijkstra。

```cpp
vector<Key> reference(const Graph& g, int source) {
    typedef pair<Key, int> Entry;
    priority_queue<Entry, vector<Entry>, greater<Entry>> queue;
    ... // 独立执行 Dijkstra，返回该源点到所有点的距离
}

template <typename Heap>
bool verify(const Graph& g, const char* name) {
    DijkstraRunner<Heap> runner(g);
    for (int s = 1; s <= g.n; s++) {
        vector<Key> expected = reference(g, s);
        for (int t = 1; t <= g.n; t++) {
            DijkstraStats st;
            Key actual = runner.run(s, t, false, st);
            ... // actual 与 expected[t] 比较，失败时打印查询信息
        }
        ... // 完整单源的 settled、sumDist 与参照结果比较
    }
    return true;
}

bool checkGraph(const Graph& g) {
    return verify<BinaryHeap>(g, "binary")
        && verify<BinomialHeap>(g, "binomial")
        && verify<FibonacciHeap>(g, "fibonacci")
        && verify<LazyBinaryHeap>(g, "lazy");
}
```

## 3. 统一适配接口与模板搜索引擎

`code/dijkstra.h` 用 `HeapAccess` 适配不同堆的操作：常规堆原地减键，惰性堆以重复插入代替减键。四种堆共用 `DijkstraRunner<Heap>` 的同一份主循环，减少代码差异对比较的影响；模板在编译时选择类型，无需运行时虚函数派发。

```cpp
template <typename Heap>
struct HeapAccess {
    static void insert(Heap& heap, int v, Key k) { ... }
    static void decrease(Heap& heap, int v, Key k) { ... }
    static int pop(Heap& heap, Key& k, const vector<Key>& dist) { ... }
    static const bool lazy = false;
};

template <>
struct HeapAccess<LazyBinaryHeap> {
    static void insert(LazyBinaryHeap& heap, int v, Key k) { ... }
    static void decrease(LazyBinaryHeap& heap, int v, Key k) { ... }
    static int pop(LazyBinaryHeap& heap, Key& k, const vector<Key>& dist) { ... }
    static const bool lazy = true;
};

template <typename Heap>
struct DijkstraRunner {
    DijkstraRunner(const Graph& g) { ... }
    Key run(int s, int t, bool full, DijkstraStats& st) { ... }
    const Graph& g_;
    vector<Key> dist;
    vector<char> done, seen;
    vector<int> touched;
    Heap heap;
};
```

## 4. CSR 图存储与缓冲读取

`code/dijkstra.h` 使用 `head`、`to`、`w` 三个数组组织 CSR 图，同一顶点的出边连续存放，减少逐边节点分配，方便顺序扫描。输入通过 `fread` 整文件读取到缓冲区，再扫描整数；这属于缓冲读取，不是内存映射。

```cpp
struct Graph {
    int n = 0;
    vector<int> head;
    vector<int> to, w;
    static Graph fromEdges(int n, const vector<int>& eu,
                           const vector<int>& ev, const vector<int>& ew) { ... }
    long long nArcs() const { ... }
};

string readWholeFile(const string& path) { ... }
long long scanInt(const char*& p, const char* end) { ... }
Graph loadGraph(const string& path) { ... }

// 顶点 u 的所有出边位于一个连续区间
for (int i = g_.head[u]; i < g_.head[u + 1]; i++) {
    int v = g_.to[i];
    Key nd = dist[u] + g_.w[i];
    ...
}
```

## 5. touched 局部恢复与容量复用

`seen` 区分本轮是否发现，`done` 区分是否已确定最短距离。首次发现时将顶点加入 `touched`，结束时只恢复这些顶点的标记，避免每轮扫描全图。

`touched` 包含早停时已发现但尚未确定的点，不能只清理出堆顶点。`dist` 无需逐轮填充无穷，其有效性由 `seen` 控制；数组标记恢复为 O(本轮发现点数)，堆清理成本另外计算。各堆也复用数组或节点池容量。

```cpp
// DijkstraRunner<Heap>::run 中的状态维护摘要
heap.clear();
touched.push_back(s);
seen[s] = 1;
dist[s] = 0;
...
if (!seen[v]) {
    seen[v] = 1;
    touched.push_back(v);
    dist[v] = nd;
    HeapAccess<Heap>::insert(heap, v, nd);
    ...
}
...
for (int v : touched) {
    done[v] = 0;
    seen[v] = 0;
}
touched.clear();
```

## 6. 稳定地址的节点池

`code/heaps.h` 中，二项堆和斐波那契堆以 `deque<Node>` 管理节点。尾部扩容不会移动已有节点，保证父子、兄弟和循环链表指针有效；查询间重置使用游标，复用已分配空间，减少反复分配和释放。

```cpp
// BinomialHeap、FibonacciHeap 的节点管理摘要
deque<Node> pool;
... // 节点使用游标 nfree、顶点到节点的映射等

Node* alloc() {
    if ((size_t)nfree >= pool.size()) pool.emplace_back();
    return &pool[(size_t)nfree++];
}

void clear() {
    ... // 清理涉及的映射、堆结构及游标，保留池空间
}
```

## 7. 有效出堆早停与失效条目过滤

点对查询只在目标作为有效最小值出堆时结束，首次发现目标时不能立即结束。惰性堆可能保留同一顶点的多个候选条目，搜索循环通过 `done` 和条目距离过滤失效记录，确保不会重复展开。

```cpp
while (!heap.empty()) {
    Key k;
    int u = HeapAccess<Heap>::pop(heap, k, dist);
    st.pops++;
    if (done[u] || k != dist[u]) continue;
    done[u] = 1;
    st.settled++;
    st.sumDist += (unsigned long long)k;
    if (!full && u == t) {
        answer = k;
        break;
    }
    ... // 扫描出边并松弛
}
```

## 8. 耗时与操作计数联合分析

`DijkstraStats` 同时记录搜索规模和堆操作，帮助解释性能差异。`decs` 记录成功改善已有候选距离的次数；惰性堆虽然不调用原地减键，这类改善仍计入 `decs`，同时计入实际入堆次数。比较计数通过查询前后作差取得，避免重复累计。

```cpp
struct DijkstraStats {
    long long pops = 0, pushes = 0, decs = 0;
    long long arcScans = 0, cmps = 0, settled = 0;
    unsigned long long sumDist = 0;
};

// DijkstraRunner<Heap>::run 内
long long c0 = heap.cmps();
...
st.decs++;
if (HeapAccess<Heap>::lazy) st.pushes++;
...
st.cmps += heap.cmps() - c0;
```

## 9. 可复现输入与统一计时口径

查询生成器固定随机种子，同一区域 d/t 使用相同种子，在顶点数一致、标准库环境相同的条件下生成相同点对。各堆使用同一图和同一查询顺序。标准点对查询固定为 1000 条，专用路网查询为 5000 条。

`code/bench_common.h` 在图与查询读取完成后才计时，控制台输出在计时之外。点对模式逐条计时并累加，计时包含搜索调用中的状态恢复；使用 `clock()`，精度与语义以实际平台实现为准。

```cpp
// 查询生成摘要
mt19937 rng(20261017u + networkIndex);
uniform_int_distribution<int> vertex(1, n);
... // 独立抽取起终点，保证起终点不同

// benchP2P 内的计时摘要
double q0 = nowMs();
Key d = R.run(srcs[i], dsts[i], false, one);
double q1 = nowMs();
totalPureAlgoMs += q1 - q0;
... // 累积统计；全部查询完成后输出结果
```

## 10. 可控的高减键数据构造

`data/gen_custom.cpp` 在固定三层拓扑上设置边权，主动制造频繁距离改善。源点到中间点 i 的距离为 i，经该点到目标 j 的候选距离为 B+(k-i)A+j，其中 A=r+1、B=k+1；目标始终在全部中间点之后确定，每轮候选距离下降 A，目标按编号倒序扫描。

默认 k=r=3000，包含 6001 个顶点和 9,003,000 条边，每次搜索有 8,997,000 次成功减键。构造用于检验高减键压力下的表现，不保证某种堆必然胜出。

```cpp
bool roadQueries(const char* net, unsigned seed) { ... }

bool denseGraph(int k, int r, int queries) {
    long long a = r + 1LL, b = k + 1LL;
    ... // 源点到中间点 i 的边权为 i
    for (int i = 1; i <= k; i++) {
        for (int j = r; j >= 1; j--) {
            long long w = b + (k - i) * a + j - i;
            ... // 写出中间点 i 到目标 j 的边
        }
    }
    ... // 写出源点到目标层的查询
}
```
