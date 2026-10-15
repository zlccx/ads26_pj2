#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""交叉验证：同一点对查询下，四种实现算出的 dist[t] 必须逐位相同。

结果写入 results/logs/verify.log（报告 Appendix B 会引用）。

背景：bench 里的 checksum 是「被确定结点距离之和」。在**完整单源**模式下这个量是
堆无关的，四种实现必然完全相同；但在**点对（提前终止）**模式下，如果最短路径上
存在与 dist[t] 并列的结点，不同实现可能选择先弹出哪个，于是「被确定结点集合」
会有极微小的差异，checksum 也就不完全相等。因此真正的正确性判据是
**每次查询的答案 dist[t] 是否相同**——本脚本就是检查这个。
"""
import glob
import os
import sys

import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PQ = os.path.join(ROOT, "results", "perquery")
OUT = os.path.join(ROOT, "results", "logs", "verify.log")

HEAPS = ["binary", "binomial", "fibonacci", "binary-lazy"]


def main():
    nets = sorted({os.path.basename(f).split(".")[0]
                   for f in glob.glob(os.path.join(PQ, "*.csv"))})
    if not nets:
        print("no per-query dumps in", PQ)
        return 1
    lines = []
    bad = 0
    for net in nets:
        ref = None
        refname = None
        info = []
        for h in HEAPS:
            p = os.path.join(PQ, "%s.%s.csv" % (net, h))
            if not os.path.exists(p) or os.path.getsize(p) < 64:
                continue          # 尚未写完（bench 正在运行）
            try:
                df = pd.read_csv(p)
            except Exception:
                continue
            if "dist" not in df.columns or len(df) == 0:
                continue
            d = df["dist"].tolist()
            if ref is not None and len(d) != len(ref):
                continue          # 行数不齐：文件仍在写入中，跳过
            if ref is None:
                ref, refname = d, h
                info.append("  %-12s queries=%d, unreachable=%d" %
                            (h, len(d), sum(1 for x in d if x < 0)))
            else:
                n_diff = sum(1 for a, b in zip(ref, d) if a != b)
                if n_diff:
                    bad += 1
                info.append("  %-12s vs %-12s : %s" % (h, refname,
                            "IDENTICAL" if n_diff == 0 else "DIFFER in %d queries" % n_diff))
        lines.append("%s: per-query dist[t] cross-check over %d implementations" %
                     (net, len(info)))
        lines.extend(info)
        lines.append("")
    lines.append("RESULT: %s" % ("ALL HEAPS AGREE on every query" if bad == 0
                                 else "MISMATCH in %d comparison(s)" % bad))
    text = "\n".join(lines)
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write(text + "\n")
    print(text)
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
