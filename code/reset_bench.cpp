#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <chrono>

using namespace std;

typedef long long Key;
const Key KEY_INF = 1LL << 60;

// 二叉堆
class BinaryHeap {
public:
	explicit BinaryHeap(int n = 0) {
		init(n);
	}

	void init(int n) {
		key.assign(n + 1, 0);
		pos.assign(n + 1, -1);
		heap.clear();
		touched.clear();
		heap.reserve(n + 1);
		touched.reserve(n + 1);
		cmp_cnt = 0;
	}

	void clear() {
		for (size_t i = 0; i < touched.size(); i ++ ) {
			pos[touched[i]] = -1;
		}
		touched.clear();
		heap.clear();
	}

	bool empty() const {
		return heap.empty();
	}

	bool contains(int v) const {
		return pos[v] >= 0;
	}

	Key keyOf(int v) const {
		return key[v];
	}

	void insert(int v, Key k) {
		key[v] = k;
		pos[v] = (int)heap.size();
		heap.push_back(v);
		touched.push_back(v);
		siftUp((int)heap.size() - 1);
	}

	void decreaseKey(int v, Key k) {
		key[v] = k;
		siftUp(pos[v]);
	}

	int extractMin() {
		int top = heap[0];
		int last = heap.back();
		heap.pop_back();
		pos[top] = -1;
		if (!heap.empty()) {
			heap[0] = last;
			pos[last] = 0;
			siftDown(0);
		}
		return top;
	}

private:
	void siftUp(int i) {
		int v = heap[i];
		Key k = key[v];
		while (i > 0) {
			int p = (i - 1) >> 1;
			int pv = heap[p];
			if (!lessKey(k, key[pv])) {
				break;
			} else {
				heap[i] = pv;
				pos[pv] = i;
				i = p;
			}
		}
		heap[i] = v;
		pos[v] = i;
	}

	void siftDown(int i) {
		int n = (int)heap.size();
		int v = heap[i];
		Key k = key[v];
		for (;;) {
			int c = 2 * i + 1;
			if (c >= n) {
				break;
			} else {
				if (c + 1 < n && lessKey(key[heap[c + 1]], key[heap[c]])) {
					c ++ ;
				}
				if (!lessKey(key[heap[c]], k)) {
					break;
				} else {
					heap[i] = heap[c];
					pos[heap[i]] = i;
					i = c;
				}
			}
		}
		heap[i] = v;
		pos[v] = i;
	}

	bool lessKey(Key a, Key b) {
		cmp_cnt ++ ;
		return a < b;
	}

	vector<Key> key;
	vector<int> pos;
	vector<int> heap;
	vector<int> touched;
	long long cmp_cnt = 0;
};

// CSR 图存储与快速解析
struct Graph {
	int n = 0;
	vector<int> head;
	vector<int> to;
	vector<int> w;
};

string readWholeFile(const string& path) {
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) {
		return string();
	} else {
		fseek(f, 0, SEEK_END);
		long sz = ftell(f);
		fseek(f, 0, SEEK_SET);
		string buf;
		if (sz > 0) {
			buf.resize((size_t)sz);
			size_t got = fread(&buf[0], 1, (size_t)sz, f);
			buf.resize(got);
		}
		fclose(f);
		return buf;
	}
}

long long scanInt(const char*& p, const char* end) {
	while (p < end && (*p < '0' || *p > '9') && *p != '-') {
		p ++ ;
	}
	bool neg = false;
	if (p < end && *p == '-') {
		neg = true;
		p ++ ;
	}
	long long x = 0;
	while (p < end && *p >= '0' && *p <= '9') {
		x = x * 10 + (*p - '0');
		p ++ ;
	}
	return neg ? -x : x;
}

const char* nextLine(const char* p, const char* end) {
	while (p < end && *p != '\n') {
		p ++ ;
	}
	if (p < end) {
		p ++ ;
	}
	return p;
}

Graph loadGraph(const string& path) {
	string buf = readWholeFile(path);
	if (buf.empty()) {
		fprintf(stderr, "[Error] Cannot read graph file %s\n", path.c_str());
		exit(1);
	}
	Graph g;
	const char* p = buf.data();
	const char* end = p + buf.size();

	vector<int> deg;
	for (const char* q = p; q < end;) {
		char c = *q;
		if (c == 'p') {
			const char* r = q + 1;
			g.n = (int)scanInt(r, end);
			scanInt(r, end);
			deg.assign(g.n + 2, 0);
			q = nextLine(q, end);
		} else if (c == 'a') {
			const char* r = q + 1;
			int u = (int)scanInt(r, end);
			scanInt(r, end);
			(void)scanInt(r, end);
			deg[u + 1] ++ ;
			q = nextLine(q, end);
		} else {
			q = nextLine(q, end);
		}
	}

	g.head.assign(g.n + 2, 0);
	for (int u = 1; u <= g.n; u ++ ) {
		g.head[u + 1] = g.head[u] + deg[u + 1];
	}
	long long m = g.head[g.n + 1];
	g.to.resize((size_t)m);
	g.w.resize((size_t)m);
	vector<int> cur(g.head.begin(), g.head.end());

	for (const char* q = p; q < end;) {
		if (*q == 'a') {
			const char* r = q + 1;
			int u = (int)scanInt(r, end);
			int v = (int)scanInt(r, end);
			int w = (int)scanInt(r, end);
			int pos = cur[u] ++ ;
			g.to[pos] = v;
			g.w[pos] = w;
		}
		q = nextLine(q, end);
	}
	return g;
}

