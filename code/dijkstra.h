#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include "heaps.h"

// 图使用 CSR 邻接表存储，避免每个顶点单独分配容器
// 顶点从一开始编号，head[u] 到 head[u + 1] 是该点的出边区间
// to 和 w 使用相同下标，分别保存终点和非负边权
struct Graph {
	int n = 0; // 顶点总数来自 DIMACS 头部，编号范围为一到 n
	vector<int> head; // CSR 偏移数组，额外保留末尾哨兵
	vector<int> to; // 按起点分组的边终点数组
	vector<int> w; // 与终点数组一一对应的原始边权

	// 从边列表构造 CSR，供小图正确性测试复用
	// 先按起点计数，再作前缀和，最后按游标填充边数组
	static Graph fromEdges(int n, const vector<int>& eu, const vector<int>& ev, const vector<int>& ew) {
		Graph g; // 先统计规模再分配图数组，避免逐边动态扩容
		g.n = n; // 测试图沿用调用者提供的顶点编号空间
		g.head.assign(n + 2, 0); // 为前缀和与末尾出边区间保留额外位置
		size_t m = eu.size(); // 三份边列表使用相同下标描述同一条弧
		for (size_t i = 0; i < m; i ++ ) {
			g.head[eu[i] + 1] ++ ; // 出度记在后一槽位，方便直接求起点偏移
		}
		for (int u = 1; u <= n; u ++ ) {
			g.head[u + 1] += g.head[u]; // 前缀和把出度转换为出边区间边界
		}
		g.to.resize(m); // 精确分配终点空间，后续按游标直接写入
		g.w.resize(m); // 权重与终点使用等长连续存储
		vector<int> cur(g.head.begin(), g.head.end()); // 填充游标单独维护，不能破坏原始区间起点
		for (size_t i = 0; i < m; i ++ ) {
			int u = eu[i];
			int pos = cur[u] ++ ; // 填充当前顶点下一条尚未写入的弧
			g.to[pos] = ev[i]; // 保留边列表中的方向，不擅自补反向边
			g.w[pos] = ew[i]; // 保留输入权重，正确性测试也允许零权边
		}
		return g;
	}

	long long nArcs() const {
		return (long long)to.size();
	}
};

// 路网文本一次读入内存，后续扫描不再进行文件操作
inline string readWholeFile(const string& path) {
    FILE* f = fopen(path.c_str(), "rb"); // 二进制方式避免换行转换影响字节长度
    if (!f) return string();
    fseek(f, 0, SEEK_END);
    long size = ftell(f); // 当前路网文件均小于此平台 long 的可表示上界
    fseek(f, 0, SEEK_SET);
    string buf; // 以连续缓冲区支持后续指针扫描
    if (size > 0) {
        buf.resize((size_t)size); // 先分配读取空间再直接写入字符存储区
        size_t got = fread(&buf[0], 1, (size_t)size, f);
        buf.resize(got); // 按实际读入字节数确定有效文本范围
    }
    fclose(f);
    return buf;
}

// 跳过格式文字和空白后读取整数，读取指针停在整数之后
// 输入遵循 DIMACS 格式，负号处理用于兼容文本中的整数表示
inline long long scanInt(const char*& p, const char* end) {
	while (p < end && (*p < '0' || *p > '9') && *p != '-') {
		p ++ ;
	}
	bool neg = false; // 整数扫描独立处理符号和绝对值
	if (p < end && *p == '-') {
		neg = true;
		p ++ ;
	}
	long long x = 0; // 使用宽整数暂存，避免解析过程过早截断
	while (p < end && *p >= '0' && *p <= '9') {
		x = x * 10 + (*p - '0'); // 按十进制逐位累积当前整数
		p ++ ;
	}
	return neg ? -x : x; // 统一在末尾应用符号
}

// 按整行推进，避免把注释中的字母误认为图记录
// 文件末尾没有换行时也能正常停止
inline const char* nextLine(const char* p, const char* end) {
	while (p < end && *p != '\n') {
		p ++ ;
	}
	if (p < end) {
		p ++ ;
	}
	return p;
}

