#include <cstdio>
#include <cstdlib>

// 固定三层图：源点、k 个中间点、r 个目标点
// 所有输出放入 data/queries/custom，由统一 bench 读取
// 源点编号为一，中间层编号为二到 k 加一
// 目标层没有出边，因此可以清楚控制所有候选路径来源
// 高减键图中后弹出的中间点会给每个目标带来更短路径
// 低减键图中首次发现的目标距离已经最优
// 这两组数据用于解释减键频率，不代表真实路网分布
// 输出的图和点对都位于 queries/custom，供 data 一模式读取
bool generate(int k, int r, bool high) {
    const char* graph = high ? "data/queries/custom/high.in" : "data/queries/custom/low.in";
    const char* queries = high ? "data/queries/custom/high_queries.in" : "data/queries/custom/low_queries.in";
    FILE* out = fopen(graph, "w"); // 图文件与查询分开，写入时覆盖旧构造结果
    if (!out) return false;
    fprintf(out, "p sp %d %lld\n", 1 + k + r, (long long)k + (long long)k * r);
    // 中间点依次出堆，控制到达目标层的候选路径是否逐次变短
    for (int i = 1; i <= k; i ++ ) fprintf(out, "a 1 %d %d\n", 1 + i, i);
    for (int i = 1; i <= k; i ++ ) {
        for (int j = 1; j <= r; j ++ ) {
            int w = high ? 3 * k - 2 * i + j : 3 * k + j; // 调整这一项即可控制后来候选路径是否更优
            fprintf(out, "a %d %d %d\n", 1 + i, 1 + k + j, w);
        }
    }
    bool ok = !ferror(out); // 写入完成后检查磁盘错误，不静默使用截断输入
    if (fclose(out)) ok = false; // 关闭时的缓冲区写入失败也应报告
    if (!ok) return false; // 图写入失败时不继续生成看似可用的查询
    out = fopen(queries, "w"); // 查询文件同样使用 DIMACS 点对格式
    if (!out) return false;
    // 查询固定从源点到目标层，完整展开中间层以保持减键控制条件
    fprintf(out, "p aux sp p2p 1000\n");
    for (int i = 0; i < 1000; i ++ ) fprintf(out, "q 1 %d\n", 2 + k + i % r);
    ok = !ferror(out); // 检查查询输出是否成功写入
    if (fclose(out)) ok = false; // 关闭时的缓冲区写入失败也应报告
    return ok;
}

// 默认规模保持原人工实验的 k 为三百和 r 为三千
// 参数只控制图规模，查询数固定为一千
// 构造目录应在运行前存在，写文件失败时返回非零退出码
int main(int argc, char** argv) {
    int k = 300, r = 3000; // 中间层和目标层规模默认与原实验一致
    if (argc != 1 && argc != 3) { fprintf(stderr, "Usage: %s [k r]\n", argv[0]); return 1; }
    if (argc == 3) { k = atoi(argv[1]); r = atoi(argv[2]); }
    // 控制顶点编号及权重范围，避免构造参数溢出
    if (k < 1 || r < 1 || (long long)3 * k + r > 2000000000LL) return 1;
    if (!generate(k, r, false) || !generate(k, r, true)) {
        fprintf(stderr, "Cannot write data/queries/custom files\n"); return 1;
    }
    printf("Generated low/high graphs and 1000 queries each\n");
    return 0;
}
