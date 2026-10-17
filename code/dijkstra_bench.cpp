#include "bench_common.h"

// 逐条执行点对查询，目标有效出堆时结束搜索
// 参数解析、数据选择与输出由测试库处理
int main(int argc, char** argv) {
    return runSelected(parseArgs(argc, argv));
}
