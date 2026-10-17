#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <vector>
#include <deque>
#include <string>
#include <random>
#include <chrono>

using namespace std;

typedef long long Key;
const Key KEY_INF = 1LL << 60;

double nowMs() {
	using namespace chrono;
	return duration<double, milli>(steady_clock::now().time_since_epoch()).count();
}

// =============================================================================
//  1. 二叉堆（数组实现 + 索引映射，支持 O(log n) decrease-key）
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

	int size() const {
		return (int)heap.size();
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
//  2. 二项堆（严格二项树森林，对度数按位合并）
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
//  3. 斐波那契堆（双向循环链表 + 级联剪枝 + 延迟合并）
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
//  4. 惰性删除二叉堆（Lazy Binary Heap）
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
//  CSR 格式图结构与 DIMACS 读取器（纯标准库实现，无外部依赖）
// =============================================================================
struct Graph {
	int n = 0;
	vector<int> head;
	vector<int> to;
	vector<int> w;

	long long nArcs() const {
		return (long long)to.size();
	}
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
		} else if (c == 's') {
			const char* r = q + 1;
			int s = (int)scanInt(r, end);
			srcs.push_back(s);
			dsts.push_back(-1);
		}
		q = nextLine(q, end);
	}
	return !srcs.empty();
}

// =============================================================================
//  Dijkstra 执行引擎与统计
// =============================================================================
struct DijkstraStats {
	long long pops = 0;
	long long pushes = 0;
	long long decs = 0;
	long long arcScans = 0;
	long long cmps = 0;
	long long settled = 0;
	unsigned long long sumDist = 0;
};

template <class Heap>
class DijkstraRunner {
public:
	explicit DijkstraRunner(const Graph& g)
		: g_(g), dist(g.n + 1, 0), done(g.n + 1, 0), heap(g.n) {
		settled.reserve(g.n + 1);
	}

