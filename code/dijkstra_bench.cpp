#include "bench_common.h"

// 普通入口逐条执行点对查询，保留到目标出堆即结束的行为
// 参数解析、数据选择与输出由公共测试代码处理
int main(int argc, char** argv) {
    return runSelected(parseArgs(argc, argv), false);
}
