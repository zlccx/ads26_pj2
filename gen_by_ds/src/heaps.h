// =============================================================================
//  heaps.h —— 三种「可索引（indexed）+ 支持 decrease-key」的最小优先队列
//
//  本文件实现 Dijkstra 需要的三种最小优先队列：
//      * BinaryHeap    : 数组式二叉堆（下标映射实现 decrease-key）
//      * BinomialHeap  : 二项堆（按度数成对合并 / 链接）
//      * FibonacciHeap : 斐波那契堆（惰性合并 + 级联切断，decrease-key 摊还 O(1)）
//
//  三者对外接口完全一致，因此 Dijkstra 可以写成模板、以堆类型作为模板参数，
//  从而在同一份算法代码上公平地切换底层堆：
//      clear()                清空（跨 query 复用对象，只清理被碰过的下标）
//      empty()                是否为空
//      insert(v, key)         插入顶点 v，关键字 key
//      decreaseKey(v, key)    把已在堆中的 v 的关键字减小为 key（key 必须更小）
//      contains(v)            v 当前是否在堆中
//      keyOf(v)               v 当前的关键字
//      extractMin()           弹出关键字最小的顶点，返回顶点编号
//      cmps()                 累计关键字比较次数（用于算法分析，不计入正确性）
//
//  设计要点（保证三种堆被公平比较）：
//   1) 所有堆都是「可索引」的：顶点 -> 堆节点 的映射由堆自己维护，
//      这样 Dijkstra 才能做 decrease-key，而不是靠「惰性删除」作弊；
//      同时这种实现方式正好是三种堆各自最自然、最标准的写法。
//   2) 所有堆的节点内存都预先按 n 大小分配好（节点池），一次分配、多次复用，
//      避免让 malloc/free 的开销污染算法本身的性能测量。
//   3) 所有堆都只在「本次 query 真正碰过」的下标上做清理，清理成本 O(工作量)，
//      不会出现 O(n) 的重置掩盖算法差异。
// =============================================================================
#pragma once

#include <vector>
#include <deque>
#include <algorithm>
#include <cstddef>

// 关键字（最短距离）类型：USA 公路网两点间最远距离可达 1e7 米量级，
// 且路径累加会超过 32 位，故一律使用 64 位有符号整数。
typedef long long Key;
static const Key KEY_INF = (Key)1 << 62;   // 表示「无穷大」

// =============================================================================
//  1) 二叉堆（Array Binary Heap）
//
//  用一维数组存完全二叉树，pos_[v] 记录顶点 v 在数组中的下标，
//  从而支持 O(1) 定位 + O(log n) 上浮/下沉，即 decrease-key O(log n)。
//      insert        : 新元素放末尾，向上浮动      O(log n)
//      decreaseKey   : 关键字变小，向上浮动        O(log n)
//      extractMin    : 队首与队尾交换，向下沉       O(log n)
// =============================================================================
class BinaryHeap {
public:
    // n 为顶点个数；顶点编号为 1..n，因此内部数组长度取 n+1
    explicit BinaryHeap(int n)
        : key_((size_t)n + 1, 0), pos_((size_t)n + 1, -1) {
        heap_.reserve((size_t)n + 1);
        touched_.reserve((size_t)n + 1);
    }

    // 清空堆：只把本次 query 碰过的顶点在 pos_ 中恢复为 -1。
    // 关键字数组无需清理——Dijkstra 首次插入时会直接覆盖。
    void clear() {
        for (size_t i = 0; i < touched_.size(); ++i) pos_[touched_[i]] = -1;
        touched_.clear();
        heap_.clear();
    }

    bool empty() const { return heap_.empty(); }
    int  size()  const { return (int)heap_.size(); }
    bool contains(int v) const { return pos_[v] >= 0; }
    Key  keyOf(int v)   const { return key_[v]; }
    long long cmps()    const { return cmps_; }

    void insert(int v, Key k) {
        key_[v] = k;
        pos_[v] = (int)heap_.size();
        heap_.push_back(v);
        touched_.push_back(v);
        siftUp((int)heap_.size() - 1);
    }

