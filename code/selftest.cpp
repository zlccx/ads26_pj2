#include <queue>
#include <cstdio>
#include <random>
#include <vector>
#include <utility>
#include <functional>
#include "dijkstra.h"

// STL 最小优先队列作为独立参照，不复用自实现堆或模板搜索循环
// 参照实现直接使用 STL priority_queue，不调用自实现堆
// 队列元素为距离与顶点的二元组，greater 使其成为最小堆
// 使用惰性删除处理改善过的距离，避免另写参考减键结构
// 它只负责提供正确答案，不参与性能结论
vector<Key> reference(const Graph& g, int source) {
    typedef pair<Key, int> Entry;
    priority_queue<Entry, vector<Entry>, greater<Entry> > queue;
    vector<Key> dist(g.n + 1, KEY_INF); // 每次参考查询重新初始化全部距离，避免复用测试对象的清理逻辑
    dist[source] = 0; // 只有源点初始可达，其余点保持无穷
    queue.push(Entry(0, source)); // STL 参照使用惰性入队，不依赖减键接口
    while (!queue.empty()) {
        Entry top = queue.top(); // 取最小候选距离与对应顶点
        queue.pop(); // 每次循环都消费一条候选记录
        Key d = top.first;
        int u = top.second;
        // 同一顶点可能有多条记录，距离不再最优的记录直接丢弃
        if (d != dist[u]) continue;
        for (int i = g.head[u]; i < g.head[u + 1]; i ++ ) {
            int v = g.to[i];
            Key nd = d + g.w[i]; // 按相同非负边权计算参考路径
            if (nd < dist[v]) {
                dist[v] = nd; // 只有严格改善才更新参考答案
                queue.push(Entry(nd, v));
            }
        }
    }
    return dist;
}

// 对每个起点检查全部终点，同时检查同一 runner 多次复用的清理行为
template <typename Heap>
// 同一个测试对象连续运行多个起终点，能覆盖早停后的状态清理
// 对不可达点也逐一核对，要求返回相同的无穷哨兵
// 完整单源核对可达点数和距离校验和
// 发现首个错误立即退出，并打印对应堆与起终点
bool verify(const Graph& g, const char* name) {
    DijkstraRunner<Heap> runner(g);
    for (int s = 1; s <= g.n; s ++ ) {
        vector<Key> expected = reference(g, s); // 一次完整 STL 搜索提供该源点到所有点的正确距离
        for (int t = 1; t <= g.n; t ++ ) {
            DijkstraStats st; // 计数器独立初始化，便于验证完整遍历统计
            Key actual = runner.run(s, t, false, st); // 逐点核对包含源点自身和不可达目标
            if (actual != expected[t]) {
                fprintf(stderr, "%s mismatch: s=%d t=%d expected=%lld actual=%lld\n",
                        name, s, t, expected[t], actual);
                return false;
            }
        }
        // 完整遍历的 settled 数和距离和也应与 STL 参照一致
        DijkstraStats st; // 计数器独立初始化，便于验证完整遍历统计
        runner.run(s, -1, true, st); // 验证统一模板的完整单源分支
        long long reached = 0; // 手动统计 STL 结果中的有限距离点数
        unsigned long long sum = 0; // 手动累积参考距离，核对 runner 的结果摘要
        for (int v = 1; v <= g.n; v ++ ) {
            if (expected[v] != KEY_INF) { reached ++ ; sum += expected[v]; }
        }
        if (st.settled != reached || st.sumDist != sum) return false;
    }
    return true;
}

// 将同一张图交给四个模板实例，参照算法保持独立
// 各堆出现错误时返回失败，不继续输出通过信息
bool checkGraph(const Graph& g) {
    return verify<BinaryHeap>(g, "binary") && verify<BinomialHeap>(g, "binomial")
        && verify<FibonacciHeap>(g, "fibonacci") && verify<LazyBinaryHeap>(g, "lazy");
}

// 检查二项树的父子关系、堆序及每阶孩子恰好一棵的结构
bool checkBinomialTree(const BinomialHeap& heap, const BinomialHeap::Node* x,
                       const BinomialHeap::Node* parent, vector<char>& visited) {
    if (x->v < 1 || (size_t)x->v >= visited.size() || visited[x->v]) return false;
    if (x->parent != parent || (parent && x->key < parent->key)) return false;
    if (heap.handle[x->v] != x) return false;
    visited[x->v] = 1;
    int degree = x->degree - 1;
    for (const BinomialHeap::Node* child = x->child; child; child = child->sibling) {
        if (child->degree != degree-- || !checkBinomialTree(heap, child, x, visited)) return false;
    }
    return degree == -1;
}

