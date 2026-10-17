本项目使用 C++ 编写，采用 C++17 标准，仅使用 C/C++ 标准库，无任何外部库依赖。
所有源文件均为独立自包含设计（不依赖任何自定义 .h 头文件），方便编译、审阅与评测。

【快速编译方法】

1. 使用 make 工具直接一键编译所有目标：
   make

   或者分别编译各个独立程序：
   make dijkstra_bench # 编译大规模道路网络基准测试程序
   make selftest       # 编译正确性与对拍验证程序
   make synth_bench    # 编译人工三层图（控制变量减键频率实验）测试程序
   make reset_bench    # 编译状态清理策略消融实验程序

2. 如果没有 make 工具，也可以直接通过 g++ 命令行单行编译：
   g++ -O3 -std=c++17 -o dijkstra_bench dijkstra_bench.cpp
   g++ -O3 -std=c++17 -o selftest selftest.cpp
   g++ -O3 -std=c++17 -o synth_bench synth_bench.cpp
   g++ -O3 -std=c++17 -o reset_bench reset_bench.cpp

【运行说明】

1. 运行自测程序（包含单元测试、10万次随机对拍与独立 SPFA 交叉验证）：
   ./selftest

2. 运行人工图控制变量实验（验证高减键 vs 低减键对堆性能的影响）：
   ./synth_bench 300 3000 10

3. 运行状态清理策略消融实验（需传入 .gr 与 .p2p 文件）：
   ./reset_bench ../data/USA-road-d.NY.gr ../data/queries/NY.p2p

4. 运行公路网评测主程序：
   ./dijkstra_bench --graph ../data/USA-road-d.NY.gr --net NY --p2p ../data/queries/NY.p2p --heaps bin,binom,fib,lazy --full-sources 16 --out ../results/results.csv
