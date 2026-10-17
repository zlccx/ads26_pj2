#ifndef BENCH_COMMON_H
#define BENCH_COMMON_H

#include <ctime>
#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include "dijkstra.h"

inline double nowMs() { return 1000.0 * clock() / CLOCKS_PER_SEC; }

// 参数只选择测试模式，输入路径由 Dataset 描述
enum class Mode { Standard, Mini, Full, Custom0, Custom1 };

struct Args {
    Mode mode = Mode::Standard;
};

struct Dataset {
    string graph, queries, net;
};

inline Args parseArgs(int argc, char** argv) {
    Args a;
    if (argc == 1) return a;
    string option = argv[1];
    if (argc == 2 && option == "-mini") a.mode = Mode::Mini;
    else if (argc == 2 && option == "-full") a.mode = Mode::Full;
    else if (argc == 3 && option == "-custom" && string(argv[2]) == "0") a.mode = Mode::Custom0;
    else if (argc == 3 && option == "-custom" && string(argv[2]) == "1") a.mode = Mode::Custom1;
    else {
        fprintf(stderr, "Usage: %s [-mini | -full | -custom 0 | -custom 1]\n", argv[0]);
        exit(2);
    }
    return a;
}

template <typename Runner>
// 对同一输入文件中的全部点对依次运行算法
// 每个查询单独计时，文件解析和控制台输出放在计时之外
// 每轮计数器单独创建，最后汇总到整批统计
// 目标点正式出堆即早停，保留不可达查询的统计
// 所有堆必须接收相同顺序的起终点，才能公平比较
double benchP2P(const char* heapName, const Graph& g, Runner& R,
              const vector<int>& srcs, const vector<int>& dsts,
              const Dataset& data) {
	DijkstraStats st;
	long long unreachable = 0; // 不可达查询仍计入总运行次数
	double totalPureAlgoMs = 0.0; // 单次计时结果累加得到整批算法时间
	for (size_t i = 0; i < srcs.size(); i ++ ) {
		DijkstraStats one; // 每次查询从零统计，避免与上一条混淆
		double q0 = nowMs(); // 查询计时起点不包含输入解析
		Key d = R.run(srcs[i], dsts[i], false, one); // false 表示允许到达目标后提前结束
		double q1 = nowMs(); // 查询返回后立即读取时间
		double dt = q1 - q0; // 统一换算后的毫秒差值
		totalPureAlgoMs += dt;

		if (d >= KEY_INF / 2) {
			unreachable ++ ; // 无穷哨兵仅作为状态，不作为真实路径长度
		}
		st.pops += one.pops; // 累加全部查询的实际出堆次数
		st.pushes += one.pushes; // 惰性重复条目也计入实际入堆次数
		st.decs += one.decs; // 统计已发现顶点的成功距离改善次数
		st.arcScans += one.arcScans; // 扫描边数直接反映搜索工作范围
		st.settled += one.settled; // 正式确定顶点不包括过期队列条目
		st.sumDist += one.sumDist; // 校验和只用于快速比较输出一致性
		st.cmps += one.cmps; // 统一累积由各堆记录的键值比较次数
	}

	double total = totalPureAlgoMs; // 写结果发生在计时结束后
	double avg = total / (double)srcs.size(); // 以全部输入查询数量计算平均时间
	unsigned long long checksum = st.sumDist & ((1ULL << 62) - 1);

	printf( "%s,%d,%lld,%s,p2p,%zu,%.3f,%.6f,%llu,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
	        data.net.c_str(), g.n, g.nArcs(), heapName, srcs.size(), total, avg, checksum,
	        st.pops, st.pushes, st.decs, st.arcScans, st.cmps, st.settled, unreachable);
	fflush(stdout);
	fprintf(stderr, "  [p2p ] %-11s runs=%zu total=%9.2fms avg=%8.5fms settled/q=%lld pops=%lld decs=%lld\n",
	        heapName, srcs.size(), total, avg, st.settled / (long long)srcs.size(), st.pops, st.decs);
    return avg;
}

// 四种堆复用同一测试函数，各自持有搜索状态
struct BenchSummary { string net, heap; double avg; };

template <typename Heap>
void runBench(const char* name, const Graph& g, const vector<int>& srcs,
              const vector<int>& dsts, const Dataset& data, vector<BenchSummary>& summaries) {
    DijkstraRunner<Heap> runner(g);
    double avg = benchP2P(name, g, runner, srcs, dsts, data);
    summaries.push_back({data.net, name, avg});
}