    // v 必须已经在堆中，且新关键字不大于旧关键字
    void decreaseKey(int v, Key k) {
        key_[v] = k;
        siftUp(pos_[v]);
    }

    int extractMin() {
        const int top = heap_[0];
        const int last = heap_.back();
        heap_.pop_back();
        pos_[top] = -1;                 // top 不再位于堆中
        if (!heap_.empty()) {
            heap_[0] = last;
            pos_[last] = 0;
            siftDown(0);
        }
        return top;
    }

private:
    // 把下标 i 处的元素向上浮动到合适位置
    void siftUp(int i) {
        const int v = heap_[i];
        const Key k = key_[v];
        while (i > 0) {
            const int p = (i - 1) >> 1;          // 父结点下标
            const int pv = heap_[p];
            if (!less(k, key_[pv])) break;       // 已满足堆序
            heap_[i] = pv;
            pos_[pv] = i;
            i = p;
        }
        heap_[i] = v;
        pos_[v] = i;
    }

    // 把下标 i 处的元素向下沉到合适位置
    void siftDown(int i) {
        const int n = (int)heap_.size();
        const int v = heap_[i];
        const Key k = key_[v];
        for (;;) {
            int c = 2 * i + 1;                       // 左孩子
            if (c >= n) break;
            // 取两个孩子中较小的那个
            if (c + 1 < n && less(key_[heap_[c + 1]], key_[heap_[c]])) ++c;
            if (!less(key_[heap_[c]], k)) break;
            heap_[i] = heap_[c];
            pos_[heap_[i]] = i;
            i = c;
        }
        heap_[i] = v;
        pos_[v] = i;
    }

    // 统一的比较入口，顺便统计比较次数
    bool less(Key a, Key b) { ++cmps_; return a < b; }

    std::vector<Key> key_;      // key_[v]：顶点 v 当前的关键字
    std::vector<int> pos_;      // pos_[v]：顶点 v 在 heap_ 中的下标，-1 表示不在堆中
    std::vector<int> heap_;     // 完全二叉树数组，heap_[i] 为顶点编号
    std::vector<int> touched_;  // 本 query 插入过的顶点，用于 O(工作量) 清理
    long long cmps_ = 0;
};

// =============================================================================
//  2) 二项堆（Binomial Heap）
//
//  二项堆是若干棵「二项树」的森林，第 k 棵二项树 B_k 有 2^k 个结点、度数为 k，
//  根链表按度数递增排列，且任一时刻每种度数至多出现一次（这是 extractMin 中
//  「合并同度数树」维持的性质）。
//      insert        : 新建单结点树 B_0，与堆做 union        O(log n)
//      extractMin    : 摘掉最小根，其孩子逆序接回根链表再 union O(log n)
//      decreaseKey   : 关键字减小后与父结点逐层交换（上浮）     O(log n)
//  注意：二项堆的 decreaseKey 是「最坏 O(log n)」，不像斐波那契堆那样摊还 O(1)。
// =============================================================================
class BinomialHeap {
public:
    explicit BinomialHeap(int n)
        : pool_((size_t)n + 1), handle_((size_t)n + 1, nullptr), in_((size_t)n + 1, 0) {
        touched_.reserve((size_t)n + 1);
    }

    void clear() {
        for (size_t i = 0; i < touched_.size(); ++i) {
            const int v = touched_[i];
            handle_[v] = nullptr;
            in_[v] = 0;
        }
        touched_.clear();
        head_ = nullptr;
        nfree_ = 0;      // 节点池回卷：一次 query 中每个顶点至多插入一次
        n_ = 0;
    }

    bool empty() const { return head_ == nullptr; }
    bool contains(int v) const { return in_[v] != 0; }
    Key  keyOf(int v)   const { return handle_[v]->key; }
    long long cmps()    const { return cmps_; }

