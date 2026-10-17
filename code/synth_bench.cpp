#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <vector>
#include <deque>
#include <chrono>

using namespace std;

typedef long long Key;

// =============================================================================
//  1. 二叉堆（数组实现 + 索引映射，支持 decrease-key）
// =============================================================================
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

	long long cmps() const {
		return cmp_cnt;
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

// =============================================================================
//  2. 惰性删除二叉堆
// =============================================================================
class LazyBinaryHeap {
public:
	explicit LazyBinaryHeap(int /*n*/ = 0) {}

	void clear() {
		key.clear();
		ver.clear();
	}

	bool empty() const {
		return key.empty();
	}

	void push(int v, Key k) {
		key.push_back(k);
		ver.push_back(v);
		siftUp((int)key.size() - 1);
	}

	int extractMin(Key &outKey) {
		int v = ver[0];
		outKey = key[0];
		Key lk = key.back();
		int lv = ver.back();
		key.pop_back();
		ver.pop_back();
		if (!key.empty()) {
			key[0] = lk;
			ver[0] = lv;
			siftDown(0);
		}
		return v;
	}

	long long cmps() const {
		return cmp_cnt;
	}

private:
	void siftUp(int i) {
		Key k = key[i];
		int v = ver[i];
		while (i > 0) {
			int p = (i - 1) >> 1;
			if (!lessKey(k, key[p])) {
				break;
			} else {
				key[i] = key[p];
				ver[i] = ver[p];
				i = p;
			}
		}
		key[i] = k;
		ver[i] = v;
	}

	void siftDown(int i) {
		int n = (int)key.size();
		Key k = key[i];
		int v = ver[i];
		for (;;) {
			int c = 2 * i + 1;
			if (c >= n) {
				break;
			} else {
				if (c + 1 < n && lessKey(key[c + 1], key[c])) {
					c ++ ;
				}
				if (!lessKey(key[c], k)) {
					break;
				} else {
					key[i] = key[c];
					ver[i] = ver[c];
					i = c;
				}
			}
		}
		key[i] = k;
		ver[i] = v;
	}

	bool lessKey(Key a, Key b) const {
		cmp_cnt ++ ;
		return a < b;
	}

	vector<Key> key;
	vector<int> ver;
	mutable long long cmp_cnt = 0;
};

// =============================================================================
//  3. 二项堆
// =============================================================================
class BinomialHeap {
public:
	struct Node {
		Key key;
		int v;
		int degree;
		Node *parent;
		Node *child;
		Node *sibling;
	};

	explicit BinomialHeap(int n = 0) {
		init(n);
	}

	void init(int n) {
		pool.resize(n + 1);
		handle.assign(n + 1, nullptr);
		in.assign(n + 1, 0);
		touched.clear();
		touched.reserve(n + 1);
		head = nullptr;
		nfree = 0;
		total_size = 0;
		cmp_cnt = 0;
	}

	void clear() {
		for (size_t i = 0; i < touched.size(); i ++ ) {
			int v = touched[i];
			handle[v] = nullptr;
			in[v] = 0;
		}
		touched.clear();
		head = nullptr;
		nfree = 0;
		total_size = 0;
	}

	bool empty() const {
		return head == nullptr;
	}

	bool contains(int v) const {
		return in[v] != 0;
	}

	Key keyOf(int v) const {
		return handle[v]->key;
	}

	long long cmps() const {
		return cmp_cnt;
	}

	void insert(int v, Key k) {
		Node *x = alloc();
		x->key = k;
		x->v = v;
		x->degree = 0;
		x->parent = nullptr;
		x->child = nullptr;
		x->sibling = nullptr;
		handle[v] = x;
		in[v] = 1;
		touched.push_back(v);
		head = unionLists(head, x);
		total_size ++ ;
	}

	void decreaseKey(int v, Key k) {
		Node *x = handle[v];
		x->key = k;
		while (x->parent && lessKey(x->key, x->parent->key)) {
			Node *p = x->parent;
			swap(x->key, p->key);
			swap(x->v, p->v);
			handle[x->v] = x;
			handle[p->v] = p;
			x = p;
		}
	}

	int extractMin() {
		Node *prev = nullptr;
		Node *minNd = nullptr;
		Node *prevMin = nullptr;
		for (Node *x = head; x; prev = x, x = x->sibling) {
			if (!minNd || lessKey(x->key, minNd->key)) {
				minNd = x;
				prevMin = prev;
			}
		}

		if (prevMin) {
			prevMin->sibling = minNd->sibling;
		} else {
			head = minNd->sibling;
		}

		int v = minNd->v;
		handle[v] = nullptr;
		in[v] = 0;
		total_size -- ;

		Node *ch = minNd->child;
		Node *rev = nullptr;
		while (ch) {
			Node *nxt = ch->sibling;
			ch->sibling = rev;
			ch->parent = nullptr;
			rev = ch;
			ch = nxt;
		}

		head = unionLists(head, rev);
		return v;
	}

private:
	Node* alloc() {
		if ((size_t)nfree >= pool.size()) {
			pool.emplace_back();
		}
		return &pool[(size_t)nfree ++ ];
	}

	static Node* mergeRoots(Node *a, Node *b) {
		Node dummy;
		dummy.sibling = nullptr;
		Node *tail = &dummy;
		while (a && b) {
			if (a->degree <= b->degree) {
				tail->sibling = a;
				a = a->sibling;
			} else {
				tail->sibling = b;
				b = b->sibling;
			}
			tail = tail->sibling;
		}
		tail->sibling = a ? a : b;
		return dummy.sibling;
	}

	void link(Node *y, Node *z) {
		y->parent = z;
		y->sibling = z->child;
		z->child = y;
		z->degree ++ ;
	}

	Node* unionLists(Node *a, Node *b) {
		Node *h = mergeRoots(a, b);
		if (!h) {
			return nullptr;
		} else {
			Node *prev = nullptr;
			Node *x = h;
			Node *nx = x->sibling;
			while (nx) {
				if (x->degree != nx->degree || (nx->sibling && nx->sibling->degree == x->degree)) {
					prev = x;
					x = nx;
				} else if (!lessKey(nx->key, x->key)) {
					x->sibling = nx->sibling;
					link(nx, x);
				} else {
					if (prev) {
						prev->sibling = nx;
					} else {
						h = nx;
					}
					link(x, nx);
					x = nx;
				}
				nx = x->sibling;
			}
			return h;
		}
	}

	bool lessKey(Key a, Key b) {
		cmp_cnt ++ ;
		return a < b;
	}

	deque<Node> pool;
	vector<Node*> handle;
	vector<char> in;
	vector<int> touched;
	Node *head = nullptr;
	int nfree = 0;
	int total_size = 0;
	long long cmp_cnt = 0;
};

// =============================================================================
//  4. 斐波那契堆
// =============================================================================
class FibonacciHeap {
public:
	struct Node {
		Key key;
		int v;
		int degree;
		bool mark;
		Node *parent;
		Node *child;
		Node *left;
		Node *right;
	};

	explicit FibonacciHeap(int n = 0) {
		init(n);
	}

	void init(int n) {
		pool.resize(n + 1);
		handle.assign(n + 1, nullptr);
		in.assign(n + 1, 0);
		A.assign(64, nullptr);
		roots.reserve(64);
		used.reserve(64);
		touched.clear();
		touched.reserve(n + 1);
		min_node = nullptr;
		nfree = 0;
		total_size = 0;
		cmp_cnt = 0;
	}

	void clear() {
		for (size_t i = 0; i < touched.size(); i ++ ) {
			int v = touched[i];
			handle[v] = nullptr;
			in[v] = 0;
		}
		touched.clear();
		min_node = nullptr;
		nfree = 0;
		total_size = 0;
		used.clear();
	}

	bool empty() const {
		return min_node == nullptr;
	}

	bool contains(int v) const {
		return in[v] != 0;
	}

	Key keyOf(int v) const {
		return handle[v]->key;
	}

	long long cmps() const {
		return cmp_cnt;
	}

	void insert(int v, Key k) {
		Node *x = alloc();
		x->key = k;
		x->v = v;
		x->degree = 0;
		x->mark = false;
		x->parent = nullptr;
		x->child = nullptr;
		x->left = x;
		x->right = x;

		if (!min_node) {
			min_node = x;
		} else {
			x->left = min_node;
			x->right = min_node->right;
			min_node->right->left = x;
			min_node->right = x;
			if (lessKey(k, min_node->key)) {
				min_node = x;
			}
		}
		handle[v] = x;
		in[v] = 1;
		touched.push_back(v);
		total_size ++ ;
	}

	void decreaseKey(int v, Key k) {
		Node *x = handle[v];
		x->key = k;
		Node *y = x->parent;
		if (y && lessKey(x->key, y->key)) {
			cut(x, y);
			cascadingCut(y);
		}
		if (lessKey(x->key, min_node->key)) {
			min_node = x;
		}
	}

	int extractMin() {
		Node *z = min_node;
		int v = z->v;

		Node *c = z->child;
		if (c) {
			do {
				Node *nxt = c->right;
				c->parent = nullptr;
				c->right = z->right;
				z->right->left = c;
				c->left = z;
				z->right = c;
				c = nxt;
			} while (c != z->child);
			z->child = nullptr;
		}

		z->left->right = z->right;
		z->right->left = z->left;

		if (z == z->right) {
			min_node = nullptr;
		} else {
			min_node = z->right;
			consolidate();
		}

		handle[v] = nullptr;
		in[v] = 0;
		total_size -- ;
		return v;
	}

private:
	Node* alloc() {
		if ((size_t)nfree >= pool.size()) {
			pool.emplace_back();
		}
		return &pool[(size_t)nfree ++ ];
	}

	void cut(Node *x, Node *y) {
		if (x->right == x) {
			y->child = nullptr;
		} else {
			x->right->left = x->left;
			x->left->right = x->right;
			if (y->child == x) {
				y->child = x->right;
			}
		}
		y->degree -- ;
		x->parent = nullptr;
		x->mark = false;

		x->left = min_node->left;
		x->right = min_node;
		min_node->left->right = x;
		min_node->left = x;
	}

	void cascadingCut(Node *y) {
		Node *z = y->parent;
		if (z) {
			if (!y->mark) {
				y->mark = true;
			} else {
				cut(y, z);
				cascadingCut(z);
			}
		}
	}

	void consolidate() {
		roots.clear();
		Node *w = min_node;
		do {
			roots.push_back(w);
			w = w->right;
		} while (w != min_node);

		used.clear();
		for (size_t i = 0; i < roots.size(); i ++ ) {
			Node *x = roots[i];
			int d = x->degree;
			while (d < (int)A.size() && A[d]) {
				Node *y = A[d];
				if (lessKey(y->key, x->key)) {
					swap(x, y);
				}
				linkFib(y, x);
				A[d] = nullptr;
				d ++ ;
			}
			if (d >= (int)A.size()) {
				A.resize(d + 1, nullptr);
			}
			used.push_back(d);
			A[d] = x;
		}

		min_node = nullptr;
		for (size_t i = 0; i < used.size(); i ++ ) {
			int d = used[i];
			Node *nd = A[d];
			if (!nd) {
				continue;
			}
			A[d] = nullptr;
			if (!min_node) {
				min_node = nd;
				nd->left = nd;
				nd->right = nd;
			} else {
				nd->right = min_node->right;
				min_node->right->left = nd;
				nd->left = min_node;
				min_node->right = nd;
				if (lessKey(nd->key, min_node->key)) {
					min_node = nd;
				}
			}
		}
		used.clear();
	}

	void linkFib(Node *y, Node *x) {
		y->left->right = y->right;
		y->right->left = y->left;
		y->parent = x;
		if (!x->child) {
			x->child = y;
			y->left = y;
			y->right = y;
		} else {
			y->right = x->child->right;
			x->child->right->left = y;
			y->left = x->child;
			x->child->right = y;
		}
		x->degree ++ ;
		y->mark = false;
	}

	bool lessKey(Key a, Key b) {
		cmp_cnt ++ ;
		return a < b;
	}

	deque<Node> pool;
	vector<Node*> handle;
	vector<char> in;
	vector<int> touched;
	vector<Node*> A;
	vector<int> used;
	vector<Node*> roots;
	Node *min_node = nullptr;
	int nfree = 0;
	int total_size = 0;
	long long cmp_cnt = 0;
};

// =============================================================================
//  图结构与 Dijkstra Runner
// =============================================================================
struct Graph {
	int n = 0;
	vector<int> head;
	vector<int> to;
	vector<int> w;

	static Graph fromEdges(int n, const vector<int>& eu, const vector<int>& ev, const vector<int>& ew) {
		Graph g;
		g.n = n;
		g.head.assign(n + 2, 0);
		size_t m = eu.size();
		for (size_t i = 0; i < m; i ++ ) {
			g.head[eu[i] + 1] ++ ;
		}
		for (int u = 1; u <= n; u ++ ) {
			g.head[u + 1] += g.head[u];
		}
		g.to.resize(m);
		g.w.resize(m);
		vector<int> cur(g.head.begin(), g.head.end());
		for (size_t i = 0; i < m; i ++ ) {
			int u = eu[i];
			int pos = cur[u] ++ ;
			g.to[pos] = ev[i];
			g.w[pos] = ew[i];
		}
		return g;
	}
};

struct DijkstraStats {
	long long pops = 0;
	long long pushes = 0;
	long long decs = 0;
	long long cmps = 0;
};

template <class Heap>
class DijkstraRunner {
public:
	explicit DijkstraRunner(const Graph& g)
		: g_(g), dist(g.n + 1, 0), done(g.n + 1, 0), heap(g.n) {
		settled.reserve(g.n + 1);
	}

	void run(int s, DijkstraStats& st) {
		long long c0 = heap.cmps();
		heap.clear();
		heap.insert(s, 0);
		dist[s] = 0;
		st.pushes ++ ;

		while (!heap.empty()) {
			int u = heap.extractMin();
			st.pops ++ ;
			done[u] = 1;
			settled.push_back(u);

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
						st.decs ++ ;
					}
				} else {
					dist[v] = nd;
					heap.insert(v, nd);
					st.pushes ++ ;
				}
			}
		}

		for (size_t i = 0; i < settled.size(); i ++ ) {
			done[settled[i]] = 0;
		}
		settled.clear();
		st.cmps += heap.cmps() - c0;
	}