// 第一遍读取顶点数并统计各点出度，确定数组所需空间
// 第二遍按顶点区间填充边，每条有向弧保留原权重
// 距离图和时间图格式相同，读取逻辑不根据权重类型分叉
inline Graph loadGraph(const string& path) {
	string buf = readWholeFile(path); // 文件读取发生在性能计时开始之前
	if (buf.empty()) {
		fprintf(stderr, "[Error] Cannot read graph file %s\n", path.c_str());
		exit(1);
	}
	Graph g; // 先统计规模再分配图数组，避免逐边动态扩容
	const char* p = buf.data(); // 扫描指针只在缓冲区生命周期内有效
	const char* end = p + buf.size(); // 所有扫描循环均使用末尾边界限制访问

	vector<int> deg; // 第一遍只统计出度，不保存重复边列表
	for (const char* q = p; q < end;) {
		char c = *q; // 按每行首字符区分头部、弧和查询记录
		if (c == 'p') {
			const char* r = q + 1; // 跳过记录类型字母后读取整数
			g.n = (int)scanInt(r, end); // p sp 行的首个整数是顶点数
			scanInt(r, end);
			deg.assign(g.n + 2, 0); // 出度数组沿用与 CSR 相同的编号偏移
			q = nextLine(q, end);
		} else if (c == 'a') {
			const char* r = q + 1; // 跳过记录类型字母后读取整数
			int u = (int)scanInt(r, end); // 当前有向弧的起点
			scanInt(r, end);
			(void)scanInt(r, end);
			deg[u + 1] ++ ; // 只计出边，入边不影响当前顶点的邻接区间
			q = nextLine(q, end);
		} else {
			q = nextLine(q, end);
		}
	}

	g.head.assign(g.n + 2, 0); // 建立每个顶点的出边起止边界
	for (int u = 1; u <= g.n; u ++ ) {
		g.head[u + 1] = g.head[u] + deg[u + 1]; // 累计前面顶点的边数作为下一区间起点
	}
	long long m = g.head[g.n + 1]; // 最终哨兵偏移等于实际读到的弧数
	g.to.resize((size_t)m);
	g.w.resize((size_t)m);
	vector<int> cur(g.head.begin(), g.head.end()); // 填充游标单独维护，不能破坏原始区间起点

	for (const char* q = p; q < end;) {
		if (*q == 'a') {
			const char* r = q + 1; // 跳过记录类型字母后读取整数
			int u = (int)scanInt(r, end); // 当前有向弧的起点
			int v = (int)scanInt(r, end); // 当前有向弧的终点
			int w = (int)scanInt(r, end); // distance 与 travel time 都作为非负整数权读取
			int pos = cur[u] ++ ; // 填充当前顶点下一条尚未写入的弧
			g.to[pos] = v; // 不改变原始有向关系
			g.w[pos] = w; // 不按坐标重新计算权重
		}
		q = nextLine(q, end);
	}
	return g;
}

// q 行表示点对查询，s 行表示单源查询
// 跳过 c 注释行和 p 头部，查询顺序与文件保持一致
// 点对早停与完整单源通过目标值和 full 参数区分
inline bool loadQueries(const string& path, vector<int>& srcs, vector<int>& dsts) {
	string buf = readWholeFile(path); // 文件读取发生在性能计时开始之前
	if (buf.empty()) {
		return false;
	}
	const char* p = buf.data(); // 扫描指针只在缓冲区生命周期内有效
	const char* end = p + buf.size(); // 所有扫描循环均使用末尾边界限制访问
	for (const char* q = p; q < end;) {
		char c = *q; // 按每行首字符区分头部、弧和查询记录
		if (c == 'q') {
			const char* r = q + 1; // 跳过记录类型字母后读取整数
			int s = (int)scanInt(r, end); // 查询起点独立于上一次查询
			int t = (int)scanInt(r, end); // 查询终点正式出堆时允许早停
			srcs.push_back(s); // 起点和终点数组使用相同下标对应一组查询
			dsts.push_back(t);
		} else if (c == 's') {
			const char* r = q + 1; // 跳过记录类型字母后读取整数
			int s = (int)scanInt(r, end); // 查询起点独立于上一次查询
			srcs.push_back(s); // 起点和终点数组使用相同下标对应一组查询
			dsts.push_back(-1); // 单源记录不指定目标，沿用完整遍历约定
		}
		q = nextLine(q, end);
	}
	return !srcs.empty();
}


// 计数对象由调用者持有，允许累加一整批查询的操作次数
// 所有计数从零开始，单次查询和整批查询使用同一种统计结构
// sumDist 累加正式确定的距离，仅用于结果交叉核对
// 统计器不包含文件读取时间，计时区间由 bench 控制
struct DijkstraStats {
    long long pops = 0, pushes = 0, decs = 0, arcScans = 0, cmps = 0, settled = 0;
    unsigned long long sumDist = 0;
};

// 常规堆支持原地减键，适配层只处理堆接口差异
template <typename Heap>
// 普通堆每个未确定顶点最多保留一个有效元素
// 已有候选距离被改善时执行原地减键，不增加入堆次数
struct HeapAccess {
    static void insert(Heap& heap, int v, Key k) { heap.insert(v, k); }
    static void decrease(Heap& heap, int v, Key k) { heap.decreaseKey(v, k); }
    static int pop(Heap& heap, Key& k, const vector<Key>& dist) {
        int u = heap.extractMin();
        k = dist[u];
        return u;
    }
    static const bool lazy = false;
};