bool loadQueries(const string& path, vector<int>& srcs, vector<int>& dsts) {
	string buf = readWholeFile(path);
	if (buf.empty()) {
		return false;
	}
	const char* p = buf.data();
	const char* end = p + buf.size();
	for (const char* q = p; q < end;) {
		char c = *q;
		if (c == 'q') {
			const char* r = q + 1;
			int s = (int)scanInt(r, end);
			int t = (int)scanInt(r, end);
			srcs.push_back(s);
			dsts.push_back(t);
		}
		q = nextLine(q, end);
	}
	return !srcs.empty();
}

class ResetAblationRunner {
public:
	explicit ResetAblationRunner(const Graph& g)
		: g_(g), dist(g.n + 1, 0), done(g.n + 1, 0), heap(g.n) {
		settled.reserve(g.n + 1);
	}

	Key run(int s, int t, bool usePartialReset, int& outSettled) {
		if (usePartialReset) {
			// 由上一轮局部清理维持干净状态
		} else {
			fill(done.begin(), done.end(), (char)0);
		}

		heap.clear();
		heap.insert(s, 0);
		dist[s] = 0;
		settled.clear();
		Key ans = KEY_INF;

		while (!heap.empty()) {
			int u = heap.extractMin();
			done[u] = 1;
			settled.push_back(u);

			if (u == t) {
				ans = dist[u];
				break;
			}

			Key du = dist[u];
			for (int i = g_.head[u]; i < g_.head[u + 1]; i ++ ) {
				int v = g_.to[i];
				if (done[v]) {
					continue;
				}
				Key nd = du + g_.w[i];
				if (heap.contains(v)) {
					if (nd < heap.keyOf(v)) {
						heap.decreaseKey(v, nd);
						dist[v] = nd;
					}
				} else {
					dist[v] = nd;
					heap.insert(v, nd);
				}
			}
		}

		outSettled = (int)settled.size();

		if (usePartialReset) {
			for (size_t i = 0; i < settled.size(); i ++ ) {
				done[settled[i]] = 0;
			}
		} else {
			// 全量重置留待下次开局 fill
		}
		return ans;
	}

private:
	const Graph& g_;
	vector<Key> dist;
	vector<char> done;
	vector<int> settled;
	BinaryHeap heap;
};

int main(int argc, char** argv) {
	string graphPath = "data/USA-road-d.NY.gr";
	string queryPath = "data/queries/NY.p2p";
	if (argc >= 3) {
		graphPath = argv[1];
		queryPath = argv[2];
	}

	printf("========================================================================\n");
	printf("  Experiment 4: Reset Strategy Ablation (Self-Contained)                \n");
	printf("  Graph: %s\n", graphPath.c_str());
	printf("========================================================================\n");

	Graph g = loadGraph(graphPath);
	vector<int> srcs, dsts;
	if (!loadQueries(queryPath, srcs, dsts)) {
		printf("[Error] Cannot load queries from %s\n", queryPath.c_str());
		return 1;
	}

	ResetAblationRunner runner(g);
	vector<int> smallQueries;
	vector<int> largeQueries;

	for (size_t i = 0; i < srcs.size(); i ++ ) {
		int setCnt = 0;
		runner.run(srcs[i], dsts[i], true, setCnt);
		if (setCnt < 5000) {
			smallQueries.push_back((int)i);
		} else if (setCnt > 50000) {
			largeQueries.push_back((int)i);
		}
	}

	printf("Query Classification: Small Scope = %zu, Large Scope = %zu\n\n",
	       smallQueries.size(), largeQueries.size());

	FILE* fp = fopen("results/reset_results.csv", "w");
	if (fp) {
		fprintf(fp, "scope,full_reset_ms,partial_reset_ms,speedup_ratio\n");
	}

	if (!smallQueries.empty()) {
		auto t0 = chrono::steady_clock::now();
		for (int idx : smallQueries) {
			int s = 0;
			runner.run(srcs[idx], dsts[idx], false, s);
		}
		auto t1 = chrono::steady_clock::now();
		double fullMs = chrono::duration<double, milli>(t1 - t0).count() / smallQueries.size();

		t0 = chrono::steady_clock::now();
		for (int idx : smallQueries) {
			int s = 0;
			runner.run(srcs[idx], dsts[idx], true, s);
		}
		t1 = chrono::steady_clock::now();
		double partialMs = chrono::duration<double, milli>(t1 - t0).count() / smallQueries.size();

		double speedup = fullMs / partialMs;
		printf("  Small Search Scope : Full Reset = %7.3f ms | Partial Reset = %7.3f ms | Speedup = %.2fx\n",
		       fullMs, partialMs, speedup);
		if (fp) {
			fprintf(fp, "Small,%.4f,%.4f,%.2f\n", fullMs, partialMs, speedup);
		}
	}

	if (!largeQueries.empty()) {
		auto t0 = chrono::steady_clock::now();
		for (int idx : largeQueries) {
			int s = 0;
			runner.run(srcs[idx], dsts[idx], false, s);
		}
		auto t1 = chrono::steady_clock::now();
		double fullMs = chrono::duration<double, milli>(t1 - t0).count() / largeQueries.size();

		t0 = chrono::steady_clock::now();
		for (int idx : largeQueries) {
			int s = 0;
			runner.run(srcs[idx], dsts[idx], true, s);
		}
		t1 = chrono::steady_clock::now();
		double partialMs = chrono::duration<double, milli>(t1 - t0).count() / largeQueries.size();

		double speedup = fullMs / partialMs;
		printf("  Large Search Scope : Full Reset = %7.3f ms | Partial Reset = %7.3f ms | Speedup = %.2fx\n",
		       fullMs, partialMs, speedup);
		if (fp) {
			fprintf(fp, "Large,%.4f,%.4f,%.2f\n", fullMs, partialMs, speedup);
		}
	}

	if (fp) {
		fclose(fp);
		printf("\n[Output] Results written to results/reset_results.csv\n");
	}
	return 0;
}