    void insert(int v, Key k) {
        Node* x = alloc();
        x->key = k; x->v = v; x->degree = 0;
        x->parent = nullptr; x->child = nullptr; x->sibling = nullptr;
        handle_[v] = x;
        in_[v] = 1;
        touched_.push_back(v);
        head_ = unionLists(head_, x);   // 把单结点树并入根链表
        ++n_;
    }

    // v 必须在堆中且新关键字 k 不大于旧关键字
    void decreaseKey(int v, Key k) {
        Node* x = handle_[v];
        x->key = k;
        // 沿父指针上浮：交换载荷（key 与顶点编号），并同步修正 handle_ 映射
        while (x->parent && less(x->key, x->parent->key)) {
            Node* p = x->parent;
            std::swap(x->key, p->key);
            std::swap(x->v, p->v);
            handle_[x->v] = x;
            handle_[p->v] = p;
            x = p;
        }
    }

    int extractMin() {
        // 1) 在根链表中找到最小根 minNd 及其前驱 prevMin
        Node* prev = nullptr;
        Node* minNd = nullptr;
        Node* prevMin = nullptr;
        for (Node* x = head_; x; prev = x, x = x->sibling) {
            if (!minNd || less(x->key, minNd->key)) { minNd = x; prevMin = prev; }
        }
        // 2) 把 minNd 从根链表中摘除
        if (prevMin) prevMin->sibling = minNd->sibling;
        else         head_ = minNd->sibling;

        const int v = minNd->v;
        handle_[v] = nullptr;
        in_[v] = 0;
        --n_;

        // 3) 把 minNd 的孩子链表逆序，变成新的根链表（孩子度数递减 -> 逆序后递增）
        Node* ch = minNd->child;
        Node* rev = nullptr;
        while (ch) {
            Node* nxt = ch->sibling;
            ch->sibling = rev;
            ch->parent = nullptr;
            rev = ch;
            ch = nxt;
        }
        // 4) 与已有根链表做 union（内部会合并同度数的树）
        head_ = unionLists(head_, rev);
        return v;
    }

private:
    struct Node {
        Key  key;             // 关键字（到源点的当前最短距离）
        int  v;               // 对应的顶点编号
        int  degree;          // 以本结点为根的子树的度数
        Node *parent, *child, *sibling;
    };

    // 从节点池里取一个结点。池预先按 n+1 个元素建好；
    // 用 std::deque 是为了在「必须扩容」时也不会让已有结点的指针失效
    // （Dijkstra 场景下每个顶点每次 query 至多插入一次，正常不会走到扩容分支）。
    Node* alloc() {
        if ((size_t)nfree_ >= pool_.size()) pool_.emplace_back();
        return &pool_[(size_t)nfree_++];
    }

    // 按度数递增合并两条有序根链表（不处理同度数，只做归并）
    static Node* mergeRoots(Node* a, Node* b) {
        Node dummy;
        dummy.sibling = nullptr;
        Node* tail = &dummy;
        while (a && b) {
            if (a->degree <= b->degree) { tail->sibling = a; a = a->sibling; }
            else                        { tail->sibling = b; b = b->sibling; }
            tail = tail->sibling;
        }
        tail->sibling = a ? a : b;
        return dummy.sibling;
    }

    // 把 y 变成 z 的孩子（要求 degree(y) == degree(z)）
    void link(Node* y, Node* z) {
        y->parent = z;
        y->sibling = z->child;
        z->child = y;
        z->degree++;
    }

    // union：归并两条根链表，再把度数相同的相邻根两两链接，保证度数唯一
    Node* unionLists(Node* a, Node* b) {
        Node* h = mergeRoots(a, b);
        if (!h) return nullptr;
        Node* prev = nullptr;
        Node* x = h;
        Node* nx = x->sibling;
        while (nx) {
            // 情况 1：度数不同，或后面还有一棵与 x 同度数的树 -> 先跳过
            if (x->degree != nx->degree ||
                (nx->sibling && nx->sibling->degree == x->degree)) {
                prev = x;
                x = nx;
            }
            // 情况 2：链接 x 与 nx，关键字小者做父结点
            else if (!less(nx->key, x->key)) {
                x->sibling = nx->sibling;
                link(nx, x);
            }
            else {
                if (prev) prev->sibling = nx;
                else      h = nx;
                link(x, nx);
                x = nx;
            }
            nx = x->sibling;
        }
        return h;
    }

