#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""由 results/results.csv 生成 report/tables.typ（可直接被 report.typ include）。

把表格生成放在 Python 里的好处：
  * 数字格式化、千位分隔符、排序、单位换算都在一处控制；
  * 报告正文只需引用 `#tbl_p2p` 之类的名字，重新跑实验后报告自动更新。
"""
import os
import sys

import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RESULTS = os.path.join(ROOT, "results", "results.csv")
OUT = os.path.join(ROOT, "report", "tables.typ")

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
    """生成 #table(...)；header/rows 中的元素若为 ('raw', s) 则按 Typst 代码插入。"""
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
        print("missing", RESULTS)
        return 1
    df = pd.read_csv(RESULTS)
    for c in ["nodes", "arcs", "runs", "pops", "pushes", "decrease_keys",
              "arc_scans", "cmps", "settled", "unreachable"]:
        df[c] = pd.to_numeric(df[c], errors="coerce")
    df["avg_ms"] = pd.to_numeric(df["avg_ms"], errors="coerce")

    nets = list(df.sort_values("nodes")["net"].drop_duplicates())
    out = []

    # ---------------- 表 1：数据集 ----------------
    rows1 = []
    for n in nets:
        d = df[(df["net"] == n) & (df["mode"] == "p2p") & (df["heap"] == "binary")]
        if d.empty:
            continue
        rows1.append([n, group(d["nodes"].iloc[0]), group(d["arcs"].iloc[0])])
    out.append("#let tbl_dataset = " + typst_table(["数据集", "结点数", "弧数"], rows1) + "\n")

    # ---------------- 表 1b：数据集 + 官方下载链接 ----------------
    rows1b = []
    order = {"NY": 0, "BAY": 1, "COL": 2, "FLA": 3, "NW": 4, "NE": 5, "CAL": 6,
             "LKS": 7, "E": 8, "W": 9, "CTR": 10, "USA": 11}
    for n in sorted(nets, key=lambda x: order.get(x, 99)):
        d = df[(df["net"] == n) & (df["mode"] == "p2p") & (df["heap"] == "binary")]
        if d.empty:
            continue
        url = ("http://www.diag.uniroma1.it/challenge9/data/USA-road-d/"
               "USA-road-d.%s.gr.gz" % n)
        rows1b.append([n, group(d["nodes"].iloc[0]), group(d["arcs"].iloc[0]),
                       ("raw", "link(\"%s\")" % url)])
    out.append("#let tbl_links = " +
               typst_table(["网络", "结点数", "弧数", "下载链接（.gr.gz，物理距离版本）"],
                           rows1b, align=["left", "right", "right", "left"]) + "\n")
    out.append('''#let link_note = "下载页：" + link("http://www.diag.uniroma1.it/challenge9/download.shtml")'''.strip() + "\n")

    # ---------------- 表 2 / 3：两种模式的运行时间 ----------------
    def runtime_table(mode, label):
        rows = []
        for n in nets:
            cells = [n]
            best = None
            for h in HEAPS:
                d = df[(df["net"] == n) & (df["mode"] == mode) & (df["heap"] == h)]
                v = float(d["avg_ms"].iloc[0]) if len(d) else float("nan")
                cells.append(fnum(v, 2))
                if h in ("binary", "binomial", "fibonacci"):
                    best = v if best is None else min(best, v)
            rows.append(cells)
        return typst_table([label, "二叉堆", "二项堆", "斐波那契堆", "二叉堆(惰性)"], rows)

    out.append("#let tbl_p2p = " + runtime_table("p2p", "数据集") + "\n")
    out.append("#let tbl_full = " + runtime_table("full", "数据集") + "\n")

    # ---------------- 表 4：堆操作计数（最大数据集，p2p） ----------------
    big = nets[-1]
    rows4 = []
    for h in HEAPS:
        d = df[(df["net"] == big) & (df["mode"] == "p2p") & (df["heap"] == h)]
        if d.empty:
            continue
        runs = float(d["runs"].iloc[0])
        rows4.append([
            HEAP_LABEL[h],
            fnum(d["pops"].iloc[0] / runs, 0),
            fnum(d["pushes"].iloc[0] / runs, 0),
            fnum(d["decrease_keys"].iloc[0] / runs, 0),
            fnum(d["arc_scans"].iloc[0] / runs, 0),
            fnum(d["cmps"].iloc[0] / runs, 0),
            fnum(d["cmps"].iloc[0] / max(1.0, d["settled"].iloc[0]), 2),
        ])
    out.append("#let tbl_ops = " +
               typst_table(["实现", "extractMin", "insert", "decreaseKey",
                            "弧扫描", "关键字比较", "比较/确定结点"], rows4) + "\n")

    # ---------------- 表 5：完整单源模式的操作计数 ----------------
    rows5 = []
    for h in HEAPS:
        d = df[(df["net"] == big) & (df["mode"] == "full") & (df["heap"] == h)]
        if d.empty:
            continue
        runs = float(d["runs"].iloc[0])
        rows5.append([
            HEAP_LABEL[h],
            fnum(d["pops"].iloc[0] / runs, 0),
            fnum(d["pushes"].iloc[0] / runs, 0),
            fnum(d["decrease_keys"].iloc[0] / runs, 0),
            fnum(d["cmps"].iloc[0] / runs, 0),
            fnum(d["cmps"].iloc[0] / max(1.0, d["settled"].iloc[0]), 2),
        ])
    out.append("#let tbl_ops_full = " +
               typst_table(["实现", "extractMin", "insert", "decreaseKey",
                            "关键字比较", "比较/确定结点"], rows5) + "\n")

    # ---------------- 关键结论用的数值 ----------------
    def val(net, heap, mode, col):
        d = df[(df["net"] == net) & (df["heap"] == heap) & (df["mode"] == mode)]
        return float(d[col].iloc[0]) if len(d) else float("nan")

    p2p = df[df["mode"] == "p2p"]
    ratios = {}
    for n in nets:
        b = val(n, "binary", "p2p", "avg_ms")
        bi = val(n, "binomial", "p2p", "avg_ms")
        f = val(n, "fibonacci", "p2p", "avg_ms")
        lz = val(n, "binary-lazy", "p2p", "avg_ms")
        if b == b:
            ratios[n] = (bi / b, f / b, lz / b)
    if ratios:
        out.append("#let ratio_binom = \"%.2f\"" % (sum(r[0] for r in ratios.values()) / len(ratios)))
        out.append("#let ratio_fib = \"%.2f\"" % (sum(r[1] for r in ratios.values()) / len(ratios)))
        out.append("#let ratio_lazy = \"%.2f\"" % (sum(r[2] for r in ratios.values()) / len(ratios)))
    nq = int(df[(df["heap"] == "binary") & (df["mode"] == "p2p")]["runs"].max())
    out.append("#let n_queries = \"%d\"" % nq)
    out.append("#let n_datasets = \"%d\"" % len(nets))
    out.append("#let largest_net = \"%s\"" % big)
    out.append("#let nodes_min = \"%s\"" % group(df["nodes"].min()))
    out.append("#let nodes_max = \"%s\"" % group(df["nodes"].max()))
    out.append("#let arcs_max = \"%s\"" % group(df["arcs"].max()))
    out.append("#let size_ratio = \"%.1f\"" % (df["nodes"].max() / df["nodes"].min()))

    # 用于正文的量化指标（都取自最大的数据集）
    def one(heap, mode, col):
        d = df[(df["net"] == big) & (df["heap"] == heap) & (df["mode"] == mode)]
        return float(d[col].iloc[0]) if len(d) else float("nan")

    pops = one("binary", "p2p", "pops")
    decs = one("binary", "p2p", "decrease_keys")
    settled = one("binary", "p2p", "settled")
    arcs = one("binary", "p2p", "arc_scans")
    out.append("#let dec_per_pop = \"%.3f\"" % (decs / pops))
    out.append("#let dec_inv = \"%.1f\"" % (pops / decs))
    out.append("#let settled_per_query = \"%s\"" % group(settled / one("binary", "p2p", "runs")))
    out.append("#let arcs_per_query = \"%s\"" % group(arcs / one("binary", "p2p", "runs")))
    lz = df[(df["net"] == big) & (df["heap"] == "binary-lazy") & (df["mode"] == "p2p")]
    if len(lz):
        out.append("#let lazy_pop_ratio = \"%.2f\"" % (float(lz["pops"].iloc[0]) / pops))
    # 完整单源模式下每次 SSSP 的 decrease-key 次数（用于第 4 章讨论）
    f_pop = one("binary", "full", "pops")
    f_dec = one("binary", "full", "decrease_keys")
    out.append("#let full_dec_per_pop = \"%.3f\"" % (f_dec / f_pop))
    # 每次 extract-min 的关键字比较次数（用于解释「比较更少但更慢」）
    for h, name in (("binary", "bin"), ("binomial", "binom"), ("fibonacci", "fib"),
                    ("binary-lazy", "lazy")):
        p = one(h, "p2p", "pops")
        c = one(h, "p2p", "cmps")
        if p == p and c == c:
            out.append("#let cmp_%s = \"%.1f\"" % (name, c / p))

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        f.write("\n".join(out) + "\n")
    print("wrote", OUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