private:
	const Graph& g_;
	vector<Key> dist;
	vector<char> done;
	vector<int> settled;
	Heap heap;
};

class LazyDijkstraRunner {
public:
	explicit LazyDijkstraRunner(const Graph& g)
		: g_(g), dist(g.n + 1, 0), done(g.n + 1, 0), seen(g.n + 1, 0), heap(g.n) {
		settled.reserve(g.n + 1);
		touchedSeen.reserve(g.n + 1);
	}

	void run(int s, DijkstraStats& st) {
		long long c0 = heap.cmps();
		heap.clear();
		heap.push(s, 0);
		dist[s] = 0;
		seen[s] = 1;
		touchedSeen.push_back(s);
		st.pushes ++ ;

		while (!heap.empty()) {
			Key k = 0;
			int u = heap.extractMin(k);
			st.pops ++ ;

			if (done[u]) {
				continue;
			} else if (k != dist[u]) {
				continue;
			} else {
				done[u] = 1;
				settled.push_back(u);

				for (int i = g_.head[u]; i < g_.head[u + 1]; i ++ ) {
					int v = g_.to[i];
					if (done[v]) {
						continue;
					}
					Key nd = k + g_.w[i];
					if (!seen[v] || nd < dist[v]) {
						dist[v] = nd;
						if (!seen[v]) {
							seen[v] = 1;
							touchedSeen.push_back(v);
						}
						heap.push(v, nd);
						st.pushes ++ ;
					}
				}
			}
		}

		for (size_t i = 0; i < settled.size(); i ++ ) {
			done[settled[i]] = 0;
		}
		settled.clear();
		for (size_t i = 0; i < touchedSeen.size(); i ++ ) {
			seen[touchedSeen[i]] = 0;
		}
		touchedSeen.clear();
		st.cmps += heap.cmps() - c0;
	}

private:
	const Graph& g_;
	vector<Key> dist;
	vector<char> done;
	vector<char> seen;
	vector<int> settled;
	vector<int> touchedSeen;
	LazyBinaryHeap heap;
};

