#ifndef HEAPS_H
#define HEAPS_H

#include <deque>
#include <vector>
#include <utility>

using namespace std;
typedef long long Key;
const Key KEY_INF = 1LL << 60;

// 二叉堆的数组中只保存顶点编号，距离由 key 按编号索引
// pos 是反向映射，任何元素移动都要更新对应位置
// 同一顶点在当前堆中最多出现一次
// 减键从 pos 直接定位，然后沿父链上浮
// 弹出最小值后使用末尾元素补根，再沿较小孩子下沉
// 数组位置 i 的父亲为 (i - 1) / 2
// 左右孩子分别位于 2 * i + 1 和 2 * i + 2
// 清空只重置 touched 中的 pos，不必重置所有候选距离
// insert 和 decreaseKey 的调用者应保证顶点成员状态正确
// 所有键值比较统一经过 lessKey，以便与其他堆比较工作量
struct BinaryHeap {
	BinaryHeap(int n = 0) {
		init(n);
	}

	// 按顶点编号分配索引空间，所有顶点初始都不在堆中
	void init(int n) {
		key.assign(n + 1, 0); // 有效性由位置索引决定，键值无需统一设为无穷
		pos.assign(n + 1, -1); // 顶点编号从一开始，负值表示尚未入堆
		heap.clear();
		touched.clear();
		heap.reserve(n + 1);
		touched.reserve(n + 1);
		cmp_cnt = 0;
	}

	// 只恢复本轮涉及的索引并复用已有容量，避免反复分配节点
	void clear() {
		for (size_t i = 0; i < touched.size(); i ++ ) { // 清理只遍历本轮涉及的顶点，不扫描整张路网
			pos[touched[i]] = -1;
		}
		touched.clear();
		heap.clear();
	}

	// 由当前有效堆元素判断搜索是否结束
	bool empty() const {
		return heap.empty();
	}

	// 返回当前堆内元素个数
	int size() const {
		return (int)heap.size();
	}

	// 通过顶点索引判断是否已经入堆，避免扫描整个堆
	bool contains(int v) const {
		return pos[v] >= 0;
	}

	// 读取顶点当前候选距离，供松弛操作判断是否需要减键
	Key keyOf(int v) const {
		return key[v];
	}

	// 比较次数累积到堆对象中，查询前后作差即可获得单轮统计
	long long cmps() const {
		return cmp_cnt;
	}

	// 建立新候选距离并维护顶点到堆位置的映射
	void insert(int v, Key k) {
		key[v] = k;
		pos[v] = (int)heap.size();
		heap.push_back(v);
		touched.push_back(v);
		siftUp((int)heap.size() - 1);
	}

	// 只允许键值减小，恢复堆序时同步维护节点编号映射
	void decreaseKey(int v, Key k) {
		key[v] = k;
		siftUp(pos[v]);
	}

	// 弹出最小候选距离，剩余结构重新满足最小堆性质
	int extractMin() {
		int top = heap[0];
		int last = heap.back();
		heap.pop_back();
		pos[top] = -1; // 弹出的顶点不再属于当前堆
		if (!heap.empty()) { // 堆非空时才需要把尾元素移动到根位置
			heap[0] = last;
			pos[last] = 0;
			siftDown(0);
		}
		return top;
	}

	// 暂存待调整元素，将祖先逐层下移以减少交换次数
	void siftUp(int i) {
		int v = heap[i]; // 保存该节点对应的图顶点编号
		Key k = key[v];
		while (i > 0) { // 只有非根节点才可能继续向父节点上浮
			int p = (i - 1) >> 1;
			int pv = heap[p];
			if (!lessKey(k, key[pv])) { // 与父节点满足最小堆序时停止移动
				break;
			} else {
				heap[i] = pv;
				pos[pv] = i; // 被下移的祖先也必须更新位置
				i = p;
			}
		}
		heap[i] = v;
		pos[v] = i; // 数组位置变化后同步反向索引
	}

	// 每次选择较小的孩子上移，直到暂存元素可以放入空位
	void siftDown(int i) {
		int n = (int)heap.size();
		int v = heap[i]; // 保存该节点对应的图顶点编号
		Key k = key[v];
		for (;;) {
			int c = 2 * i + 1;
			if (c >= n) { // 当前空位没有孩子，暂存元素可直接落位
				break;
			} else {
				if (c + 1 < n && lessKey(key[heap[c + 1]], key[heap[c]])) { // 右孩子存在且更小时，优先选择右孩子
					c ++ ;
				}
				if (!lessKey(key[heap[c]], k)) { // 孩子中的最小键值已不能继续改善当前层
					break;
				} else {
					heap[i] = heap[c];
					pos[heap[i]] = i;
					i = c;
				}
			}
		}
		heap[i] = v;
		pos[v] = i; // 数组位置变化后同步反向索引
	}

