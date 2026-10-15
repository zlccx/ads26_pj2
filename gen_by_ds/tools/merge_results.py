#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""把 results/raw/*.csv 合并成 results/results.csv，并按数据集规模排序。"""
import glob
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "results", "raw")
DST = os.path.join(ROOT, "results", "results.csv")

HEADER = ("net,nodes,arcs,heap,mode,runs,total_ms,avg_ms,checksum,"
          "pops,pushes,decrease_keys,arc_scans,cmps,settled,unreachable").split(",")


def main():
    rows = []
    for f in sorted(glob.glob(os.path.join(RAW, "*.csv"))):
        with open(f) as fh:
            for line in fh:
                line = line.strip()
                if not line or line.startswith("net,"):
                    continue
                rows.append(line.split(","))
    if not rows:
        print("no raw results found in", RAW)
        return 1
    # 按 (结点数, 堆顺序, 模式) 排序，读起来更顺
    heap_order = {"binary": 0, "binomial": 1, "fibonacci": 2, "binary-lazy": 3}
    mode_order = {"p2p": 0, "full": 1}
    rows.sort(key=lambda r: (int(r[1]),
                             mode_order.get(r[4], 9),
                             heap_order.get(r[3], 9)))
    with open(DST, "w") as fh:
        fh.write(",".join(HEADER) + "\n")
        for r in rows:
            fh.write(",".join(r) + "\n")
    print("merged %d rows -> %s" % (len(rows), DST))
    return 0


if __name__ == "__main__":
    sys.exit(main())
