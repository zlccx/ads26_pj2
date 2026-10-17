#ifndef BENCH_COMMON_H
#define BENCH_COMMON_H

#include <ctime>
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include <cstdlib>
#include "dijkstra.h"
#include "offline.h"

inline double nowMs() { return 1000.0 * clock() / CLOCKS_PER_SEC; }

// 默认 data 为零，表示测试 USA 路网
// data 为一时选择人工输入，仍使用相同图格式与算法
// 显式指定的 graph 和 p2p 路径优先于默认路径
// 结果按追加模式写出，复现实验前应选用新的输出文件
struct Args {
	bool mini = false; // 只测试最小三个路网的两种权重版本
	int data = 0;
	string graph; // 允许调用者指定任意同格式路网文件
	string net = "?"; // 结果标签用于区分网络和边权版本
	string p2p; // 点对查询文件路径
	string ss; // 可选的单源查询文件路径
	string heaps = "bin,binom,fib,lazy"; // 默认比较四种堆，允许只选部分实现
	string out; // 留空时将结果输出到标准输出
	string dump; // 留空时不额外写逐查询明细
	int fullSources = 0; // 默认不额外执行全图单源遍历
	unsigned seed = 20261017u; // 完整单源采样使用固定基础种子
};

// 解析参数
inline Args parseArgs(int argc, char** argv) {
	Args a; // 先使用默认值，再由命令行逐项覆盖
	for (int i = 1; i < argc; i ++ ) {
		string k = argv[i]; // 当前参数名称，参数值由 next 消费
		auto next = [&]() -> string {
			if (i + 1 < argc) {
				return argv[ ++ i]; // 读取值的同时跳过已消费的命令行位置
			} else {
				return string();
			}
		};
		if (k == "-mini") {
			a.mini = true; // 启用最小三张网络的双权重批量测试
		} else if (k == "--data") {
			string value = next();
			if (value != "0" && value != "1") { fprintf(stderr, "--data must be 0 or 1\n"); exit(2); }
			a.data = atoi(value.c_str());
		} else if (k == "--graph") {
			a.graph = next(); // 显式图路径优先于按数据类型生成的默认路径
		} else if (k == "--net") {
			a.net = next(); // 标签直接写入 CSV 的网络列
		} else if (k == "--p2p") {
			a.p2p = next(); // 点对顺序由输入文件固定
		} else if (k == "--ss") {
			a.ss = next(); // 兼容 DIMACS 单源查询文件
		} else if (k == "--heaps") {
			a.heaps = next(); // 堆列表稍后由完整标识匹配解析
		} else if (k == "--out") {
			a.out = next(); // 输出文件采用追加模式保留先前行
		} else if (k == "--dump") {
			a.dump = next(); // 明细文件名还会追加堆类型后缀
		} else if (k == "--full-sources") {
			a.fullSources = atoi(next().c_str()); // 显式选择额外完整单源的查询数
		} else if (k == "--seed") {
			a.seed = (unsigned)strtoul(next().c_str(), nullptr, 10);
		} else {
			fprintf(stderr, "[bench] Unknown option %s\n", k.c_str());
			exit(2);
		}
	}
	// 两类数据都使用相同格式和相同的模板测试流程
	if (a.graph.empty()) {
		a.graph = a.data == 0 ? "data/assets/d/USA-road-d.NY.gr" : "data/queries/custom/high.in";
	}
	if (a.p2p.empty() && a.ss.empty()) {
		a.p2p = a.data == 0 ? "data/queries/d/NY.in" : "data/queries/custom/high_queries.in";
	}
	if (a.net == "?") a.net = a.data == 0 ? "NY-d" : "custom-high";
	return a;
}

static const char* CSV_HEADER =
    "net,nodes,arcs,heap,mode,runs,total_ms,avg_ms,checksum,"
    "pops,pushes,decrease_keys,arc_scans,cmps,settled,unreachable";

// 按逗号分割堆类型列表，使用完整标识匹配
// 不使用子串查找，避免 bin 与 binom 相互误匹配
inline bool hasHeap(const string& list, const char* name) {
	size_t p = 0; // 从第一个堆标识开始查找
	string nm(name); // 将目标标识转为可直接比较的字符串
	while (p <= list.size()) {
		size_t q = list.find(',', p);
		string tok = list.substr(p, (q == string::npos ? list.size() : q) - p);
		if (tok == nm) {
			return true;
		}
		if (q == string::npos) {
			break;
		}
		p = q + 1;
	}
	return false;
}

