#include <cerrno>
#include <cstdio>
#include <cstring>
#include <random>
#include <cstdlib>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

// 专用输入与标准查询分开存放，不覆盖正在使用的标准测试数据
bool makeDirectory(const char* path) {
#ifdef _WIN32
    return _mkdir(path) == 0 || errno == EEXIST;
#else
    return mkdir(path, 0755) == 0 || errno == EEXIST;
#endif
}

bool closeOutput(FILE* out) {
    bool ok = !ferror(out);
    return fclose(out) == 0 && ok;
}

// 保持标准生成器的种子和抽样规则，前一千条与同环境的标准输入一致
bool roadQueries(const char* net, unsigned seed) {
    char path[160];
    snprintf(path, sizeof(path), "data/assets/d/USA-road-d.%s.gr", net);
    FILE* in = fopen(path, "r");
    if (!in) { fprintf(stderr, "Cannot read %s\n", path); return false; }
    int n = 0;
    long long m = 0;
    char line[1024];
    while (fgets(line, sizeof(line), in)) {
        if (sscanf(line, "p sp %d %lld", &n, &m) == 2) break;
    }
    fclose(in);
    if (n < 2 || m < 0) return false;
    snprintf(path, sizeof(path), "data/queries/custom/%s_5000.in", net);
    FILE* out = fopen(path, "w");
    if (!out) return false;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> vertex(1, n);
    fprintf(out, "c uniform random queries, seed=%u\np aux sp p2p 5000\n", seed);
    for (int i = 0; i < 5000; i ++ ) {
        int s = vertex(rng), t = vertex(rng);
        while (s == t) t = vertex(rng);
        fprintf(out, "q %d %d\n", s, t);
    }
    bool ok = closeOutput(out);
    if (ok) printf("Generated %s: 5000 queries\n", path);
    return ok;
}

// 三层高减键图：所有中间点先出堆，每轮候选距离下降 A，且倒序更新目标
// d_i(v_j)=B+(k-i)*A+j；源点到中间点的距离为 i
// 因此中间点到目标的边权为 d_i(v_j)-i，全部为正整数
// 倒序编号使本轮后更新的目标距离更小，增加二叉堆上浮压力
bool denseGraph(int k, int r, int queries) {
    const char* graph = "data/queries/custom/dense.in";
    FILE* out = fopen(graph, "w");
    if (!out) return false;
    long long a = r + 1LL, b = k + 1LL;
    fprintf(out, "p sp %d %lld\n", 1 + k + r, k + 1LL * k * r);
    for (int i = 1; i <= k; i ++ ) fprintf(out, "a 1 %d %d\n", 1 + i, i);
    for (int i = 1; i <= k; i ++ ) {
        for (int j = r; j >= 1; j -- ) {
            long long w = b + (k - i) * a + j - i;
            fprintf(out, "a %d %d %lld\n", 1 + i, 1 + k + j, w);
        }
    }
    if (!closeOutput(out)) return false;
    out = fopen("data/queries/custom/dense_queries.in", "w");
    if (!out) return false;
    fprintf(out, "p aux sp p2p %d\n", queries);
    for (int i = 0; i < queries; i ++ ) fprintf(out, "q 1 %d\n", 2 + k + i % r);
    bool ok = closeOutput(out);
    if (ok) printf("Generated %s: %d nodes, %lld arcs, %d queries, %lld decreases/search\n",
                   graph, 1 + k + r, k + 1LL * k * r, queries, 1LL * (k - 1) * r);
    return ok;
}

int main(int argc, char** argv) {
    int mode = -1, k = 3000, r = 3000, queries = 1000;
    for (int i = 1; i < argc; i ++ ) {
        if (i + 1 >= argc) { fprintf(stderr, "Missing option value\n"); return 2; }
        const char* option = argv[i];
        char* end = nullptr;
        long value = strtol(argv[++ i], &end, 10);
        if (!end || *end || value < 0 || value > 1000000) return 2;
        if (strcmp(option, "-custom") == 0 && value <= 1) mode = (int)value;
        else if (strcmp(option, "--k") == 0) k = (int)value;
        else if (strcmp(option, "--r") == 0) r = (int)value;
        else if (strcmp(option, "--queries") == 0) queries = (int)value;
        else { fprintf(stderr, "Usage: %s [-custom 0|1] [--k N --r N --queries Q]\n", argv[0]); return 2; }
    }
    // 图读取器使用 int 边权，限制构造值以避免整数溢出
    if (k < 1 || r < 1 || queries < 1 || k + 1LL + (k - 1LL) * (r + 1LL) + r > 2000000000LL) return 2;
    if (!makeDirectory("data") || !makeDirectory("data/queries") || !makeDirectory("data/queries/custom")) return 1;
    bool ok = true;
    if (mode != 1) {
        const char* nets[] = {"NY", "BAY", "COL"};
        for (int i = 0; i < 3; i ++ ) if (!roadQueries(nets[i], 20261017u + i)) ok = false;
    }
    if (mode != 0 && !denseGraph(k, r, queries)) ok = false;
    if (!ok) fprintf(stderr, "Custom input generation failed\n");
    return ok ? 0 : 1;
}