    bool less(Key a, Key b) { ++cmps_; return a < b; }

    std::deque<Node>    pool_;     // 预分配节点池；用 deque 保证扩容时已有结点地址不变
    std::vector<Node*>  handle_;   // handle_[v]：顶点 v 对应的堆结点
    std::vector<char>   in_;       // in_[v]：顶点 v 是否在堆中
    std::vector<int>    touched_;  // 本次 query 插入过的顶点
    Node* head_ = nullptr;         // 根链表头
    int   nfree_ = 0;              // 节点池已用数量
    int   n_ = 0;                  // 堆中元素个数
    long long cmps_ = 0;
};

// =============================================================================
//  3) 斐波那契堆（Fibonacci Heap）
//
//  斐波那契堆由若干棵「无序二项树」组成，用循环双向链表维护根链表和孩子链表，
//  并维护一个指向最小根的指针 min_。与二项堆最大的区别：
//      insert        把新结点直接挂在根链表上，不合并        O(1) 摊还
//      extractMin    才做 consolidate（把同度数根两两链接），
//                    均摊到每次 insert 上是 O(1)             O(log n) 摊还
//      decreaseKey   直接把结点从父结点上「切」(cut) 下来挂到根链表；
//                    若父结点此前已被切过一次(mark)，则级联切断     O(1) 摊还
//  所以斐波那契堆相对二项堆的改进点正是 decrease-key 从 O(log n) 降到 O(1) 摊还，
//  而这恰好对应 Dijkstra 中「松弛边」的次数（可达 O(m) 量级），因此理论上更优。
// =============================================================================
class FibonacciHeap {
public:
    explicit FibonacciHeap(int n)
        : pool_((size_t)n + 1), handle_((size_t)n + 1, nullptr), in_((size_t)n + 1, 0),
          A_(64, nullptr) {
        touched_.reserve((size_t)n + 1);
        roots_.reserve(64);
        used_.reserve(64);
    }

    void clear() {
        for (size_t i = 0; i < touched_.size(); ++i) {
            const int v = touched_[i];
            handle_[v] = nullptr;
            in_[v] = 0;
        }
        touched_.clear();
        min_ = nullptr;
        nfree_ = 0;
        n_ = 0;
        used_.clear();
    }

    bool empty() const { return min_ == nullptr; }
    bool contains(int v) const { return in_[v] != 0; }
    Key  keyOf(int v)   const { return handle_[v]->key; }
    long long cmps()    const { return cmps_; }

    void insert(int v, Key k) {
        Node* x = alloc();
        x->key = k; x->v = v; x->degree = 0; x->mark = false;
        x->parent = nullptr; x->child = nullptr;
        x->left = x; x->right = x;
        if (!min_) {
            min_ = x;                       // 空堆：直接成为唯一根
        }
        else {
            // 插入到 min_ 的右侧（根链表是循环双向链表）
            x->left = min_;
            x->right = min_->right;
            min_->right->left = x;
            min_->right = x;
            if (less(k, min_->key)) min_ = x;
        }
        handle_[v] = x;
        in_[v] = 1;
        touched_.push_back(v);
        ++n_;
    }

    // v 必须在堆中且新关键字 k 不大于旧关键字
    void decreaseKey(int v, Key k) {
        Node* x = handle_[v];
        x->key = k;
        Node* y = x->parent;
        // 堆序被破坏则切断（并视情况级联向上）
        if (y && less(x->key, y->key)) {
            cut(x, y);
            cascadingCut(y);
        }
        if (less(x->key, min_->key)) min_ = x;
    }