bool checkBinomialState(const BinomialHeap& heap, const vector<Key>& expected,
                        const vector<char>& present) {
    vector<char> visited(present.size(), 0);
    int lastDegree = -1, count = 0;
    for (const BinomialHeap::Node* root = heap.head; root; root = root->sibling) {
        if (root->degree <= lastDegree || !checkBinomialTree(heap, root, nullptr, visited)) return false;
        lastDegree = root->degree;
    }
    for (size_t v = 1; v < present.size(); v++) {
        if (visited[v] != present[v] || heap.contains((int)v) != (present[v] != 0)) return false;
        if (present[v]) {
            if (heap.keyOf((int)v) != expected[v]) return false;
            count++;
        }
    }
    return heap.total_size == count && heap.empty() == (count == 0);
}

// 连续插入跨越二的幂边界，再混合插入、减键和出堆，检查进位与数组映射
bool testBinomialOperations() {
    const int n = 128;
    BinomialHeap heap(n);
    vector<Key> expected(n + 1, 0);
    vector<char> present(n + 1, 0);
    for (int v = 1; v <= n; v++) {
        expected[v] = (n - v) / 5; // 包含并列键值
        present[v] = 1;
        heap.insert(v, expected[v]);
        if (!checkBinomialState(heap, expected, present)) return false;
    }
    mt19937 rng(20261018u);
    for (int step = 0; step < 3000; step++) {
        int v = 1 + rng() % n;
        int operation = rng() % 3;
        if (operation == 0 && !present[v]) {
            expected[v] = rng() % 201;
            present[v] = 1;
            heap.insert(v, expected[v]);
        } else if (operation == 1 && present[v]) {
            expected[v] -= 1 + rng() % 8;
            heap.decreaseKey(v, expected[v]);
        } else if (!heap.empty()) {
            Key minimum = KEY_INF;
            for (int u = 1; u <= n; u++) if (present[u] && expected[u] < minimum) minimum = expected[u];
            int u = heap.extractMin();
            if (!present[u] || expected[u] != minimum) return false;
            present[u] = 0;
        }
        if (!checkBinomialState(heap, expected, present)) return false;
    }
    heap.clear();
    present.assign(n + 1, 0);
    if (!checkBinomialState(heap, expected, present)) return false;
    heap.insert(1, 0);
    return heap.extractMin() == 1 && heap.empty();
}

// 自测采用小图，允许枚举所有起终点而不追求运行速度
// 随机图使用固定种子，发现问题后能重新构造同一用例
// 零权边和相同距离用于覆盖不同堆的并列最小值处理
// 自环和平行边不应改变最短路正确性
// 随机图可能不连通，因而同时检查不可达路径
int main() {
    if (!testBinomialOperations()) {
        fprintf(stderr, "Binomial heap operation test failed\n");
        return 1;
    }
    // 固定图覆盖零权边、平行边、相同距离、自环和不可达顶点
    Graph fixed = Graph::fromEdges(7, {1, 1, 1, 2, 2, 3, 4, 5},
        {2, 2, 3, 3, 4, 4, 4, 6}, {9, 0, 1, 1, 2, 0, 0, 3});
    if (!checkGraph(fixed)) return 1;
    // 固定随机种子生成多组非负权图，每组对所有点对进行交叉验证
    mt19937 rng(20261017u); // 固定序列确保随机失败用例能够复现
    for (int test = 0; test < 30; test ++ ) {
        int n = 2 + rng() % 35; // 小规模允许对所有点对穷举验证
        vector<int> from, to, weight;
        int m = rng() % (n * n + 1); // 覆盖空图、稀疏图和较密图
        for (int i = 0; i < m; i ++ ) {
            from.push_back(1 + rng() % n);
            to.push_back(1 + rng() % n);
            weight.push_back(rng() % 100); // 非负权包含零，覆盖并列最短路径
        }
        if (!checkGraph(Graph::fromEdges(n, from, to, weight))) return 1;
    }
    printf("ALL TESTS PASSED: four heaps match STL priority_queue\n");
    return 0;
}