// 逐条记录用于后续按搜索规模分析性能
// 距离不可达时导出为负一，避免将无穷哨兵当作真实距离
struct QueryRecord {
	size_t idx;
	int src;
	int dst;
	Key dist;
	double elapsedMs;
	long long settled;
	long long pops;
};

template <typename Runner>
// 对同一输入文件中的全部点对依次运行算法
// 每个查询单独计时，文件解析和 CSV 写出放在计时之外
// 每轮计数器单独创建，最后汇总到整批统计
// 目标点正式出堆即早停，保留不可达查询的统计
// dump 参数非空时另外导出每次查询的耗时与搜索规模
// 所有堆必须接收相同顺序的起终点，才能公平比较
void benchP2P(const char* heapName, const Graph& g, Runner& R,
              const vector<int>& srcs, const vector<int>& dsts,
              const Args& a, FILE* out, const string& dumpPrefix) {
	DijkstraStats st;
	long long unreachable = 0; // 不可达查询仍计入总运行次数
	vector<QueryRecord> records; // 只有需要导出明细时才保留逐条记录
	if (!dumpPrefix.empty()) {
		records.reserve(srcs.size()); // 避免记录过程中频繁重新分配明细数组
	}

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
		if (!dumpPrefix.empty()) {
			records.push_back({i, srcs[i], dsts[i], (d >= KEY_INF / 2 ? -1 : d), dt, one.settled, one.pops});
		}
		st.pops += one.pops; // 累加全部查询的实际出堆次数
		st.pushes += one.pushes; // 惰性重复条目也计入实际入堆次数
		st.decs += one.decs; // 统计已发现顶点的成功距离改善次数
		st.arcScans += one.arcScans; // 扫描边数直接反映搜索工作范围
		st.settled += one.settled; // 正式确定顶点不包括过期队列条目
		st.sumDist += one.sumDist; // 校验和只用于快速比较输出一致性
		st.cmps += one.cmps; // 统一累积由各堆记录的键值比较次数
	}

	if (!dumpPrefix.empty()) {
		string fn = dumpPrefix + "." + heapName + ".csv";
		FILE* dump = fopen(fn.c_str(), "w");
		if (dump) {
			fprintf(dump, "idx,src,dst,dist,ms,settled,pops\n");
			for (const auto& r : records) {
				fprintf(dump, "%zu,%d,%d,%lld,%.6f,%lld,%lld\n",
				        r.idx, r.src, r.dst, (long long)r.dist, r.elapsedMs, r.settled, r.pops);
			}
			fclose(dump);
		}
	}

	double total = totalPureAlgoMs; // 写结果发生在计时结束后
	double avg = total / (double)srcs.size(); // 以全部输入查询数量计算平均时间
	unsigned long long checksum = st.sumDist & ((1ULL << 62) - 1);

	fprintf(out, "%s,%d,%lld,%s,p2p,%zu,%.3f,%.6f,%llu,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
	        a.net.c_str(), g.n, g.nArcs(), heapName, srcs.size(), total, avg, checksum,
	        st.pops, st.pushes, st.decs, st.arcScans, st.cmps, st.settled, unreachable);
	fflush(out);
	fprintf(stderr, "  [p2p ] %-11s runs=%zu total=%9.2fms avg=%8.5fms settled/q=%lld pops=%lld decs=%lld\n",
	        heapName, srcs.size(), total, avg, st.settled / (long long)srcs.size(), st.pops, st.decs);
}

template <typename Runner>
// 完整单源不在某个目标处提前停止
// 对每个源点遍历其可达部分，汇总正式确定的距离
// 所有堆复用同一批固定种子产生的源点
// 计时只包围查询循环，不计文件读取和结果输出
void benchFull(const char* heapName, const Graph& g, Runner& R,
               const vector<int>& sources, const Args& a, FILE* out) {
	DijkstraStats st;
	double t0 = nowMs(); // 开始统计完整单源批次的算法时间
	for (size_t i = 0; i < sources.size(); i ++ ) {
		R.run(sources[i], -1, true, st); // true 禁用早停，遍历该源点的所有可达顶点
	}
	double t1 = nowMs(); // 所有源点处理完成后结束整批计时

	double total = t1 - t0;
	double avg = total / (double)sources.size();
	unsigned long long checksum = st.sumDist & ((1ULL << 62) - 1);

	fprintf(out, "%s,%d,%lld,%s,full,%zu,%.3f,%.6f,%llu,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
	        a.net.c_str(), g.n, g.nArcs(), heapName, sources.size(), total, avg, checksum,
	        st.pops, st.pushes, st.decs, st.arcScans, st.cmps, st.settled, 0LL);
	fflush(out);
	fprintf(stderr, "  [full] %-11s runs=%zu total=%9.2fms avg=%8.5fms settled/run=%lld pops=%lld decs=%lld\n",
	        heapName, sources.size(), total, avg, st.settled / (long long)sources.size(), st.pops, st.decs);
}

