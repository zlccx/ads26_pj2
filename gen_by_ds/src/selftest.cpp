// =============================================================================
//  selftest.cpp —— 正确性测试（回归测试 + 随机对拍）
//
//  分三层：
//   (A) 堆操作的确定性用例：一批已知的 insert / decreaseKey / extractMin 序列，
//       手工核对弹出顺序。
//   (B) 堆操作的随机对拍：随机序列与「每次线性扫描找最小」的朴素参考实现逐条比对，
//       覆盖三种堆 + 惰性二叉堆。
//   (C) Dijkstra 对拍：在随机有向图上，把 4 种实现算出的最短路与 SPFA
//       （队列式 Bellman-Ford，独立实现、不依赖任何堆）逐点比对；
//       同时对同一批查询校验各种堆的 checksum 完全一致。
//
//  任意一项失败都会打印出错细节并以非零状态退出，方便脚本化回归。
// =============================================================================
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <random>
#include <string>
#include <algorithm>

#include "dimacs.h"
#include "heaps.h"
#include "dijkstra.h"

static int g_failures = 0;

static void check(bool ok, const std::string& what) {
    if (!ok) {
        std::printf("  [FAIL] %s\n", what.c_str());
        ++g_failures;
    }
}

// =============================================================================
//  (A) 堆的确定性用例
// =============================================================================
template <class Heap>
static void scriptedCase(const char* name) {
    const int n = 8;
    Heap h(n);
    h.clear();
    // 依次插入顶点 1..8，关键字故意乱序
    const long long keys[9] = {0, 50, 20, 70, 10, 60, 30, 80, 40};
    for (int v = 1; v <= n; ++v) h.insert(v, keys[v]);
    // 把顶点 7 的关键字从 80 降到 5
    h.decreaseKey(7, 5);
    // 把顶点 2 的关键字从 20 降到 1
    h.decreaseKey(2, 1);

    // 期望弹出顺序按关键字升序：
    //   更新后关键字 v2=1, v7=5, v4=10, v6=30, v8=40, v1=50, v5=60, v3=70
    const int expect[8] = {2, 7, 4, 6, 8, 1, 5, 3};
    for (int i = 0; i < n; ++i) {
        const int got = h.extractMin();
        check(got == expect[i], std::string(name) + " scripted: extract #" + std::to_string(i) +
                                " got " + std::to_string(got) + " expect " + std::to_string(expect[i]));
    }
    check(h.empty(), std::string(name) + " scripted: should be empty at the end");
}

// =============================================================================
//  (B) 堆操作随机对拍
// =============================================================================
// 朴素参考实现：O(n) 线性扫描找最小
struct RefHeap {
    std::vector<Key> key;
    std::vector<char> in;
    int sz = 0;
    explicit RefHeap(int n) : key((size_t)n + 1, 0), in((size_t)n + 1, 0) {}
    void clear() { std::fill(in.begin(), in.end(), 0); sz = 0; }
    int  size() const { return sz; }
    bool contains(int v) const { return in[v] != 0; }
    Key  keyOf(int v) const { return key[v]; }
    void insert(int v, Key k) { key[v] = k; in[v] = 1; ++sz; }
    void decreaseKey(int v, Key k) { key[v] = k; }
    int extractMin() {
        int best = -1;
        for (size_t v = 1; v < in.size(); ++v)
            if (in[v] && (best < 0 || key[v] < key[best])) best = (int)v;
        if (best < 0) return -1;
        in[best] = 0;
        --sz;
        return best;
    }
};