	// 所有键值比较统一计数，便于解释不同堆的实际工作量
	bool lessKey(Key a, Key b) {
		cmp_cnt ++ ; // 只统计键值比较，不包含循环条件判断
		return a < b;
	}

	vector<Key> key; // 保存节点键值或按顶点索引的候选距离
	vector<int> pos; // 顶点到数组位置的反向索引，不在堆中时使用无效值
	vector<int> heap; // 数组位置表示完全二叉树中的位置
	vector<int> touched; // 记录本轮涉及的顶点，使清理只访问实际使用的索引
	long long cmp_cnt = 0; // 累计键值比较次数，清空堆时保留以便作差
};

// =============================================================================
//  2. 二项堆（严格二项树森林，对度数按位合并）
// =============================================================================
// 二项堆由不同阶数的二项树组成，根链表按阶数排列
// 一棵阶数为 k 的二项树包含二的 k 次方个节点
// 同阶树合并时以较小根作为新父节点
// 插入单节点相当于与一个零阶二项树合并
// 根表遇到连续三个同阶根时要避免破坏阶数顺序
// 减键沿父链交换键值和顶点编号，因此节点映射也要交换
// 弹出最小根需要先扫描所有根，不依赖缓存最小指针
// 摘下最小根后逆转孩子链表，再与剩余根表归并
// 节点池复用保留历史容量，clear 不逐个释放节点
// 该实现只用于非负权 Dijkstra，不提供删除任意键的接口
struct BinomialHeap {
	struct Node {
		Key key; // 保存节点键值或按顶点索引的候选距离
		int v; // 保存该节点对应的图顶点编号
		int degree; // 孩子数量，也是归并时使用的树阶数
		Node *parent; // 父指针用于减键上浮或级联切断
		Node *child; // 孩子链表入口，弹出父节点时需要提升孩子
		Node *sibling; // 兄弟指针将同层节点连接成链表
	};

	BinomialHeap(int n = 0) {
		init(n);
	}

	// 按顶点编号分配索引空间，所有顶点初始都不在堆中
	void init(int n) {
		pool.resize(n + 1);
		handle.assign(n + 1, nullptr);
		in.assign(n + 1, 0);
		touched.clear();
		touched.reserve(n + 1);
		head = nullptr;
		nfree = 0; // 保留节点池容量，下一轮从首个节点复用
		total_size = 0;
		cmp_cnt = 0;
	}

	// 只恢复本轮涉及的索引并复用已有容量，避免反复分配节点
	void clear() {
		for (size_t i = 0; i < touched.size(); i ++ ) { // 清理只遍历本轮涉及的顶点，不扫描整张路网
			int v = touched[i]; // 保存该节点对应的图顶点编号
			handle[v] = nullptr;
			in[v] = 0;
		}
		touched.clear();
		head = nullptr;
		nfree = 0; // 保留节点池容量，下一轮从首个节点复用
		total_size = 0;
	}

	// 由当前有效堆元素判断搜索是否结束
	bool empty() const {
		return head == nullptr;
	}

	// 通过顶点索引判断是否已经入堆，避免扫描整个堆
	bool contains(int v) const {
		return in[v] != 0;
	}

	// 读取顶点当前候选距离，供松弛操作判断是否需要减键
	Key keyOf(int v) const {
		return handle[v]->key;
	}

	// 比较次数累积到堆对象中，查询前后作差即可获得单轮统计
	long long cmps() const {
		return cmp_cnt;
	}

	// 建立新候选距离并维护顶点到堆位置的映射
	void insert(int v, Key k) {
		Node *x = alloc();
		x->key = k;
		x->v = v;
		x->degree = 0;
		x->parent = nullptr; // 移到根链表后不再有父节点
		x->child = nullptr; // 更新孩子入口，保证后续能遍历全部孩子
		x->sibling = nullptr; // 维护同层链表，便于根表归并或孩子提升
		handle[v] = x;
		in[v] = 1;
		touched.push_back(v);
		head = unionLists(head, x);
		total_size ++ ; // 插入增加有效元素数，节点池容量不代表堆大小
	}

