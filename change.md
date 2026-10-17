# 当前仓库状态

更新日期：2026-10-17

当前保留四种堆在真实 USA 路网上的标准性能对比、两个专用测试模式，以及 STL priority_queue 正确性自测。

- 每张图默认生成 1000 对随机查询，每条查询独立执行，到目标有效出堆时早停
- 标准测试直接运行 code/dijkstra_bench.exe：先测试 NY/BAY/COL 的 d+t 六图并输出对比，再测试 NY 到 CAL 的 d 七图并输出对比；两阶段独立计时，默认只输出控制台；-full 才测试全部二十四图
- 支持 distance / travel time 两种权重；mini 仍可单独测试 NY、BAY、COL 的两种权重版本
- bench 仅保留默认、-mini、-full、-custom 0、-custom 1 五种固定模式；Args 仅保存模式，Dataset 保存内部路径
- 固定比较四种堆并输出控制台，保留操作计数；不提供单图路径、堆筛选、文件导出和额外完整单源参数
- 旧人工 low/high、状态清理消融、搜索规模分档、离线同源优化及其专属结果已删除
- 已恢复 gen_custom.cpp：模式 0 测试 NY-d 的 5000 对查询（生成器同时提供 NY/BAY/COL 输入）；模式 1 为 k=r=3000 的大步长下降、倒序更新三层图，默认 1000 对查询
- bench 支持 -custom 0 / -custom 1；专用输入不覆盖标准查询
- 普通 Dijkstra 的局部状态清理作为现有实现保留
- 二项堆插入仅处理根链表开头的连续进位，遇到不同阶根即停止，均摊 O(1)、单次最坏 O(log N)
- selftest 包括四种堆的 STL 最短路对拍，以及二项堆独立的插入、减键、出堆和结构检查；编译运行全部通过

`documents/report.md` 已按四章结构更新，附录 A 为小组分工，附录 B 为当前七份源码。实验表格和四张配图使用根目录的 `fulltest.res` 与 `customtest.res`；实现亮点整理在 `code_outline.md`。姓名和学号暂留占位，PDF 按要求暂不更新。

两个 `.res` 文件记录的性能测试早于二项堆插入优化，这次优化后只重新验证了正确性，尚未重新计时。报告中的性能数据仍对应 `.res` 的实测记录。

`results/results.csv`、`results/raw/`、`results/perquery/` 和其他基础图表未用于本次报告的数据分析。

下载脚本默认准备七张 d 和三张 t 图；加 -full 才准备全部二十四图。
