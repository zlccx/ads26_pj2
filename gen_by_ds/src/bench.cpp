// =============================================================================
//  bench.cpp —— 性能测试主程序
//
//  用法：
//    bench --graph <file.gr> [--net NAME] [--p2p <file.p2p>|--ss <file.ss>]
//          [--heaps bin,binom,fib,lazy] [--full-sources K] [--seed S]
//          [--out results.csv] [--dump out.txt]
//
//  程序流程：
//    1) 读入 CSR 图（加载时间单独打印，不计入算法计时）；
//    2) 读入查询集合（.p2p 的点对 或 .ss 的单源）；
//    3) 对每一种堆：跑完整个查询集合，统计总时间 / 平均时间 / 校验和 / 堆操作次数；
//    4) 可选：对 K 个随机源点跑「完整单源最短路」（不提前终止），
//       用于观察「必须扫描整个连通分量」时各种堆的差异；
//    5) 结果以 CSV 形式输出（stdout，同时可写入 --out 文件）。
//
//  校验和（checksum）取「所有被确定结点的距离之和」模 2^62，与 DIMACS 的一致性
//  检查思路一致，用来验证不同堆得到的最短路长度完全一致。
// =============================================================================
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <random>
#include <chrono>

#include "dimacs.h"
#include "heaps.h"
#include "dijkstra.h"

static double nowMs() {
    using namespace std::chrono;
    return duration<double, std::milli>(steady_clock::now().time_since_epoch()).count();
}

struct Args {
    std::string graph, net = "?", p2p, ss, heaps = "bin,binom,fib,lazy", out, dump;
    int fullSources = 0;
    unsigned seed = 20241024u;
};

static Args parseArgs(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        const std::string k = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : std::string(); };
        if (k == "--graph")             a.graph = next();
        else if (k == "--net")          a.net = next();
        else if (k == "--p2p")          a.p2p = next();
        else if (k == "--ss")           a.ss = next();
        else if (k == "--heaps")        a.heaps = next();
        else if (k == "--out")          a.out = next();
        else if (k == "--dump")         a.dump = next();
        else if (k == "--full-sources") a.fullSources = std::atoi(next().c_str());
        else if (k == "--seed")         a.seed = (unsigned)std::strtoul(next().c_str(), nullptr, 10);
        else { std::fprintf(stderr, "[bench] unknown option %s\n", k.c_str()); std::exit(2); }
    }
    if (a.graph.empty()) { std::fprintf(stderr, "[bench] --graph required\n"); std::exit(2); }
    return a;
}

static const char* CSV_HEADER =
    "net,nodes,arcs,heap,mode,runs,total_ms,avg_ms,checksum,"
    "pops,pushes,decrease_keys,arc_scans,cmps,settled,unreachable";

// 判断 --heaps 中是否精确包含某个名字（避免 "bin" 匹配到 "binom"）
static bool hasHeap(const std::string& list, const char* name) {
    size_t p = 0;
    const std::string nm(name);
    while (p <= list.size()) {
        const size_t q = list.find(',', p);
        const std::string tok = list.substr(p, (q == std::string::npos ? list.size() : q) - p);
        if (tok == nm) return true;
        if (q == std::string::npos) break;
        p = q + 1;
    }
    return false;
}

// ---------------------------------------------------------------------------
//  点对查询实验：跑完整个查询集合（提前终止）
//
//  dumpPrefix 非空时，把每次查询的明细写成 CSV：
//      <prefix>.<heapName>.csv  内容为 idx,src,dst,dist,settled_ms,settled,pops
//  便于后续按「搜索规模」分桶分析（报告中 runtime vs. 搜索规模的图）。
// ---------------------------------------------------------------------------
template <class Runner>
static void benchP2P(const char* heapName, const Graph& g, Runner& R,
                     const std::vector<int>& srcs, const std::vector<int>& dsts,
                     const Args& a, std::FILE* out, const std::string& dumpPrefix) {
    DijkstraStats st;
    long long unreachable = 0;

    std::FILE* dump = nullptr;
    if (!dumpPrefix.empty()) {
        const std::string fn = dumpPrefix + "." + heapName + ".csv";
        dump = std::fopen(fn.c_str(), "w");
        if (dump) std::fprintf(dump, "idx,src,dst,dist,ms,settled,pops\n");
    }

    const double t0 = nowMs();
    for (size_t i = 0; i < srcs.size(); ++i) {
        DijkstraStats one;
        const double q0 = nowMs();
        const Key d = R.run(srcs[i], dsts[i], false, one);
        const double q1 = nowMs();
        if (d >= KEY_INF / 2) ++unreachable;
        if (dump) {
            std::fprintf(dump, "%zu,%d,%d,%lld,%.6f,%lld,%lld\n",
                         i, srcs[i], dsts[i], (long long)(d >= KEY_INF / 2 ? -1 : d),
                         q1 - q0, one.settled, one.pops);
        }
        st.pops += one.pops; st.pushes += one.pushes; st.decs += one.decs;
        st.arcScans += one.arcScans; st.settled += one.settled; st.sumDist += one.sumDist;
        st.cmps += one.cmps;
    }
    const double t1 = nowMs();
    if (dump) std::fclose(dump);

    const double total = t1 - t0;
    const double avg = total / (double)srcs.size();
    const unsigned long long checksum = st.sumDist & ((1ULL << 62) - 1);

    std::fprintf(out, "%s,%d,%lld,%s,p2p,%zu,%.3f,%.6f,%llu,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
                 a.net.c_str(), g.n, g.nArcs(), heapName, srcs.size(), total, avg, checksum,
                 st.pops, st.pushes, st.decs, st.arcScans, st.cmps, st.settled, unreachable);
    std::fflush(out);
    std::fprintf(stderr, "  [p2p ] %-11s runs=%zu total=%9.2fms avg=%8.5fms "
                         "settled/q=%lld pops=%lld decs=%lld arcs=%lld\n",
                 heapName, srcs.size(), total, avg, st.settled / (long long)srcs.size(),
                 st.pops, st.decs, st.arcScans);
}

