// =============================================================================
//  dimacs.h —— DIMACS Challenge 9 图数据 / 查询数据的读取
//
//  文件格式（来自 Challenge 9 官方说明）：
//    .gr  文件：  c 注释行
//                 p sp <n> <m>        结点数、弧数
//                 a <u> <v> <w>       一条从 u 到 v、权为 w 的弧
//    .p2p 文件：  c 注释行
//                 p aux sp p2p <k>    k 组查询
//                 q <s> <t>           源点 s、终点 t
//    .ss  文件：  p aux sp ss <k> / s <s>   单源查询
//
//  实现要点：
//   1) 一次性把整个文件读进内存，再用自写的手工扫描器解析，比 >> 快得多；
//      解析分两遍：第一遍只统计每个结点的出度，第二遍再填写 CSR 数组，
//      因此除了图本身之外不需要 (u,v,w) 三元组的临时存储。
//   2) 图用 CSR（压缩稀疏行）存储：head[] + to[] + w[]，
//      遍历某结点所有出边是连续内存访问，缓存友好，这对 Dijkstra 的实测很关键。
// =============================================================================
#pragma once

#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ----------------------------------------------------------------------------
//  图：CSR 结构
// ----------------------------------------------------------------------------
struct Graph {
    int n = 0;                     // 结点数，编号 1..n
    std::vector<int> head;         // 长度 n+2：head[u]..head[u+1]-1 是 u 的出边区间
    std::vector<int> to;           // 弧头
    std::vector<int> w;            // 弧权

    long long nArcs() const { return (long long)to.size(); }

    // 从边表构造 CSR（测试程序用）
    static Graph fromEdges(int n, const std::vector<int>& eu,
                           const std::vector<int>& ev,
                           const std::vector<int>& ew);
};

inline Graph Graph::fromEdges(int n, const std::vector<int>& eu,
                              const std::vector<int>& ev,
                              const std::vector<int>& ew) {
    Graph g;
    g.n = n;
    g.head.assign((size_t)n + 2, 0);
    const size_t m = eu.size();
    for (size_t i = 0; i < m; ++i) ++g.head[(size_t)eu[i] + 1];
    for (int u = 1; u <= n; ++u) g.head[u + 1] += g.head[u];
    g.to.resize(m);
    g.w.resize(m);
    std::vector<int> cursor(g.head.begin(), g.head.end());
    for (size_t i = 0; i < m; ++i) {
        const int u = eu[i];
        const int p = cursor[u]++;
        g.to[p] = ev[i];
        g.w[p] = ew[i];
    }
    return g;
}

// ----------------------------------------------------------------------------
//  文件读取工具
// ----------------------------------------------------------------------------

// 把整个文件读入内存；失败则返回空
inline std::string readWholeFile(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return std::string();
    std::fseek(f, 0, SEEK_END);
    const long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::string buf;
    if (sz > 0) {
        buf.resize((size_t)sz);
        const size_t got = std::fread(&buf[0], 1, (size_t)sz, f);
        buf.resize(got);
    }
    std::fclose(f);
    return buf;
}

// 手工整数扫描：跳过非数字字符，读一个整数
inline long long scanInt(const char*& p, const char* end) {
    while (p < end && (*p < '0' || *p > '9') && *p != '-') ++p;
    bool neg = false;
    if (p < end && *p == '-') { neg = true; ++p; }
    long long x = 0;
    while (p < end && *p >= '0' && *p <= '9') { x = x * 10 + (*p - '0'); ++p; }
    return neg ? -x : x;
}

// 跳过当前行剩余部分，返回下一行的起点（若已到文件末尾返回 end）
inline const char* nextLine(const char* p, const char* end) {
    while (p < end && *p != '\n') ++p;
    if (p < end) ++p;
    return p;
}

// ----------------------------------------------------------------------------
//  读取 .gr 图文件
// ----------------------------------------------------------------------------
inline Graph loadGraph(const std::string& path, long long* outLoadMs = nullptr) {
    const std::string buf = readWholeFile(path);
    if (buf.empty()) {
        std::fprintf(stderr, "[dimacs] cannot read graph file: %s\n", path.c_str());
        std::exit(1);
    }
    Graph g;
    const char* p = buf.data();
    const char* end = p + buf.size();

    // ---- 第 1 遍：找问题行拿到 n，并统计每个结点的出度 ----
    std::vector<int> deg;
    for (const char* q = p; q < end; ) {
        const char c = *q;
        if (c == 'p') {
            // p sp n m
            const char* r = q + 1;
            g.n = (int)scanInt(r, end);          // 跳过 "sp" 后读到 n
            scanInt(r, end);                     // m（由 to.size() 实际决定）
            deg.assign((size_t)g.n + 2, 0);
            q = nextLine(q, end);
        }
        else if (c == 'a') {
            const char* r = q + 1;
            const int u = (int)scanInt(r, end);
            scanInt(r, end);                     // v，本条不需要
            (void)scanInt(r, end);               // w
            ++deg[(size_t)u + 1];                // 计入 u 的出度
            q = nextLine(q, end);
        }
        else {
            q = nextLine(q, end);
        }
    }
    if (g.n <= 0) {
        std::fprintf(stderr, "[dimacs] bad graph header: %s\n", path.c_str());
        std::exit(1);
    }

    // ---- 前缀和得到 head[] ----
    g.head.assign((size_t)g.n + 2, 0);
    for (int u = 1; u <= g.n; ++u) g.head[u + 1] = g.head[u] + deg[u + 1];
    const long long m = g.head[g.n + 1];
    g.to.resize((size_t)m);
    g.w.resize((size_t)m);
    std::vector<int> cursor(g.head.begin(), g.head.end());

    // ---- 第 2 遍：填 CSR ----
    for (const char* q = p; q < end; ) {
        if (*q == 'a') {
            const char* r = q + 1;
            const int u = (int)scanInt(r, end);
            const int v = (int)scanInt(r, end);
            const int w = (int)scanInt(r, end);
            const int pos = cursor[u]++;
            g.to[pos] = v;
            g.w[pos] = w;
        }
        q = nextLine(q, end);
    }
    (void)outLoadMs;
    return g;
}

// ----------------------------------------------------------------------------
//  读取查询文件（.p2p 或 .ss）
// ----------------------------------------------------------------------------
inline bool loadQueries(const std::string& path, std::vector<int>& srcs,
                        std::vector<int>& dsts) {
    const std::string buf = readWholeFile(path);
    if (buf.empty()) return false;
    const char* p = buf.data();
    const char* end = p + buf.size();
    for (const char* q = p; q < end; ) {
        const char c = *q;
        if (c == 'q') {                        // q s t
            const char* r = q + 1;
            const int s = (int)scanInt(r, end);
            const int t = (int)scanInt(r, end);
            srcs.push_back(s);
            dsts.push_back(t);
        }
        else if (c == 's') {                   // s S（单源）
            const char* r = q + 1;
            const int s = (int)scanInt(r, end);
            srcs.push_back(s);
            dsts.push_back(-1);
        }
        q = nextLine(q, end);
    }
    return !srcs.empty();
}