// 离线查询使用原有 CSV 列，mode 标为 offline 避免与逐点对混淆
// runs 列仍表示回答的点对数，实际 Dijkstra 次数另行打印
// 平均耗时按输入点对数计算，包含离线排序及答案收集
// 校验和在该模式下累计每条查询答案，可用于四种堆之间核对
// 同源共享一次搜索，因此不为每条查询虚构单独的搜索耗时
// dump 只写原始序号、起终点和答案，按输入顺序导出
// 不可达目标在导出时记为负一，内部仍使用 KEY_INF
// 所有输入路径、堆选择和 mini 调度与普通入口保持一致
template <typename Heap>
void benchOffline(const char* name, const Graph& g, DijkstraRunner<Heap>& runner,
                  const vector<int>& srcs, const vector<int>& dsts,
                  const Args& a, FILE* out, const string& dumpPrefix) {
    // 输入必须是合法点对，单源 s 行没有目标，不能套用分组回答
    for (size_t i = 0; i < srcs.size(); i ++ ) {
        if (srcs[i] < 1 || srcs[i] > g.n || dsts[i] < 1 || dsts[i] > g.n) {
            fprintf(stderr, "[offline] requires valid point-to-point queries\n");
            exit(2);
        }
    }
    double start = nowMs();
    OfflineResult result = solveOffline(runner, srcs, dsts);
    double total = nowMs() - start;
    // 计时结束后再汇总答案并进行文件操作
    long long unreachable = 0;
    unsigned long long checksum = 0;
    for (Key answer : result.answers) {
        if (answer == KEY_INF) unreachable ++ ;
        else checksum += (unsigned long long)answer;
    }
    checksum &= ((1ULL << 62) - 1);
    const DijkstraStats& st = result.stats;
    fprintf(out, "%s,%d,%lld,%s,offline,%zu,%.3f,%.6f,%llu,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
        a.net.c_str(), g.n, g.nArcs(), name, srcs.size(), total, total / srcs.size(), checksum,
        st.pops, st.pushes, st.decs, st.arcScans, st.cmps, st.settled, unreachable);
    fflush(out);
    fprintf(stderr, "[offline] %s queries=%zu dijkstra_runs=%zu total=%.3fms\n",
            name, srcs.size(), result.runs, total);
    if (!dumpPrefix.empty()) {
        string path = dumpPrefix + "." + name + ".csv";
        FILE* dump = fopen(path.c_str(), "w");
        if (!dump) { fprintf(stderr, "Cannot write %s\n", path.c_str()); exit(1); }
        fprintf(dump, "idx,src,dst,dist\n");
        for (size_t i = 0; i < srcs.size(); i ++ ) {
            Key answer = result.answers[i];
            fprintf(dump, "%zu,%d,%d,%lld\n", i, srcs[i], dsts[i], answer == KEY_INF ? -1 : answer);
        }
        fclose(dump);
    }
}

// 堆类型是唯一变化的参数，点对和完整单源测试复用同一入口
template <typename Heap>
// 这一层只根据模板参数创建对应的 runner
// 点对查询和完整单源使用上面的公共测试函数
// 添加新的堆时只需补齐适配接口并增加一次模板调用
void runBench(const char* name, const Graph& g, const vector<int>& srcs,
              const vector<int>& dsts, const vector<int>& sources, const Args& a, FILE* out, bool offline = false) {
    DijkstraRunner<Heap> runner(g); // 每种堆独立拥有状态，图数据只读共享
    if (!srcs.empty()) {
        if (offline) benchOffline(name, g, runner, srcs, dsts, a, out, a.dump);
        else benchP2P(name, g, runner, srcs, dsts, a, out, a.dump);
    }
    if (!sources.empty()) benchFull(name, g, runner, sources, a, out);
}

