#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  make_report_tables.py —— 自动生成 documents/tables.typ 数据表格脚本
# =============================================================================

import os
import sys
import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RESULTS = os.path.join(ROOT, "results", "results.csv")
OUT = os.path.join(ROOT, "documents", "tables.typ")

HEAPS = ["binary", "binomial", "fibonacci", "binary-lazy"]
HEAP_LABEL = {
    "binary": "二叉堆",
    "binomial": "二项堆",
    "fibonacci": "斐波那契堆",
    "binary-lazy": "二叉堆(惰性)",
}

def group(n):
    return "{:,}".format(int(round(n)))

def fnum(x, d=2):
    return ("{:,.%df}" % d).format(float(x))

def typst_table(header, rows, align=None, column_gutter="6pt"):
    n = len(header)
    align = align or (["left"] + ["right"] * (n - 1))

    def cell(c):
        if isinstance(c, tuple) and c and c[0] == "raw":
            return c[1]
        return '"%s"' % str(c).replace("\\", "\\\\").replace('"', '\\"')

    lines = ["table(",
             "  columns: %d," % n,
             "  align: (%s)," % ", ".join(align),
             "  column-gutter: %s," % column_gutter,
             "  stroke: 0.4pt + luma(150),",
             "  " + ", ".join("strong(%s)" % cell(h) for h in header) + ",",
             "  ..(("]
    for i, r in enumerate(rows):
        lines.append("    " + ", ".join(cell(c) for c in r) + ("," if i + 1 < len(rows) else ""))
    lines.append("  )).flatten(),")
    lines.append(")")
    return "\n".join(lines)

def main():
    if not os.path.exists(RESULTS):
        print(f"[Error] Missing {RESULTS}")
        return 1
    df = pd.read_csv(RESULTS)
    for c in ["nodes", "arcs", "runs", "pops", "pushes", "decrease_keys",
              "arc_scans", "cmps", "settled", "unreachable"]:
        df[c] = pd.to_numeric(df[c], errors="coerce")
    df["avg_ms"] = pd.to_numeric(df["avg_ms"], errors="coerce")

    nets = list(df.sort_values("nodes")["net"].drop_duplicates())
    out = []

    # 表 1：数据集
    rows1 = []
    for n in nets:
        d = df[(df["net"] == n) & (df["mode"] == "p2p") & (df["heap"] == "binary")]
        if d.empty:
            continue
        rows1.append([n, group(d["nodes"].iloc[0]), group(d["arcs"].iloc[0])])
    out.append("#let tbl_dataset = " + typst_table(["数据集", "结点数", "弧数"], rows1) + "\n")

    # 表 1b：官方下载链接
    rows1b = []
    for n in nets:
        d = df[(df["net"] == n) & (df["mode"] == "p2p") & (df["heap"] == "binary")]
        if d.empty:
            continue
        url = ("http://www.diag.uniroma1.it/challenge9/data/USA-road-d/"
               "USA-road-d.%s.gr.gz" % n)
        rows1b.append([n, group(d["nodes"].iloc[0]), group(d["arcs"].iloc[0]),
                       ("raw", "link(\"%s\")" % url)])
    out.append("#let tbl_links = " +
               typst_table(["网络", "结点数", "弧数", "下载链接（.gr.gz 官方源）"],
                           rows1b, align=["left", "right", "right", "left"]) + "\n")
    out.append('''#let link_note = "官方下载索引页：" + link("http://www.diag.uniroma1.it/challenge9/download.shtml")''' + "\n")

    # 表 2 / 3：两种模式的运行时间
    def runtime_table(mode):
        rows = []
        for n in nets:
            row = [n]
            for h in HEAPS:
                sub = df[(df["net"] == n) & (df["mode"] == mode) & (df["heap"] == h)]
                if sub.empty:
                    row.append("-")
                else:
                    row.append(fnum(sub["avg_ms"].iloc[0], 3))
            rows.append(row)
        hdr = ["网络"] + [HEAP_LABEL[h] for h in HEAPS]
        return typst_table(hdr, rows)

    out.append("#let tbl_p2p = " + runtime_table("p2p") + "\n")
    out.append("#let tbl_full = " + runtime_table("full") + "\n")

    # 表 4：操作计数分析
    rows4 = []
    for n in nets:
        b = df[(df["net"] == n) & (df["mode"] == "p2p") & (df["heap"] == "binary")]
        f = df[(df["net"] == n) & (df["mode"] == "p2p") & (df["heap"] == "fibonacci")]
        if b.empty:
            continue
        rb = b.iloc[0]
        settled = rb["settled"]
        decs = rb["decrease_keys"]
        dec_rate = (decs / settled * 100.0) if settled > 0 else 0
        cmp_bin = rb["cmps"] / rb["pops"] if rb["pops"] > 0 else 0
        cmp_fib = f["cmps"].iloc[0] / f["pops"].iloc[0] if (not f.empty and f["pops"].iloc[0] > 0) else 0
        rows4.append([n, group(settled), group(decs), "%.2f%%" % dec_rate,
                      fnum(cmp_bin, 1), fnum(cmp_fib, 1)])
    hdr4 = ["网络", "确定顶点数", "减键次数", "每百点减键率", "二叉堆单次弹出比较", "斐波那契堆单次弹出比较"]
    out.append("#let tbl_ops = " + typst_table(hdr4, rows4) + "\n")

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print(f"[Success] Generated Typst tables to {OUT}")

if __name__ == "__main__":
    main()