    int extractMin() {
        Node* z = min_;
        const int v = z->v;

        // 1) 把 z 的所有孩子依次挂到根链表上（挂在 z 的右侧）
        Node* c = z->child;
        if (c) {
            do {
                Node* nxt = c->right;            // 先保存，避免被 splice 破坏
                c->parent = nullptr;
                // 把 c 插到 z 与 z->right 之间
                c->right = z->right;
                z->right->left = c;
                c->left = z;
                z->right = c;
                c = nxt;
            } while (c != z->child);
            z->child = nullptr;
        }

        // 2) 把 z 从根链表中摘除
        z->left->right = z->right;
        z->right->left = z->left;
        if (z == z->right) {
            min_ = nullptr;                      // 根链表中只剩 z
        }
        else {
            min_ = z->right;
            consolidate();                       // 合并同度数根，恢复摊还 O(log n)
        }

        handle_[v] = nullptr;
        in_[v] = 0;
        --n_;
        return v;
    }

private:
    struct Node {
        Key  key;
        int  v;
        int  degree;
        bool mark;                // 是否已失去过一个孩子（用于级联切断）
        Node *parent, *child, *left, *right;
    };

    Node* alloc() {
        if ((size_t)nfree_ >= pool_.size()) pool_.emplace_back();
        return &pool_[(size_t)nfree_++];
    }

    // 把 x 从父结点 y 的孩子链表中摘下，挂到根链表（min_ 左侧）
    void cut(Node* x, Node* y) {
        if (x->right == x) y->child = nullptr;
        else {
            x->right->left = x->left;
            x->left->right = x->right;
            if (y->child == x) y->child = x->right;
        }
        y->degree--;
        x->parent = nullptr;
        x->mark = false;
        x->left = min_->left;
        x->right = min_;
        min_->left->right = x;
        min_->left = x;
    }

    // 级联切断：若 y 已被切过一次(mark)，则继续把 y 也切下来
    void cascadingCut(Node* y) {
        Node* z = y->parent;
        if (z) {
            if (!y->mark) y->mark = true;
            else { cut(y, z); cascadingCut(z); }
        }
    }

    // 合并根链表：每种度数至多保留一棵根，其余挂为它的孩子
    //
    // 关于 A_ 与 used_ 的说明：A_[d] 是「当前度数恰为 d 的那棵根」。
    // 直接每次 memset(A_) 的代价与 A_ 的大小成正比（D(n) + 1），而每次 extract-min
    // 都需要 consolidate，所以这里改用 used_ 记录本回合真正写过的度数，
    // 只在最后复位这几项，代价与「根数」成正比。
    void consolidate() {
        // 先把当前根链表的结点收集到临时数组（遍历期间会改动链表，故先收集）
        roots_.clear();
        Node* w = min_;
        do { roots_.push_back(w); w = w->right; } while (w != min_);

        used_.clear();
        for (size_t i = 0; i < roots_.size(); ++i) {
            Node* x = roots_[i];
            int d = x->degree;
            while (d < (int)A_.size() && A_[d]) {
                Node* y = A_[d];
                if (less(y->key, x->key)) std::swap(x, y);  // 保证 x 是较小者
                linkFib(y, x);                              // y 成为 x 的孩子
                A_[d] = nullptr;
                ++d;
            }
            // 度数可能超过预设的 A_ 长度（理论上 D(n) = O(log n)，此分支不会触发），
            // 这里做一次兜底扩容，保证任何输入下都不会越界。
            if (d >= (int)A_.size()) A_.resize((size_t)d + 1, nullptr);
            used_.push_back(d);     // 循环退出时 A_[d] 必为 nullptr
            A_[d] = x;
        }

        // 由 A_ 重建根链表（同时求出新的 min_）
        min_ = nullptr;
        for (size_t i = 0; i < used_.size(); ++i) {
            const int d = used_[i];
            Node* nd = A_[d];
            if (!nd) continue;        // 该度数在前面已被消费过
            A_[d] = nullptr;          // 消费掉，避免同一结点被插入两次
            if (!min_) {
                min_ = nd;
                nd->left = nd; nd->right = nd;
            }
            else {
                nd->right = min_->right;
                min_->right->left = nd;
                nd->left = min_;
                min_->right = nd;
                if (less(nd->key, min_->key)) min_ = nd;
            }
        }
        used_.clear();
    }