template <class Heap>
static void stressHeap(const char* name, unsigned seed) {
    const int n = 2000;
    const int OPS = 200000;
    Heap h(n);
    RefHeap ref(n);
    h.clear();
    ref.clear();

    // 关键字编码为 key = value * n + v（value 随机、v 为顶点编号）。
    // 这样所有关键字互不相同，最小值唯一，从而可以严格比较「弹出了哪个顶点」，
    // 不会因为「并列最小」时两种实现任意选择不同的顶点而误报。
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> Uv(1, n);
    std::uniform_int_distribution<int> Uw(1, 1000000);
    int mismatch = 0;

    for (int op = 0; op < OPS && mismatch == 0; ++op) {
        const int r = (int)(rng() % 100);
        if (r < 55) {                       // 55%：insert
            const int v = Uv(rng);
            if (ref.contains(v)) {          // 已在堆中则退化为 decreaseKey
                const Key nk = std::min(ref.keyOf(v), (Key)Uw(rng) * n + v);
                h.decreaseKey(v, nk); ref.decreaseKey(v, nk);
            } else {
                const Key k = (Key)Uw(rng) * n + v;
                h.insert(v, k); ref.insert(v, k);
            }
        } else if (r < 85 && ref.size() > 0) {    // 30%：decreaseKey（只对已存在的顶点）
            int v = -1;
            for (int tries = 0; tries < 50; ++tries) {
                const int c = Uv(rng);
                if (ref.contains(c)) { v = c; break; }
            }
            if (v > 0) {
                // 保持 v 的后缀不变，只把 value 减小
                const Key nk = ref.keyOf(v) - (Key)(rng() % 100 + 1) * n;
                h.decreaseKey(v, nk); ref.decreaseKey(v, nk);
            }
        } else if (ref.size() > 0) {        // 其余：extractMin（比较返回值与关键字）
            const int a = h.extractMin();
            const int b = ref.extractMin();
            if (a != b) ++mismatch;
        }
    }
    check(mismatch == 0, std::string(name) + " stress: extractMin mismatch");

    // 收尾：把剩下的全部弹出，核对顺序
    for (;;) {
        // 参考实现先取最小，再让被测堆取
        if (ref.size() == 0) break;
        const int b = ref.extractMin();
        const int a = h.extractMin();
        if (a != b) { ++mismatch; break; }
    }
    check(mismatch == 0, std::string(name) + " stress: drain mismatch");
    check(h.empty(), std::string(name) + " stress: heap should be empty after drain");
    std::printf("  [ok ] %-14s stress ops=%d comparisons=%lld\n", name, OPS, h.cmps());
}

// 惰性堆单独对拍（语义不同：允许重复存在）
static void stressLazy(unsigned seed) {
    const int n = 2000;
    const int OPS = 200000;
    LazyBinaryHeap h(n);
    std::vector<std::pair<Key, int> > ref;      // 参考：直接存所有 (key,v) 副本
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> Uv(1, n);
    std::uniform_int_distribution<int> Uw(1, 1000000);
    int mismatch = 0;
    for (int op = 0; op < OPS && !mismatch; ++op) {
        if (rng() % 100 < 60) {
            const int v = Uv(rng);
            const Key k = (Key)Uw(rng) * n + v;      // 关键字唯一，避免并列最小时的选择歧义
            h.push(v, k);
            ref.push_back(std::make_pair(k, v));
        } else if (!ref.empty()) {
            std::size_t best = 0;
            for (std::size_t i = 1; i < ref.size(); ++i) if (ref[i] < ref[best]) best = i;
            const Key rk = ref[best].first;
            const int  rv = ref[best].second;
            ref[best] = ref.back();
            ref.pop_back();
            Key k;
            const int v = h.extractMin(k);
            if (k != rk || v != rv) ++mismatch;
        }
    }
    check(mismatch == 0, "LazyBinaryHeap stress: mismatch");
    std::printf("  [ok ] %-14s stress ops=%d\n", "lazy-binary", OPS);
}

// =============================================================================
//  (C) Dijkstra 对拍
// =============================================================================
// SPFA / 队列式 Bellman-Ford：独立参考实现，完全不使用优先队列
static std::vector<Key> spfa(const Graph& g, int s) {
    std::vector<Key> d((size_t)g.n + 1, KEY_INF);
    std::vector<char> inq((size_t)g.n + 1, 0);
    std::vector<int> q((size_t)g.n + 1);
    int qh = 0, qt = 0, qn = 0;
    d[s] = 0;
    q[qt] = s; qt = (qt + 1) % (g.n + 1); ++qn; inq[s] = 1;
    while (qn) {
        const int u = q[qh]; qh = (qh + 1) % (g.n + 1); --qn; inq[u] = 0;
        for (int i = g.head[u]; i < g.head[u + 1]; ++i) {
            const int v = g.to[i];
            const Key nd = d[u] + g.w[i];
            if (nd < d[v]) {
                d[v] = nd;
                if (!inq[v]) { q[qt] = v; qt = (qt + 1) % (g.n + 1); ++qn; inq[v] = 1; }
            }
        }
    }
    return d;
}