// 惰性堆用重复入堆代替原地减键，过期记录由统一搜索循环跳过
template <>
// 模板特化只适配接口，不复制整份 Dijkstra
// 旧键值随出堆返回，搜索层据此识别已经过期的条目
struct HeapAccess<LazyBinaryHeap> {
    static void insert(LazyBinaryHeap& heap, int v, Key k) { heap.push(v, k); }
    static void decrease(LazyBinaryHeap& heap, int v, Key k) { heap.push(v, k); }
    static int pop(LazyBinaryHeap& heap, Key& k, const vector<Key>&) { return heap.extractMin(k); }
    static const bool lazy = true;
};

// 所有自实现堆共用这一份 Dijkstra，避免各测试副本逐渐产生差异
template <typename Heap>
// seen 表示本轮是否发现过顶点，done 表示是否已经正式确定
// dist 保存当前最短候选距离，只有 seen 为真时才在本轮有效
// 早停时堆中可能还保留元素，下次开始先清空堆
// 清理只遍历 touched，包含已发现但尚未确定的顶点
struct DijkstraRunner {
    DijkstraRunner(const Graph& g) : g_(g), dist(g.n + 1, KEY_INF),
        done(g.n + 1, 0), seen(g.n + 1, 0), heap(g.n) {}

    // 每轮开始清空堆，但不重置比较次数的历史累计值
    // 不同查询之间只复用容量，不复用上一次的搜索状态
    // 非负边权保证首次有效出堆的距离就是最短距离
    // 不可达目标返回 KEY_INF，不能用零距离表示失败
    Key run(int s, int t, bool full, DijkstraStats& st,
            const vector<int>* targets = nullptr, vector<Key>* answers = nullptr) {
        long long c0 = heap.cmps(); // 保存比较计数基线，结尾统计本轮增量
        heap.clear(); // 丢弃上次早停后尚未出堆的候选项
        // 首次入堆的点都会记录下来，早停时未弹出的点也必须在结尾清理
        touched.push_back(s); // 源点也必须参加查询末尾的状态恢复
        seen[s] = 1; // 区分有效的零距离和未发现顶点
        dist[s] = 0; // 源点到自身的最短距离恒为零
        HeapAccess<Heap>::insert(heap, s, 0);
        st.pushes ++ ;
        Key answer = KEY_INF; // 默认不可达，只有目标正式确定才写入答案
        while (!heap.empty()) {
            Key k;
            int u = HeapAccess<Heap>::pop(heap, k, dist); // 接口差异限制在适配层，不复制搜索循环
            st.pops ++ ;
            if (done[u] || k != dist[u]) continue; // 惰性堆的重复记录和过期距离都不能重复展开
            done[u] = 1; // 正式确定顶点后，其最短距离不再改变
            st.settled ++ ;
            st.sumDist += (unsigned long long)k;
            // 只有目标点正式出堆才能确定答案，发现目标点时不能提前结束
            if (!full && u == t) { answer = k; break; }
            for (int i = g_.head[u]; i < g_.head[u + 1]; i ++ ) {
                st.arcScans ++ ;
                int v = g_.to[i]; // 只扫描当前顶点在 CSR 中的连续出边
                if (done[v]) continue; // 非负权图中已经确定的顶点无需再次松弛
                Key nd = k + g_.w[i]; // 经当前顶点到邻点的候选路径长度
                if (!seen[v]) {
                    seen[v] = 1; // 首次发现时登记状态，避免依赖旧 dist 值
                    touched.push_back(v);
                    dist[v] = nd;
                    HeapAccess<Heap>::insert(heap, v, nd);
                    st.pushes ++ ;
                } else if (nd < dist[v]) {
                    dist[v] = nd;
                    HeapAccess<Heap>::decrease(heap, v, nd);
                    st.decs ++ ;
                    if (HeapAccess<Heap>::lazy) st.pushes ++ ; // 惰性减键实际新增队列条目，单独累计入堆数
                }
            }
        }
        // 离线同源查询在清理 seen 前读取答案，避免误读上一轮留下的 dist
        // 只有本轮实际发现的目标才可达，不需要每次把整个距离数组清成无穷
        if (targets && answers) {
            answers->clear();
            for (int v : *targets) answers->push_back(seen[v] ? dist[v] : KEY_INF);
        }
        // 保留 dist 供正确性测试检查，只恢复查询状态
        for (int v : touched) { done[v] = 0; seen[v] = 0; }
        touched.clear();
        st.cmps += heap.cmps() - c0; // 比较次数使用差值，避免把前几轮统计重复计入
        return answer;
    }

    const Graph& g_;
    vector<Key> dist;
    vector<char> done, seen;
    vector<int> touched;
    Heap heap;
};

#endif