// ---------------------------------------------------------------------------
//  完整单源实验：K 个随机源点，不提前终止
// ---------------------------------------------------------------------------
template <class Runner>
static void benchFull(const char* heapName, const Graph& g, Runner& R,
                      const std::vector<int>& sources, const Args& a, std::FILE* out) {
    DijkstraStats st;

    const double t0 = nowMs();
    for (size_t i = 0; i < sources.size(); ++i) R.run(sources[i], -1, true, st);
    const double t1 = nowMs();

    const double total = t1 - t0;
    const double avg = total / (double)sources.size();
    const unsigned long long checksum = st.sumDist & ((1ULL << 62) - 1);

    std::fprintf(out, "%s,%d,%lld,%s,full,%zu,%.3f,%.6f,%llu,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
                 a.net.c_str(), g.n, g.nArcs(), heapName, sources.size(), total, avg, checksum,
                 st.pops, st.pushes, st.decs, st.arcScans, st.cmps, st.settled, 0LL);
    std::fflush(out);
    std::fprintf(stderr, "  [full] %-11s runs=%zu total=%9.2fms avg=%8.5fms "
                         "settled/run=%lld pops=%lld decs=%lld cmps=%lld\n",
                 heapName, sources.size(), total, avg, st.settled / (long long)sources.size(),
                 st.pops, st.decs, st.cmps);
}

int main(int argc, char** argv) {
    const Args a = parseArgs(argc, argv);

    std::fprintf(stderr, "[bench] loading graph %s ...\n", a.graph.c_str());
    const double t0 = nowMs();
    const Graph g = loadGraph(a.graph);
    const double t1 = nowMs();
    std::fprintf(stderr, "[bench] graph loaded: n=%d m=%lld in %.0f ms\n",
                 g.n, g.nArcs(), t1 - t0);

    std::vector<int> srcs, dsts;
    const std::string qfile = a.p2p.empty() ? a.ss : a.p2p;
    if (!qfile.empty()) {
        if (!loadQueries(qfile, srcs, dsts)) {
            std::fprintf(stderr, "[bench] cannot read query file: %s\n", qfile.c_str());
            std::exit(1);
        }
        std::fprintf(stderr, "[bench] loaded %zu queries from %s\n", srcs.size(), qfile.c_str());
    }

    std::vector<int> fullSources;
    if (a.fullSources > 0) {
        std::mt19937 rng(a.seed ^ 0x9e3779b9u);
        std::uniform_int_distribution<int> U(1, g.n);
        for (int i = 0; i < a.fullSources; ++i) fullSources.push_back(U(rng));
    }

    std::FILE* out = stdout;
    if (!a.out.empty()) {
        bool needHeader = true;
        if (std::FILE* probe = std::fopen(a.out.c_str(), "rb")) {   // 已有内容则不再写表头
            std::fseek(probe, 0, SEEK_END);
            needHeader = std::ftell(probe) == 0;
            std::fclose(probe);
        }
        out = std::fopen(a.out.c_str(), "a");
        if (!out) { std::fprintf(stderr, "[bench] cannot open --out\n"); std::exit(1); }
        if (needHeader) std::fprintf(out, "%s\n", CSV_HEADER);
    }
    std::FILE* dumpFile = nullptr;
    (void)dumpFile;

    if (hasHeap(a.heaps, "bin")) {
        DijkstraRunner<BinaryHeap> R(g);
        if (!srcs.empty()) benchP2P("binary", g, R, srcs, dsts, a, out, a.dump);
        if (!fullSources.empty()) benchFull("binary", g, R, fullSources, a, out);
    }
    if (hasHeap(a.heaps, "binom")) {
        DijkstraRunner<BinomialHeap> R(g);
        if (!srcs.empty()) benchP2P("binomial", g, R, srcs, dsts, a, out, a.dump);
        if (!fullSources.empty()) benchFull("binomial", g, R, fullSources, a, out);
    }
    if (hasHeap(a.heaps, "fib")) {
        DijkstraRunner<FibonacciHeap> R(g);
        if (!srcs.empty()) benchP2P("fibonacci", g, R, srcs, dsts, a, out, a.dump);
        if (!fullSources.empty()) benchFull("fibonacci", g, R, fullSources, a, out);
    }
    if (hasHeap(a.heaps, "lazy")) {
        LazyDijkstraRunner R(g);
        if (!srcs.empty()) benchP2P("binary-lazy", g, R, srcs, dsts, a, out, a.dump);
        if (!fullSources.empty()) benchFull("binary-lazy", g, R, fullSources, a, out);
    }

    if (out != stdout) std::fclose(out);
    return 0;
}