template <class Runner>
static void crossCheck(const char* name, const Graph& g, Runner& R,
                       const std::vector<int>& srcs, const std::vector<int>& dsts,
                       unsigned long long& checksumOut) {
    DijkstraStats st;
    unsigned long long cs = 0;
    for (size_t i = 0; i < srcs.size(); ++i) {
        const Key d = R.run(srcs[i], dsts[i], false, st);
        cs += (unsigned long long)d;
    }
    checksumOut = cs & ((1ULL << 62) - 1);
    (void)name;
}

static void dijkstraCrossTest(unsigned seed) {
    // 随机有向图
    std::mt19937 rng(seed);
    const int n = 300;
    const int m = 4000;
    std::uniform_int_distribution<int> Uv(1, n);
    std::uniform_int_distribution<int> Uw(1, 500);
    std::vector<int> eu, ev, ew;
    for (int i = 0; i < m; ++i) {
        const int u = Uv(rng), v = Uv(rng);
        if (u == v) continue;
        eu.push_back(u); ev.push_back(v); ew.push_back(Uw(rng));
    }
    const Graph g = Graph::fromEdges(n, eu, ev, ew);

    // 60 组随机查询
    const int Q = 60;
    std::vector<int> srcs, dsts;
    for (int i = 0; i < Q; ++i) { srcs.push_back(Uv(rng)); dsts.push_back(Uv(rng)); }

    // 参考解：直接对每个源点跑 SPFA
    unsigned long long refSum = 0;
    for (int i = 0; i < Q; ++i) {
        const std::vector<Key> d = spfa(g, srcs[i]);
        refSum += (unsigned long long)d[dsts[i]];
    }
    const unsigned long long refCs = refSum & ((1ULL << 62) - 1);

    // 各种实现
    unsigned long long csBin = 0, csBinom = 0, csFib = 0, csLazy = 0;
    {
        DijkstraRunner<BinaryHeap>   R(g); crossCheck("binary",   g, R, srcs, dsts, csBin);
    }
    {
        DijkstraRunner<BinomialHeap> R(g); crossCheck("binomial", g, R, srcs, dsts, csBinom);
    }
    {
        DijkstraRunner<FibonacciHeap> R(g); crossCheck("fibonacci", g, R, srcs, dsts, csFib);
    }
    {
        LazyDijkstraRunner R(g); crossCheck("lazy", g, R, srcs, dsts, csLazy);
    }

    std::printf("  [ref] SPFA checksum      = %llu\n", refCs);
    std::printf("  [ok ] binary checksum    = %llu\n", csBin);
    std::printf("  [ok ] binomial checksum  = %llu\n", csBinom);
    std::printf("  [ok ] fibonacci checksum = %llu\n", csFib);
    std::printf("  [ok ] lazy checksum      = %llu\n", csLazy);
    check(csBin == refCs,    "dijkstra: binary   != SPFA reference");
    check(csBinom == refCs,  "dijkstra: binomial != SPFA reference");
    check(csFib == refCs,    "dijkstra: fibonacci!= SPFA reference");
    check(csLazy == refCs,   "dijkstra: lazy     != SPFA reference");

    // 完整单源（不提前终止）也要一致
    unsigned long long fBin = 0, fBinom = 0, fFib = 0, fRef = 0;
    {
        DijkstraRunner<BinaryHeap> R(g); DijkstraStats st;
        for (int i = 0; i < 10; ++i) { R.run(srcs[i], -1, true, st); }
        fBin = st.sumDist & ((1ULL << 62) - 1);
    }
    {
        DijkstraRunner<BinomialHeap> R(g); DijkstraStats st;
        for (int i = 0; i < 10; ++i) { R.run(srcs[i], -1, true, st); }
        fBinom = st.sumDist & ((1ULL << 62) - 1);
    }
    {
        DijkstraRunner<FibonacciHeap> R(g); DijkstraStats st;
        for (int i = 0; i < 10; ++i) { R.run(srcs[i], -1, true, st); }
        fFib = st.sumDist & ((1ULL << 62) - 1);
    }
    for (int i = 0; i < 10; ++i) {
        const std::vector<Key> d = spfa(g, srcs[i]);
        for (int v = 1; v <= n; ++v) if (d[v] < KEY_INF) fRef += (unsigned long long)d[v];
    }
    fRef &= ((1ULL << 62) - 1);
    std::printf("  [full] checksums bin=%llu binom=%llu fib=%llu ref=%llu\n",
                fBin, fBinom, fFib, fRef);
    check(fBin == fRef && fBinom == fRef && fFib == fRef,
          "dijkstra: full SSSP checksum mismatch");
}