	Key run(int s, int t, bool full, DijkstraStats& st) {
		long long c0 = heap.cmps();
		heap.clear();
		heap.insert(s, 0);
		dist[s] = 0;
		st.pushes ++ ;
		Key ans = KEY_INF;

		while (!heap.empty()) {
			int u = heap.extractMin();
			st.pops ++ ;
			done[u] = 1;
			settled.push_back(u);
			st.settled ++ ;
			st.sumDist += (unsigned long long)dist[u];

			if (!full && u == t) {
				ans = dist[u];
				break;
			}

			Key du = dist[u];
			for (int i = g_.head[u]; i < g_.head[u + 1]; i ++ ) {
				st.arcScans ++ ;
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
		return ans;
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

	Key run(int s, int t, bool full, DijkstraStats& st) {
		long long c0 = heap.cmps();
		heap.clear();
		heap.push(s, 0);
		dist[s] = 0;
		seen[s] = 1;
		touchedSeen.push_back(s);
		st.pushes ++ ;
		Key ans = KEY_INF;

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
				st.settled ++ ;
				st.sumDist += (unsigned long long)k;

				if (!full && u == t) {
					ans = k;
					break;
				}

				for (int i = g_.head[u]; i < g_.head[u + 1]; i ++ ) {
					st.arcScans ++ ;
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
		return ans;
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
//  主程序与命令行参数解析
// =============================================================================
struct Args {
	string graph;
	string net = "?";
	string p2p;
	string ss;
	string heaps = "bin,binom,fib,lazy";
	string out;
	string dump;
	int fullSources = 0;
	unsigned seed = 20261017u;
};

Args parseArgs(int argc, char** argv) {
	Args a;
	for (int i = 1; i < argc; i ++ ) {
		string k = argv[i];
		auto next = [&]() -> string {
			if (i + 1 < argc) {
				return argv[ ++ i];
			} else {
				return string();
			}
		};
		if (k == "--graph") {
			a.graph = next();
		} else if (k == "--net") {
			a.net = next();
		} else if (k == "--p2p") {
			a.p2p = next();
		} else if (k == "--ss") {
			a.ss = next();
		} else if (k == "--heaps") {
			a.heaps = next();
		} else if (k == "--out") {
			a.out = next();
		} else if (k == "--dump") {
			a.dump = next();
		} else if (k == "--full-sources") {
			a.fullSources = atoi(next().c_str());
		} else if (k == "--seed") {
			a.seed = (unsigned)strtoul(next().c_str(), nullptr, 10);
		} else {
			fprintf(stderr, "[bench] Unknown option %s\n", k.c_str());
			exit(2);
		}
	}
	if (a.graph.empty()) {
		fprintf(stderr, "[bench] --graph required\n");
		exit(2);
	}
	return a;
}

const char* CSV_HEADER =
    "net,nodes,arcs,heap,mode,runs,total_ms,avg_ms,checksum,"
    "pops,pushes,decrease_keys,arc_scans,cmps,settled,unreachable";

bool hasHeap(const string& list, const char* name) {
	size_t p = 0;
	string nm(name);
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

struct QueryRecord {
	size_t idx;
	int src;
	int dst;
	Key dist;
	double elapsedMs;
	long long settled;
	long long pops;
};

template <class Runner>
void benchP2P(const char* heapName, const Graph& g, Runner& R,
              const vector<int>& srcs, const vector<int>& dsts,
              const Args& a, FILE* out, const string& dumpPrefix) {
	DijkstraStats st;
	long long unreachable = 0;
	vector<QueryRecord> records;
	if (!dumpPrefix.empty()) {
		records.reserve(srcs.size());
	}

	double totalPureAlgoMs = 0.0;
	for (size_t i = 0; i < srcs.size(); i ++ ) {
		DijkstraStats one;
		double q0 = nowMs();
		Key d = R.run(srcs[i], dsts[i], false, one);
		double q1 = nowMs();
		double dt = q1 - q0;
		totalPureAlgoMs += dt;

		if (d >= KEY_INF / 2) {
			unreachable ++ ;
		}
		if (!dumpPrefix.empty()) {
			records.push_back({i, srcs[i], dsts[i], (d >= KEY_INF / 2 ? -1 : d), dt, one.settled, one.pops});
		}
		st.pops += one.pops;
		st.pushes += one.pushes;
		st.decs += one.decs;
		st.arcScans += one.arcScans;
		st.settled += one.settled;
		st.sumDist += one.sumDist;
		st.cmps += one.cmps;
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

	double total = totalPureAlgoMs;
	double avg = total / (double)srcs.size();
	unsigned long long checksum = st.sumDist & ((1ULL << 62) - 1);

	fprintf(out, "%s,%d,%lld,%s,p2p,%zu,%.3f,%.6f,%llu,%lld,%lld,%lld,%lld,%lld,%lld,%lld\n",
	        a.net.c_str(), g.n, g.nArcs(), heapName, srcs.size(), total, avg, checksum,
	        st.pops, st.pushes, st.decs, st.arcScans, st.cmps, st.settled, unreachable);
	fflush(out);
	fprintf(stderr, "  [p2p ] %-11s runs=%zu total=%9.2fms avg=%8.5fms settled/q=%lld pops=%lld decs=%lld\n",
	        heapName, srcs.size(), total, avg, st.settled / (long long)srcs.size(), st.pops, st.decs);
}

template <class Runner>
void benchFull(const char* heapName, const Graph& g, Runner& R,
               const vector<int>& sources, const Args& a, FILE* out) {
	DijkstraStats st;
	double t0 = nowMs();
	for (size_t i = 0; i < sources.size(); i ++ ) {
		R.run(sources[i], -1, true, st);
	}
	double t1 = nowMs();

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

int main(int argc, char** argv) {
	Args a = parseArgs(argc, argv);

	fprintf(stderr, "[bench] Loading graph %s ...\n", a.graph.c_str());
	double t0 = nowMs();
	Graph g = loadGraph(a.graph);
	double t1 = nowMs();
	fprintf(stderr, "[bench] Graph loaded: n=%d m=%lld in %.1f ms\n", g.n, g.nArcs(), t1 - t0);

	vector<int> srcs, dsts;
	string qfile = a.p2p.empty() ? a.ss : a.p2p;
	if (!qfile.empty()) {
		if (!loadQueries(qfile, srcs, dsts)) {
			fprintf(stderr, "[bench] Cannot read query file %s\n", qfile.c_str());
			exit(1);
		}
		fprintf(stderr, "[bench] Loaded %zu queries from %s\n", srcs.size(), qfile.c_str());
	}

	vector<int> fullSources;
	if (a.fullSources > 0) {
		mt19937 rng(a.seed ^ 0x9e3779b9u);
		uniform_int_distribution<int> U(1, g.n);
		for (int i = 0; i < a.fullSources; i ++ ) {
			fullSources.push_back(U(rng));
		}
	}

	FILE* out = stdout;
	if (!a.out.empty()) {
		bool needHeader = true;
		if (FILE* probe = fopen(a.out.c_str(), "rb")) {
			fseek(probe, 0, SEEK_END);
			needHeader = (ftell(probe) == 0);
			fclose(probe);
		}
		out = fopen(a.out.c_str(), "a");
		if (!out) {
			fprintf(stderr, "[bench] Cannot open --out %s\n", a.out.c_str());
			exit(1);
		}
		if (needHeader) {
			fprintf(out, "%s\n", CSV_HEADER);
		}
	}

	if (hasHeap(a.heaps, "bin")) {
		DijkstraRunner<BinaryHeap> R(g);
		if (!srcs.empty()) {
			benchP2P("binary", g, R, srcs, dsts, a, out, a.dump);
		}
		if (!fullSources.empty()) {
			benchFull("binary", g, R, fullSources, a, out);
		}
	}
	if (hasHeap(a.heaps, "binom")) {
		DijkstraRunner<BinomialHeap> R(g);
		if (!srcs.empty()) {
			benchP2P("binomial", g, R, srcs, dsts, a, out, a.dump);
		}
		if (!fullSources.empty()) {
			benchFull("binomial", g, R, fullSources, a, out);
		}
	}
	if (hasHeap(a.heaps, "fib")) {
		DijkstraRunner<FibonacciHeap> R(g);
		if (!srcs.empty()) {
			benchP2P("fibonacci", g, R, srcs, dsts, a, out, a.dump);
		}
		if (!fullSources.empty()) {
			benchFull("fibonacci", g, R, fullSources, a, out);
		}
	}
	if (hasHeap(a.heaps, "lazy")) {
		LazyDijkstraRunner R(g);
		if (!srcs.empty()) {
			benchP2P("binary-lazy", g, R, srcs, dsts, a, out, a.dump);
		}
		if (!fullSources.empty()) {
			benchFull("binary-lazy", g, R, fullSources, a, out);
		}
	}

	if (out != stdout) {
		fclose(out);
	}
	return 0;
}
