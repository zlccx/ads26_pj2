#include "bench_common.h"

// 优化入口先按起点离线分组，每个不同起点只执行一次完整 Dijkstra
// 堆实现、局部状态清理、参数和数据选择与普通 bench 共用
// 排序及答案收集计入耗时，文件读取和结果导出不计入
int main(int argc, char** argv) {
    return runSelected(parseArgs(argc, argv), true);
}