// =============================================================================
//  人工三层图构造与控制变量测试
// =============================================================================
Graph buildSyntheticGraph(int k, int r, bool highDecrease) {
	int n = 1 + k + r;
	vector<int> eu, ev, ew;
	size_t m = (size_t)k + (size_t)k * (size_t)r;
	eu.reserve(m);
	ev.reserve(m);
	ew.reserve(m);

	for (int i = 1; i <= k; i ++ ) {
		eu.push_back(1);
		ev.push_back(1 + i);
		ew.push_back(i);
	}

	for (int i = 1; i <= k; i ++ ) {
		int u = 1 + i;
		for (int j = 1; j <= r; j ++ ) {
			int v = 1 + k + j;
			eu.push_back(u);
			ev.push_back(v);
			if (highDecrease) {
				ew.push_back(3 * k - 2 * i + j);
			} else {
				ew.push_back(3 * k + j);
			}
		}
	}
	return Graph::fromEdges(n, eu, ev, ew);
}

template <class Runner>
void benchmarkCase(const char* heapName, const char* caseName,
                   Runner& runner, int iters, FILE* csvOut) {
	DijkstraStats stWarm;
	runner.run(1, stWarm);

	DijkstraStats st;
	auto t0 = chrono::steady_clock::now();
	for (int i = 0; i < iters; i ++ ) {
		runner.run(1, st);
	}
	auto t1 = chrono::steady_clock::now();
	double avgMs = chrono::duration<double, milli>(t1 - t0).count() / iters;
	long long avgDecs = st.decs / iters;
	long long avgPops = st.pops / iters;
	long long avgCmps = st.cmps / iters;

	printf("  %-15s | %-10s | %10.3f ms | %10lld decs | %10lld cmps\n",
	       heapName, caseName, avgMs, avgDecs, avgCmps);

	if (csvOut) {
		fprintf(csvOut, "%s,%s,%.4f,%lld,%lld,%lld\n",
		        heapName, caseName, avgMs, avgDecs, avgPops, avgCmps);
	}
}