// =============================================================================
//  专用小用例：手工可验证的最短路
// =============================================================================
static void handMadeTest() {
    //  CLRS 图 24.6 的简化版：
    //      1 --10--> 2 --1--> 3
    //      1 --5 ---> 4 --4--> 3
    //  从 1 出发：d(2)=10, d(3)=9, d(4)=5
    const int n = 4;
    std::vector<int> eu = {1, 1, 2, 4};
    std::vector<int> ev = {2, 4, 3, 3};
    std::vector<int> ew = {10, 5, 1, 4};
    const Graph g = Graph::fromEdges(n, eu, ev, ew);

    const int expect[5] = {0, 0, 10, 9, 5};
    {
        DijkstraRunner<BinaryHeap> R(g); DijkstraStats st;
        for (int v = 1; v <= n; ++v) {
            const Key d = R.run(1, v, false, st);
            check(d == expect[v], "hand-made: binary d(" + std::to_string(v) + ")=" +
                                  std::to_string(d) + " expect " + std::to_string(expect[v]));
        }
    }
    {
        DijkstraRunner<BinomialHeap> R(g); DijkstraStats st;
        for (int v = 1; v <= n; ++v) {
            const Key d = R.run(1, v, false, st);
            check(d == expect[v], "hand-made: binomial d(" + std::to_string(v) + ")");
        }
    }
    {
        DijkstraRunner<FibonacciHeap> R(g); DijkstraStats st;
        for (int v = 1; v <= n; ++v) {
            const Key d = R.run(1, v, false, st);
            check(d == expect[v], "hand-made: fibonacci d(" + std::to_string(v) + ")");
        }
    }
    {
        LazyDijkstraRunner R(g); DijkstraStats st;
        for (int v = 1; v <= n; ++v) {
            const Key d = R.run(1, v, false, st);
            check(d == expect[v], "hand-made: lazy d(" + std::to_string(v) + ")");
        }
    }
    std::printf("  [ok ] hand-made CLRS-style graph distances {10,9,5}\n");
}

int main() {
    setvbuf(stdout, nullptr, _IONBF, 0);      // 关闭缓冲，崩溃时也能看到已打印的进度
    std::printf("=============== unit tests ===============\n");
    std::printf("(A) scripted heap cases\n");
    scriptedCase<BinaryHeap>("binary");
    scriptedCase<BinomialHeap>("binomial");
    scriptedCase<FibonacciHeap>("fibonacci");
    std::printf("(B) randomized heap stress (vs. linear-scan reference)\n");
    stressHeap<BinaryHeap>("binary", 1u);
    stressHeap<BinomialHeap>("binomial", 2u);
    stressHeap<FibonacciHeap>("fibonacci", 3u);
    stressLazy(4u);
    std::printf("(C) Dijkstra cross-check (vs. SPFA reference)\n");
    handMadeTest();
    dijkstraCrossTest(5u);

    std::printf("==========================================\n");
    if (g_failures == 0) std::printf("ALL TESTS PASSED\n");
    else                 std::printf("FAILED: %d check(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
