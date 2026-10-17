#ifndef OFFLINE_H
#define OFFLINE_H

#include <vector>
#include <algorithm>
#include "dijkstra.h"

// 保存原下标，离线重排不改变最终答案与输入查询的对应关系
struct OfflineQuery {
    int source;
    int target;
    size_t index;
};

// runs 是不同起点数，不是原始点对数量
// answers 保持原输入顺序，便于与逐查询算法逐项比较
struct OfflineResult {
    vector<Key> answers;
    DijkstraStats stats;
    size_t runs = 0;
};

// 排序和答案收集属于离线策略本身，由 bench 一并计时
// 四种堆共用此模板，图读取和 CSV 导出仍放在计时区间外
template <typename Heap>
OfflineResult solveOffline(DijkstraRunner<Heap>& runner,
                           const vector<int>& sources, const vector<int>& targets) {
    OfflineResult result;
    result.answers.resize(sources.size(), KEY_INF);
    vector<OfflineQuery> queries;
    queries.reserve(sources.size());
    for (size_t i = 0; i < sources.size(); i ++ ) {
        queries.push_back({sources[i], targets[i], i});
    }
    // 相同起点相邻后，每组只需一次完整单源搜索
    // 相同起点内按原下标排序，使输出和调试结果可复现
    sort(queries.begin(), queries.end(), [](const OfflineQuery& a, const OfflineQuery& b) {
        return a.source != b.source ? a.source < b.source : a.index < b.index;
    });
    for (size_t begin = 0; begin < queries.size();) {
        size_t end = begin + 1;
        while (end < queries.size() && queries[end].source == queries[begin].source) end ++ ;
        vector<int> groupTargets;
        groupTargets.reserve(end - begin);
        for (size_t i = begin; i < end; i ++ ) groupTargets.push_back(queries[i].target);
        vector<Key> groupAnswers;
        // 查询全部提前读入，同源目标在搜索完成后直接读取距离
        // full 为真，必须搜索完整可达分量，不能在单个目标处早停
        // runner 仅清理 touched 中的状态，不执行逐轮全数组 memset
        runner.run(queries[begin].source, -1, true, result.stats, &groupTargets, &groupAnswers);
        result.runs ++ ;
        for (size_t i = begin; i < end; i ++ ) {
            result.answers[queries[i].index] = groupAnswers[i - begin];
        }
        begin = end;
    }
    return result;
}

#endif