inline void runDataset(const Dataset& data, vector<BenchSummary>& summaries) {
    fprintf(stderr, "[bench] Loading graph %s ...\n", data.graph.c_str());
    double start = nowMs();
    Graph g = loadGraph(data.graph);
    fprintf(stderr, "[bench] Graph loaded: n=%d m=%lld in %.1f ms\n", g.n, g.nArcs(), nowMs() - start);
    vector<int> srcs, dsts;
    if (!loadQueries(data.queries, srcs, dsts)) {
        fprintf(stderr, "[bench] Cannot read queries %s\n", data.queries.c_str());
        exit(1);
    }
    for (size_t i = 0; i < srcs.size(); i++) {
        if (srcs[i] < 1 || srcs[i] > g.n || dsts[i] < 1 || dsts[i] > g.n) {
            fprintf(stderr, "[bench] Invalid point-to-point query at index %zu\n", i);
            exit(1);
        }
    }
    fprintf(stderr, "[bench] Loaded %zu queries from %s\n", srcs.size(), data.queries.c_str());
    runBench<BinaryHeap>("binary", g, srcs, dsts, data, summaries);
    runBench<BinomialHeap>("binomial", g, srcs, dsts, data, summaries);
    runBench<FibonacciHeap>("fibonacci", g, srcs, dsts, data, summaries);
    runBench<LazyBinaryHeap>("binary-lazy", g, srcs, dsts, data, summaries);
}

// 每个阶段保存小型汇总，不保留图；图在单次 runDataset 返回时释放
inline double findAverage(const vector<BenchSummary>& rows, const string& net, const string& heap) {
    for (const auto& row : rows) if (row.net == net && row.heap == heap) return row.avg;
    return -1;
}

inline void printComparison(const vector<BenchSummary>& rows, bool compareWeights) {
    printf("\n=== %s ===\n", compareWeights ? "d/t comparison" : "Heap comparison");
    if (compareWeights) {
        printf("%-8s %-12s %12s %12s %12s\n", "network", "heap", "d_avg_ms", "t_avg_ms", "t/d");
        for (const auto& row : rows) {
            if (row.net.substr(row.net.size() - 2) != "-d") continue;
            string net = row.net.substr(0, row.net.size() - 2);
            double t = findAverage(rows, net + "-t", row.heap);
            if (row.avg > 0 && t >= 0)
                printf("%-8s %-12s %12.6f %12.6f %12.3f\n", net.c_str(), row.heap.c_str(), row.avg, t, t / row.avg);
            else printf("%-8s %-12s %12.6f %12.6f %12s\n", net.c_str(), row.heap.c_str(), row.avg, t, "N/A");
        }
    } else {
        printf("%-8s %-12s %12s %16s\n", "network", "heap", "avg_ms", "ratio_to_binary");
        for (const auto& row : rows) {
            double base = findAverage(rows, row.net, "binary");
            if (base > 0) printf("%-8s %-12s %12.6f %16.3f\n", row.net.c_str(), row.heap.c_str(), row.avg, row.avg / base);
            else printf("%-8s %-12s %12.6f %16s\n", row.net.c_str(), row.heap.c_str(), row.avg, "N/A");
        }
    }
    fflush(stdout);
}

inline int runStage(int count, bool bothWeights, const char* title, bool customQueries = false) {
    const char* nets[] = {"NY", "BAY", "COL", "FLA", "NW", "NE", "CAL", "LKS", "E", "W", "CTR", "USA"};
    vector<BenchSummary> summaries;
    fprintf(stderr, "\n=== %s ===\n", title);
    for (int kindIndex = 0; kindIndex < (bothWeights ? 2 : 1); kindIndex ++ ) {
        char kind = kindIndex == 0 ? 'd' : 't';
        for (int i = 0; i < count; i ++ ) {
            Dataset data;
            data.graph = string("data/assets/") + kind + "/USA-road-" + kind + "." + nets[i] + ".gr";
            data.queries = customQueries ? string("data/queries/custom/") + nets[i] + "_5000.in"
                                         : string("data/queries/") + kind + "/" + nets[i] + ".in";
            data.net = string(nets[i]) + "-" + kind;
            runDataset(data, summaries);
        }
    }
    if (bothWeights) printComparison(summaries, true);
    printComparison(summaries, false);
    return 0;
}

// 默认六张小图比较权重，再独立测试七张距离图；-full 才跑全部图
inline int runSelected(const Args& a) {
    if (a.mode == Mode::Custom0) return runStage(1, false, "custom_5000", true);
    if (a.mode == Mode::Custom1) {
        Dataset data = {"data/queries/custom/dense.in", "data/queries/custom/dense_queries.in", "custom-dense"};
        vector<BenchSummary> summaries;
        runDataset(data, summaries);
        printComparison(summaries, false);
        return 0;
    }
    if (a.mode == Mode::Full) return runStage(12, true, "full");
    runStage(3, true, "small_dt");
    if (a.mode == Mode::Mini) return 0;
    return runStage(7, false, "distance_7");
}

#endif