	// 只允许键值减小，恢复堆序时同步维护节点编号映射
	void decreaseKey(int v, Key k) {
		Node *x = handle[v];
		x->key = k;
		while (x->parent && lessKey(x->key, x->parent->key)) {
			Node *p = x->parent;
			swap(x->key, p->key); // 交换键值和顶点归属后，必须保持索引映射一致
			swap(x->v, p->v); // 交换键值和顶点归属后，必须保持索引映射一致
			handle[x->v] = x;
			handle[p->v] = p;
			x = p;
		}
	}

	// 弹出最小候选距离，剩余结构重新满足最小堆性质
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
			prevMin->sibling = minNd->sibling; // 维护同层链表，便于根表归并或孩子提升
		} else {
			head = minNd->sibling;
		}

		int v = minNd->v; // 保存该节点对应的图顶点编号
		handle[v] = nullptr;
		in[v] = 0;
		total_size -- ; // 弹出只减少有效元素数，节点空间留待复用

		Node *ch = minNd->child;
		Node *rev = nullptr;
		while (ch) {
			Node *nxt = ch->sibling;
			ch->sibling = rev; // 维护同层链表，便于根表归并或孩子提升
			ch->parent = nullptr; // 更新所属父节点，供减键时沿父链处理
			rev = ch;
			ch = nxt;
		}

		head = unionLists(head, rev);
		return v;
	}

	// 复用稳定地址的节点池，避免每次插入都申请独立内存
	Node* alloc() {
		if ((size_t)nfree >= pool.size()) { // 只有历史容量不足时才扩展节点池
			pool.emplace_back();
		}
		return &pool[(size_t)nfree ++ ]; // 返回池内稳定地址，不能使用会移动节点的数组
	}

	static Node* mergeRoots(Node *a, Node *b) {
		Node dummy;
		dummy.sibling = nullptr;
		Node *tail = &dummy;
		while (a && b) {
			if (a->degree <= b->degree) {
				tail->sibling = a; // 维护同层链表，便于根表归并或孩子提升
				a = a->sibling;
			} else {
				tail->sibling = b; // 维护同层链表，便于根表归并或孩子提升
				b = b->sibling;
			}
			tail = tail->sibling;
		}
		tail->sibling = a ? a : b; // 维护同层链表，便于根表归并或孩子提升
		return dummy.sibling;
	}

	// 将较大根挂到较小根下，两棵同阶树合并后阶数增加一
	void link(Node *y, Node *z) {
		y->parent = z; // 合并后较小根成为父节点
		y->sibling = z->child; // 维护同层链表，便于根表归并或孩子提升
		z->child = y; // 更新孩子入口，保证后续能遍历全部孩子
		z->degree ++ ; // 挂接一个孩子后，父节点度数增加一
	}

	// 按阶数合并根链表，并处理连续出现多个同阶根的情况
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
					x->sibling = nx->sibling; // 维护同层链表，便于根表归并或孩子提升
					link(nx, x);
				} else {
					if (prev) {
						prev->sibling = nx; // 维护同层链表，便于根表归并或孩子提升
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

	// 所有键值比较统一计数，便于解释不同堆的实际工作量
	bool lessKey(Key a, Key b) {
		cmp_cnt ++ ; // 只统计键值比较，不包含循环条件判断
		return a < b;
	}

	deque<Node> pool; // 双端队列扩容后已有节点地址保持有效
	vector<Node*> handle;
	vector<char> in;
	vector<int> touched; // 记录本轮涉及的顶点，使清理只访问实际使用的索引
	Node *head = nullptr; // 根链表入口，根节点按度数有序连接
	int nfree = 0; // 节点池下一可复用位置，清空后从头使用
	int total_size = 0; // 有效元素总数，不包含节点池中的闲置空间
	long long cmp_cnt = 0; // 累计键值比较次数，清空堆时保留以便作差
};

// =============================================================================
//  3. 斐波那契堆（双向循环链表 + 级联剪枝 + 延迟合并）
// =============================================================================
// 根节点和同胞节点均以循环双链表连接
// 最小根指针避免每次取最小值都扫描根链表
// 插入只挂接新根，树的归并延迟到弹出最小值时
// 度数归并表 A 的下标代表根节点拥有的孩子数
// 归并前先收集根节点，防止修改链表时丢失遍历位置
// 非根节点首次失去孩子时设置 mark
// 已标记节点再次失去孩子时被切到根链表
// 级联切断沿父链继续，遇到未标记节点或根时停止
// 从父节点切下的根重新清除 mark，保持势能分析的不变量
// 节点池使用稳定地址，父子与兄弟指针在扩容后继续有效
struct FibonacciHeap {
	struct Node {
		Key key; // 保存节点键值或按顶点索引的候选距离
		int v; // 保存该节点对应的图顶点编号
		int degree; // 孩子数量，也是归并时使用的树阶数
		bool mark; // 记录非根节点是否已经失去过一个孩子
		Node *parent; // 父指针用于减键上浮或级联切断
		Node *child; // 孩子链表入口，弹出父节点时需要提升孩子
		Node *left; // 循环双链表的前驱，用于常数时间摘除节点
		Node *right; // 循环双链表的后继，用于常数时间拼接节点
	};

	FibonacciHeap(int n = 0) {
		init(n);
	}

	// 按顶点编号分配索引空间，所有顶点初始都不在堆中
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
		nfree = 0; // 保留节点池容量，下一轮从首个节点复用
		total_size = 0;
		cmp_cnt = 0;
	}

	// 只恢复本轮涉及的索引并复用已有容量，避免反复分配节点
	void clear() {
		for (size_t i = 0; i < touched.size(); i ++ ) { // 清理只遍历本轮涉及的顶点，不扫描整张路网
			int v = touched[i]; // 保存该节点对应的图顶点编号
			handle[v] = nullptr;
			in[v] = 0;
		}
		touched.clear();
		min_node = nullptr;
		nfree = 0; // 保留节点池容量，下一轮从首个节点复用
		total_size = 0;
		used.clear();
	}

	// 由当前有效堆元素判断搜索是否结束
	bool empty() const {
		return min_node == nullptr;
	}

	// 通过顶点索引判断是否已经入堆，避免扫描整个堆
	bool contains(int v) const {
		return in[v] != 0;
	}

	// 读取顶点当前候选距离，供松弛操作判断是否需要减键
	Key keyOf(int v) const {
		return handle[v]->key;
	}

	// 比较次数累积到堆对象中，查询前后作差即可获得单轮统计
	long long cmps() const {
		return cmp_cnt;
	}

	// 建立新候选距离并维护顶点到堆位置的映射
	void insert(int v, Key k) {
		Node *x = alloc();
		x->key = k;
		x->v = v;
		x->degree = 0;
		x->mark = false; // 成为根或新孩子时重新开始记录失子状态
		x->parent = nullptr; // 移到根链表后不再有父节点
		x->child = nullptr; // 更新孩子入口，保证后续能遍历全部孩子
		x->left = x; // 维护双向链接，使后续可以常数时间摘链
		x->right = x; // 维护后继关系，保留循环链表的连接不变量

		if (!min_node) { // 空堆或出现更小根时更新最小节点缓存
			min_node = x;
		} else {
			x->left = min_node; // 维护双向链接，使后续可以常数时间摘链
			x->right = min_node->right; // 维护后继关系，保留循环链表的连接不变量
			min_node->right->left = x;
			min_node->right = x; // 维护后继关系，保留循环链表的连接不变量
			if (lessKey(k, min_node->key)) {
				min_node = x;
			}
		}
		handle[v] = x;
		in[v] = 1;
		touched.push_back(v);
		total_size ++ ; // 插入增加有效元素数，节点池容量不代表堆大小
	}

	// 只允许键值减小，恢复堆序时同步维护节点编号映射
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

	// 弹出最小候选距离，剩余结构重新满足最小堆性质
	int extractMin() {
		Node *z = min_node;
		int v = z->v; // 保存该节点对应的图顶点编号

		Node *c = z->child;
		if (c) {
			do {
				Node *nxt = c->right;
				c->parent = nullptr; // 更新所属父节点，供减键时沿父链处理
				c->right = z->right; // 维护后继关系，保留循环链表的连接不变量
				z->right->left = c;
				c->left = z; // 维护双向链接，使后续可以常数时间摘链
				z->right = c; // 维护后继关系，保留循环链表的连接不变量
				c = nxt;
			} while (c != z->child);
			z->child = nullptr; // 更新孩子入口，保证后续能遍历全部孩子
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
		total_size -- ; // 弹出只减少有效元素数，节点空间留待复用
		return v;
	}

	// 复用稳定地址的节点池，避免每次插入都申请独立内存
	Node* alloc() {
		if ((size_t)nfree >= pool.size()) { // 只有历史容量不足时才扩展节点池
			pool.emplace_back();
		}
		return &pool[(size_t)nfree ++ ]; // 返回池内稳定地址，不能使用会移动节点的数组
	}

	// 将违反父子堆序的节点移到根链表，同时更新父节点度数
	void cut(Node *x, Node *y) {
		if (x->right == x) {
			y->child = nullptr; // 更新孩子入口，保证后续能遍历全部孩子
		} else {
			x->right->left = x->left;
			x->left->right = x->right;
			if (y->child == x) {
				y->child = x->right; // 更新孩子入口，保证后续能遍历全部孩子
			}
		}
		y->degree -- ; // 切走一个孩子后，父节点度数减少一
		x->parent = nullptr; // 移到根链表后不再有父节点
		x->mark = false; // 成为根或新孩子时重新开始记录失子状态

		x->left = min_node->left; // 维护双向链接，使后续可以常数时间摘链
		x->right = min_node; // 维护后继关系，保留循环链表的连接不变量
		min_node->left->right = x;
		min_node->left = x; // 维护双向链接，使后续可以常数时间摘链
	}

	// 第一次失去孩子只标记，再次失去孩子则继续向上切断
	void cascadingCut(Node *y) {
		Node *z = y->parent;
		if (z) {
			if (!y->mark) { // 尚未失子时先标记，避免立即触发级联切断
				y->mark = true; // 第一次失子只标记，暂不向上切断
			} else {
				cut(y, z);
				cascadingCut(z);
			}
		}
	}

	// 按度数归并根节点，使每个度数至多保留一棵树
	void consolidate() {
		roots.clear();
		Node *w = min_node;
		do {
			roots.push_back(w);
			w = w->right;
		} while (w != min_node);

		used.clear();
		for (size_t i = 0; i < roots.size(); i ++ ) { // 根快照与正在修改的循环链表分离
			Node *x = roots[i];
			int d = x->degree;
			while (d < (int)A.size() && A[d]) {
				Node *y = A[d];
				if (lessKey(y->key, x->key)) { // 归并时保证较小键值的根保留为父节点
					swap(x, y); // 交换键值和顶点归属后，必须保持索引映射一致
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
		for (size_t i = 0; i < used.size(); i ++ ) { // 只处理实际占用过的度数槽位
			int d = used[i];
			Node *nd = A[d];
			if (!nd) {
				continue;
			}
			A[d] = nullptr;
			if (!min_node) { // 空堆或出现更小根时更新最小节点缓存
				min_node = nd;
				nd->left = nd; // 维护双向链接，使后续可以常数时间摘链
				nd->right = nd; // 维护后继关系，保留循环链表的连接不变量
			} else {
				nd->right = min_node->right; // 维护后继关系，保留循环链表的连接不变量
				min_node->right->left = nd;
				nd->left = min_node; // 维护双向链接，使后续可以常数时间摘链
				min_node->right = nd; // 维护后继关系，保留循环链表的连接不变量
				if (lessKey(nd->key, min_node->key)) {
					min_node = nd;
				}
			}
		}
		used.clear();
	}

	// 将同度数的大根移为小根的孩子，并清除新孩子的标记
	void linkFib(Node *y, Node *x) {
		y->left->right = y->right;
		y->right->left = y->left;
		y->parent = x; // 归并只改变树结构，不交换节点地址
		if (!x->child) {
			x->child = y; // 更新孩子入口，保证后续能遍历全部孩子
			y->left = y; // 维护双向链接，使后续可以常数时间摘链
			y->right = y; // 维护后继关系，保留循环链表的连接不变量
		} else {
			y->right = x->child->right; // 维护后继关系，保留循环链表的连接不变量
			x->child->right->left = y;
			y->left = x->child; // 维护双向链接，使后续可以常数时间摘链
			x->child->right = y;
		}
		x->degree ++ ; // 挂接一个孩子后，父节点度数增加一
		y->mark = false; // 新挂接的孩子尚未失去过孩子
	}

	// 所有键值比较统一计数，便于解释不同堆的实际工作量
	bool lessKey(Key a, Key b) {
		cmp_cnt ++ ; // 只统计键值比较，不包含循环条件判断
		return a < b;
	}

	deque<Node> pool; // 双端队列扩容后已有节点地址保持有效
	vector<Node*> handle;
	vector<char> in;
	vector<int> touched; // 记录本轮涉及的顶点，使清理只访问实际使用的索引
	vector<Node*> A; // 按节点度数索引的临时归并表
	vector<int> used; // 记录本次使用过的度数槽位，便于局部清理
	vector<Node*> roots; // 先收集根节点，再修改链表以避免遍历失效
	Node *min_node = nullptr; // 缓存全堆最小根，插入和减键时及时更新
	int nfree = 0; // 节点池下一可复用位置，清空后从头使用
	int total_size = 0; // 有效元素总数，不包含节点池中的闲置空间
	long long cmp_cnt = 0; // 累计键值比较次数，清空堆时保留以便作差
};

// =============================================================================
//  4. 惰性删除二叉堆（Lazy Binary Heap）
// =============================================================================
// 惰性二叉堆存储顶点和当时距离，不维护顶点位置映射
// 同一顶点的距离改善后直接追加一条新记录
// 因此队列条目数可能远大于图中顶点数
// 出堆同时返回键值，调用者需要检查记录是否过期
// 过期记录不能再次扫描该顶点的出边
// 比较只使用键值，相同键值的弹出顺序不影响最短距离
// 平行键值数组和顶点数组的移动必须使用相同位置
// 清空只丢弃队列条目，顶点的 seen 和 done 由搜索层管理
// 该堆不直接提供 contains 和 decreaseKey
// 模板适配层把逻辑减键转换为 push，搜索循环保持统一
struct LazyBinaryHeap {
	LazyBinaryHeap(int /*n*/ = 0) {}

	// 只恢复本轮涉及的索引并复用已有容量，避免反复分配节点
	void clear() {
		key.clear();
		ver.clear();
	}

	// 由当前有效堆元素判断搜索是否结束
	bool empty() const {
		return key.empty();
	}

	// 惰性堆允许同一顶点重复入堆，旧记录由搜索层识别
	void push(int v, Key k) {
		key.push_back(k);
		ver.push_back(v);
		siftUp((int)key.size() - 1);
	}

	// 弹出最小候选距离，剩余结构重新满足最小堆性质
	int extractMin(Key &outKey) {
		int v = ver[0]; // 保存该节点对应的图顶点编号
		outKey = key[0];
		Key lk = key.back();
		int lv = ver.back();
		key.pop_back();
		ver.pop_back();
		if (!key.empty()) { // 堆非空时才需要把尾元素移动到根位置
			key[0] = lk;
			ver[0] = lv;
			siftDown(0);
		}
		return v;
	}

	// 比较次数累积到堆对象中，查询前后作差即可获得单轮统计
	long long cmps() const {
		return cmp_cnt;
	}

	// 暂存待调整元素，将祖先逐层下移以减少交换次数
	void siftUp(int i) {
		Key k = key[i];
		int v = ver[i]; // 保存该节点对应的图顶点编号
		while (i > 0) { // 只有非根节点才可能继续向父节点上浮
			int p = (i - 1) >> 1;
			if (!lessKey(k, key[p])) { // 父节点已经不大于当前键值，上浮结束
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

	// 每次选择较小的孩子上移，直到暂存元素可以放入空位
	void siftDown(int i) {
		int n = (int)key.size();
		Key k = key[i];
		int v = ver[i]; // 保存该节点对应的图顶点编号
		for (;;) {
			int c = 2 * i + 1;
			if (c >= n) { // 当前空位没有孩子，暂存元素可直接落位
				break;
			} else {
				if (c + 1 < n && lessKey(key[c + 1], key[c])) { // 右孩子存在且更小时，优先选择右孩子
					c ++ ;
				}
				if (!lessKey(key[c], k)) { // 最小孩子也不小于暂存键值，堆序已经恢复
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

	// 所有键值比较统一计数，便于解释不同堆的实际工作量
	bool lessKey(Key a, Key b) const {
		cmp_cnt ++ ; // 只统计键值比较，不包含循环条件判断
		return a < b;
	}

	vector<Key> key; // 保存节点键值或按顶点索引的候选距离
	vector<int> ver; // 与键值数组平行存储的顶点编号
	mutable long long cmp_cnt = 0; // 累计键值比较次数，清空堆时保留以便作差
};


#endif