int main(int argc, char** argv) {
	int k = 300;
	int r = 3000;
	int iters = 10;
	if (argc >= 3) {
		k = atoi(argv[1]);
		r = atoi(argv[2]);
	}
	if (argc >= 4) {
		iters = atoi(argv[3]);
	}

	printf("========================================================================\n");
	printf("  Experiment 3: Controlled Variable Benchmark (Self-Contained)          \n");
	printf("  k = %d, r = %d, iters = %d, Expected Decs: Case A = 0, Case B = %lld  \n",
	       k, r, iters, (long long)(k - 1) * r);
	printf("========================================================================\n");

	FILE* fp = fopen("results/synth_results.csv", "w");
	if (fp) {
		fprintf(fp, "heap,case,avg_ms,decs,pops,cmps\n");
	}

	Graph gLow = buildSyntheticGraph(k, r, false);
	Graph gHigh = buildSyntheticGraph(k, r, true);

	DijkstraRunner<BinaryHeap> binLow(gLow), binHigh(gHigh);
	DijkstraRunner<BinomialHeap> bnmLow(gLow), bnmHigh(gHigh);
	DijkstraRunner<FibonacciHeap> fibLow(gLow), fibHigh(gHigh);
	LazyDijkstraRunner lazyLow(gLow), lazyHigh(gHigh);

	printf("\n--- Evaluating Case A (Low-Dec: 0 Expected) ---\n");
	benchmarkCase("BinaryHeap", "LowDec", binLow, iters, fp);
	benchmarkCase("LazyBinaryHeap", "LowDec", lazyLow, iters, fp);
	benchmarkCase("BinomialHeap", "LowDec", bnmLow, iters, fp);
	benchmarkCase("FibonacciHeap", "LowDec", fibLow, iters, fp);

	printf("\n--- Evaluating Case B (High-Dec: %lld Expected) ---\n", (long long)(k - 1) * r);
	benchmarkCase("BinaryHeap", "HighDec", binHigh, iters, fp);
	benchmarkCase("LazyBinaryHeap", "HighDec", lazyHigh, iters, fp);
	benchmarkCase("BinomialHeap", "HighDec", bnmHigh, iters, fp);
	benchmarkCase("FibonacciHeap", "HighDec", fibHigh, iters, fp);

	if (fp) {
		fclose(fp);
		printf("\n[Output] Results written to results/synth_results.csv\n");
	}
	return 0;
}