    // 把 y 从根链表摘下并作为 x 的孩子（要求 degree(y) == degree(x)）
    void linkFib(Node* y, Node* x) {
        // 从根链表移除 y
        y->left->right = y->right;
        y->right->left = y->left;
        // 挂到 x 的孩子链表中
        y->parent = x;
        if (!x->child) {
            x->child = y;
            y->left = y; y->right = y;
        }
        else {
            y->right = x->child->right;
            x->child->right->left = y;
            y->left = x->child;
            x->child->right = y;
        }
        x->degree++;
        y->mark = false;
    }

    bool less(Key a, Key b) { ++cmps_; return a < b; }

    std::deque<Node>    pool_;     // 预分配节点池；用 deque 保证扩容时已有结点地址不变
    std::vector<Node*>  handle_;   // handle_[v]：顶点 v 对应的堆结点
    std::vector<char>   in_;       // in_[v]：顶点 v 是否在堆中
    std::vector<int>    touched_;  // 本次 query 插入过的顶点
    std::vector<Node*>  A_;        // 度数 -> 根
    std::vector<int>    used_;     // A_ 中本回合被写过的度数（用于 O(根数) 复位）
    std::vector<Node*>  roots_;    // consolidate 用的临时根集合
    Node* min_ = nullptr;
    int   nfree_ = 0;              // 节点池已用数量
    int   n_ = 0;                  // 堆中元素个数
    long long cmps_ = 0;
};

// =============================================================================
//  4) 惰性删除二叉堆（Lazy Binary Heap）——额外的对照实现
//
//  这是工程上最常用的 Dijkstra 写法：堆里只存 (key, v)，不做 decrease-key，
//  松弛成功就直接 push 一个新副本；extractMin 时若发现 key 不是最新值就丢弃。
//  它的堆元素个数会涨到 O(m)，理论复杂度略差，但常数因子小、实现简单。
//  加入它是为了在报告中回答一个关键问题：
//      「既然斐波那契堆理论最优，为什么实践中大家用普通二叉堆？」
// =============================================================================
class LazyBinaryHeap {
public:
    explicit LazyBinaryHeap(int /*n*/) { }

    void clear() { key_.clear(); ver_.clear(); }

    bool empty() const { return key_.empty(); }

    // 插入（或再次插入）顶点 v 及其关键字
    void push(int v, Key k) {
        key_.push_back(k);
        ver_.push_back(v);
        siftUp((int)key_.size() - 1);
    }

    // 弹出最小元素；返回顶点编号，key 通过 outKey 带回
    int extractMin(Key& outKey) {
        const int v = ver_[0];
        outKey = key_[0];
        const Key lk = key_.back();
        const int  lv = ver_.back();
        key_.pop_back();
        ver_.pop_back();
        if (!key_.empty()) {
            key_[0] = lk;
            ver_[0] = lv;
            siftDown(0);
        }
        return v;
    }

    long long cmps() const { return cmps_; }

private:
    void siftUp(int i) {
        const Key k = key_[i];
        const int v = ver_[i];
        while (i > 0) {
            const int p = (i - 1) >> 1;
            if (!less(k, key_[p])) break;
            key_[i] = key_[p];
            ver_[i] = ver_[p];
            i = p;
        }
        key_[i] = k;
        ver_[i] = v;
    }

    void siftDown(int i) {
        const int n = (int)key_.size();
        const Key k = key_[i];
        const int v = ver_[i];
        for (;;) {
            int c = 2 * i + 1;
            if (c >= n) break;
            if (c + 1 < n && less(key_[c + 1], key_[c])) ++c;
            if (!less(key_[c], k)) break;
            key_[i] = key_[c];
            ver_[i] = ver_[c];
            i = c;
        }
        key_[i] = k;
        ver_[i] = v;
    }

    bool less(Key a, Key b) const { ++cmps_; return a < b; }

    std::vector<Key> key_;
    std::vector<int> ver_;
    mutable long long cmps_ = 0;
};
