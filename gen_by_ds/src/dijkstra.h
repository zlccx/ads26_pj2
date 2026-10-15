// =============================================================================
//  dijkstra.h —— 以最小优先队列（模板参数）为核心的 Dijkstra 实现
//
//  提供两个「Runner」类，它们各自持有跨 query 复用的缓冲区：
//      DijkstraRunner<Heap>      —— 可索引堆 + decrease-key 的标准写法
//      LazyDijkstraRunner        —— 二叉堆 + 惰性删除的工程写法（对照用）
//
//  支持两种模式：
//      full = false : 点到点查询。当终点被弹出（即已确定最短距离）时立即停止，
//                     这是 DIMACS P2P 问题的标准做法。
//      full = true  : 完整单源最短路，把源点可达的整个集合都扫描完。
//
//  复杂度（n 结点、m 弧）：
//      二叉堆      : O((n + m) log n)
//      二项堆      : O((n + m) log n)      （但 decrease-key 的常数更大）
//      斐波那契堆  : O(m + n log n)
// =============================================================================
#pragma once

#include <vector>
#include <chrono>
#include "dimacs.h"
#include "heaps.h"

// 一次 Dijkstra 调用的统计信息
struct DijkstraStats {
    long long pops     = 0;   // extractMin 次数
    long long pushes   = 0;   // insert 次数
    long long decs     = 0;   // decreaseKey 次数
    long long arcScans = 0;   // 扫描过的弧数（松弛尝试次数）
    long long cmps     = 0;   // 关键字比较次数
    long long settled  = 0;   // 被确定最短距离的结点数
    // 所有被确定结点的距离之和模 2^62：不同堆之间应逐位一致，用于交叉验证
    unsigned long long sumDist = 0;
};

// ---------------------------------------------------------------------------
//  可索引堆 + decrease-key 版本的 Dijkstra
// ---------------------------------------------------------------------------
template <class Heap>
class DijkstraRunner {
public:
    explicit DijkstraRunner(const Graph& g)
        : g_(g), dist_((size_t)g.n + 1, 0), done_((size_t)g.n + 1, 0),
          heap_(g.n) {
        settled_.reserve((size_t)g.n + 1);
    }

    // 运行一次 Dijkstra：返回 dist[t]；full=true 时 t 无意义，返回值为 0
    Key run(int s, int t, bool full, DijkstraStats& st) {
        const long long c0 = heap_.cmps();     // 本此调用的比较次数增量
        heap_.clear();
        heap_.insert(s, 0);
        dist_[s] = 0;
        Key answer = KEY_INF;                 // 若终点不可达则保持为无穷大

        while (!heap_.empty()) {
            const int u = heap_.extractMin();
            ++st.pops;
            done_[u] = 1;
            settled_.push_back(u);
            ++st.settled;
            st.sumDist += (unsigned long long)dist_[u];
            if (!full && u == t) { answer = dist_[u]; break; }

            const Key du = dist_[u];
            for (int i = g_.head[u]; i < g_.head[u + 1]; ++i) {
                ++st.arcScans;
                const int v = g_.to[i];
                if (done_[v]) continue;                 // v 已确定，无需松弛
                const Key nd = du + g_.w[i];
                if (heap_.contains(v)) {
                    if (nd < heap_.keyOf(v)) {          // 松弛成功 -> decrease-key
                        heap_.decreaseKey(v, nd);
                        dist_[v] = nd;
                        ++st.decs;
                    }
                }
                else {
                    dist_[v] = nd;                      // 首次发现 -> insert
                    heap_.insert(v, nd);
                    ++st.pushes;
                }
            }
        }

        // 清理 done_ 标记（只清理本次碰过的结点，O(工作量)）
        for (size_t i = 0; i < settled_.size(); ++i) done_[settled_[i]] = 0;
        settled_.clear();
        st.cmps += heap_.cmps() - c0;
        return answer;
    }

private:
    const Graph& g_;
    std::vector<Key> dist_;
    std::vector<char> done_;
    std::vector<int> settled_;
    Heap heap_;
};

// ---------------------------------------------------------------------------
//  惰性删除版本的 Dijkstra（只搭配 LazyBinaryHeap 使用）
//
//  不做 decrease-key：松弛成功就再 push 一份 (key, v)，弹到陈旧副本便丢弃。
//  堆规模最坏可到 O(m)，但常数因子小，是工程上最常见的写法。
// ---------------------------------------------------------------------------
class LazyDijkstraRunner {
public:
    explicit LazyDijkstraRunner(const Graph& g)
        : g_(g), dist_((size_t)g.n + 1, 0), done_((size_t)g.n + 1, 0),
          seen_((size_t)g.n + 1, 0), heap_(g.n) {}

    Key run(int s, int t, bool full, DijkstraStats& st) {
        const long long c0 = heap_.cmps();     // 本次调用的比较次数增量
        heap_.clear();
        heap_.push(s, 0);
        dist_[s] = 0;
        seen_[s] = 1;
        touchedSeen_.push_back(s);
        Key answer = KEY_INF;                 // 若终点不可达则保持为无穷大

        while (!heap_.empty()) {
            Key k = 0;
            const int u = heap_.extractMin(k);
            ++st.pops;
            if (done_[u]) continue;                 // 陈旧副本，直接丢弃
            if (k != dist_[u]) continue;            // 关键字不是最新值
            done_[u] = 1;
            settled_.push_back(u);
            ++st.settled;
            st.sumDist += (unsigned long long)k;
            if (!full && u == t) { answer = k; break; }

            for (int i = g_.head[u]; i < g_.head[u + 1]; ++i) {
                ++st.arcScans;
                const int v = g_.to[i];
                if (done_[v]) continue;
                const Key nd = k + g_.w[i];
                if (!seen_[v] || nd < dist_[v]) {   // 松弛成功 -> 直接再 push 一份
                    dist_[v] = nd;
                    if (!seen_[v]) { seen_[v] = 1; touchedSeen_.push_back(v); }
                    heap_.push(v, nd);
                    ++st.pushes;
                }
            }
        }

        for (size_t i = 0; i < settled_.size(); ++i) done_[settled_[i]] = 0;
        settled_.clear();
        for (size_t i = 0; i < touchedSeen_.size(); ++i) seen_[touchedSeen_[i]] = 0;
        touchedSeen_.clear();
        st.cmps += heap_.cmps() - c0;
        return answer;
    }

private:
    const Graph& g_;
    std::vector<Key> dist_;
    std::vector<char> done_;
    std::vector<char> seen_;
    std::vector<int> settled_;
    std::vector<int> touchedSeen_;
    LazyBinaryHeap heap_;
};