// 先解析和加载输入，再开始测试，因此磁盘读取不混入算法耗时
// 四种堆顺序接收相同图对象和查询列表
// 读取的图对象在 runner 生命周期内保持有效
// 报告的毫秒时间由 clock 与 CLOCKS_PER_SEC 换算
inline int runDataset(const Args& a, bool offline = false) {

	fprintf(stderr, "[bench] Loading graph %s ...\n", a.graph.c_str());
	double t0 = nowMs(); // 开始统计完整单源批次的算法时间
	Graph g = loadGraph(a.graph); // 建图阶段不计入查询耗时
	double t1 = nowMs(); // 所有源点处理完成后结束整批计时
	fprintf(stderr, "[bench] Graph loaded: n=%d m=%lld in %.1f ms\n", g.n, g.nArcs(), t1 - t0);

	vector<int> srcs, dsts; // 按相同下标保存每条查询的起点和终点
	string qfile = a.p2p.empty() ? a.ss : a.p2p;
	if (!qfile.empty()) {
		if (!loadQueries(qfile, srcs, dsts)) {
			fprintf(stderr, "[bench] Cannot read query file %s\n", qfile.c_str());
			exit(1);
		}
		fprintf(stderr, "[bench] Loaded %zu queries from %s\n", srcs.size(), qfile.c_str());
	}

	vector<int> fullSources; // 批次内所有堆复用同一份源点列表
	if (a.fullSources > 0) {
		mt19937 rng(a.seed ^ 0x9e3779b9u); // 与文件查询生成使用独立随机序列
		uniform_int_distribution<int> U(1, g.n); // 顶点编号在合法范围内均匀抽样
		for (int i = 0; i < a.fullSources; i ++ ) {
			fullSources.push_back(U(rng));
		}
	}

	FILE* out = stdout; // 不指定文件时允许重定向标准输出
	if (!a.out.empty()) {
		bool needHeader = true; // 新文件需要表头，已有非空文件只追加数据行
		if (FILE* probe = fopen(a.out.c_str(), "rb")) {
			fseek(probe, 0, SEEK_END);
			needHeader = (ftell(probe) == 0); // 通过长度判断是否已经存在结果内容
			fclose(probe);
		}
		out = fopen(a.out.c_str(), "a"); // 追加而不覆盖已有实验结果
		if (!out) {
			fprintf(stderr, "[bench] Cannot open --out %s\n", a.out.c_str());
			exit(1);
		}
		if (needHeader) {
			fprintf(out, "%s\n", CSV_HEADER);
		}
	}

	if (hasHeap(a.heaps, "bin")) runBench<BinaryHeap>("binary", g, srcs, dsts, fullSources, a, out, offline);
	if (hasHeap(a.heaps, "binom")) runBench<BinomialHeap>("binomial", g, srcs, dsts, fullSources, a, out, offline);
	if (hasHeap(a.heaps, "fib")) runBench<FibonacciHeap>("fibonacci", g, srcs, dsts, fullSources, a, out, offline);
	if (hasHeap(a.heaps, "lazy")) runBench<LazyBinaryHeap>("binary-lazy", g, srcs, dsts, fullSources, a, out, offline);

	if (out != stdout) {
		fclose(out);
	}
	return 0;
}

// mini 仅控制输入枚举，六张图仍复用完全相同的模板测试逻辑
inline int runSelected(const Args& a, bool offline) {
    if (!a.mini) return runDataset(a, offline);
    if (a.data != 0) {
        fprintf(stderr, "-mini requires --data 0\n");
        return 2;
    }
    const char* nets[] = {"NY", "BAY", "COL"}; // 按顶点规模选取最小三个区域
    const char kinds[] = {'d', 't'}; // 分别测试物理距离和通行时间边权
    for (char kind : kinds) {
        for (const char* net : nets) {
            Args one = a; // 保留堆选择与输出参数，只替换当前数据集
            one.graph = string("data/assets/") + kind + "/USA-road-" + kind + "." + net + ".gr";
            one.p2p = string("data/queries/") + kind + "/" + net + ".in";
            one.net = string(net) + "-" + kind;
            one.ss.clear(); // mini 使用点对文件，不混入外部单源文件
            // 六个数据集的逐查询文件必须分开，避免使用同一前缀覆盖
            if (!a.dump.empty()) one.dump = a.dump + "." + one.net;
            fprintf(stderr, "[mini] %s\n", one.net.c_str());
            int status = runDataset(one, offline);
            if (status) return status;
        }
    }
    return 0;
}

#endif
