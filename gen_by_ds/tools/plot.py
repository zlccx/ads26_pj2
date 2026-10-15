#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""由 results/results.csv 与 results/perquery/*.csv 生成报告用图（PNG + SVG）。

图（全部为英文标注，避免字体问题；插入 Typst 后由正文中文图题说明）：
    fig1_p2p_runtime.png     点对查询平均耗时 vs 图规模
    fig2_full_runtime.png    完整单源最短路平均耗时 vs 图规模
    fig3_relative.png        各数据集上相对最快实现的耗时倍率
    fig4_ops.png             堆操作次数 / 比较次数指标
    fig5_vs_settled.png      耗时 vs 单次查询实际搜索规模（最大数据集）
"""
import glob
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
FIG = os.path.join(RES, "figs")

DISPLAY = {
    "binary": "Binary heap",
    "binomial": "Binomial heap",
    "fibonacci": "Fibonacci heap",
    "binary-lazy": "Binary heap (lazy deletion)",
}
COLOR = {
    "binary": "#1f77b4",
    "binomial": "#2ca02c",
    "fibonacci": "#d62728",
    "binary-lazy": "#ff7f0e",
}
MARKER = {"binary": "o", "binomial": "s", "fibonacci": "^", "binary-lazy": "D"}


def save(fig, name):
    os.makedirs(FIG, exist_ok=True)
    for ext in ("png", "svg"):
        fig.savefig(os.path.join(FIG, "%s.%s" % (name, ext)), dpi=200, bbox_inches="tight")
    plt.close(fig)
    print("  wrote figs/%s.png|.svg" % name)


def load():
    csv = os.path.join(RES, "results.csv")
    if not os.path.exists(csv):
        print("missing", csv)
        sys.exit(1)
    df = pd.read_csv(csv)
    for c in ("nodes", "arcs", "runs", "pops", "pushes", "decrease_keys",
              "arc_scans", "cmps", "settled", "unreachable"):
        if c in df:
            df[c] = pd.to_numeric(df[c], errors="coerce")
    df["total_ms"] = pd.to_numeric(df["total_ms"], errors="coerce")
    df["avg_ms"] = pd.to_numeric(df["avg_ms"], errors="coerce")
    return df


def runtime_vs_size(df, mode, name, ylabel, title):
    sub = df[df["mode"] == mode]
    if sub.empty:
        print("  (skip %s: no rows)" % name)
        return
    fig, ax = plt.subplots(figsize=(6.6, 4.4))
    for h in ["binary", "binomial", "fibonacci", "binary-lazy"]:
        s = sub[sub["heap"] == h].sort_values("nodes")
        if s.empty:
            continue
        ax.plot(s["nodes"] / 1e6, s["avg_ms"], marker=MARKER[h], color=COLOR[h],
                label=DISPLAY[h], linewidth=1.8, markersize=6)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("graph size (#nodes, millions)")
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(True, which="both", ls=":", alpha=0.5)
    ax.legend(fontsize=8)
    save(fig, name)


def relative_bars(df, mode, name, ylabel, title):
    sub = df[df["mode"] == mode].copy()
    if sub.empty:
        return
    nets = list(sub.sort_values("nodes")["net"].drop_duplicates())
    heaps = [h for h in ["binary", "binomial", "fibonacci", "binary-lazy"]
             if h in set(sub["heap"])]
    fig, ax = plt.subplots(figsize=(7.6, 4.2))
    width = 0.8 / len(heaps)
    for i, h in enumerate(heaps):
        vals = []
        for n in nets:
            r = sub[(sub["net"] == n) & (sub["heap"] == h)]
            vals.append(float(r["avg_ms"].iloc[0]) if len(r) else float("nan"))
        best = [float(sub[sub["net"] == n]["avg_ms"].min()) for n in nets]
        rel = [v / b for v, b in zip(vals, best)]
        xs = [j + i * width - 0.4 + width / 2 for j in range(len(nets))]
        ax.bar(xs, rel, width=width * 0.92, color=COLOR[h], label=DISPLAY[h])
    ax.axhline(1.0, color="black", lw=0.8, ls="--")
    ax.set_xticks(range(len(nets)))
    ax.set_xticklabels(nets)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    ax.grid(True, axis="y", ls=":", alpha=0.5)
    ax.legend(fontsize=8, ncol=2)
    save(fig, name)


def ops_figure(df, name):
    sub = df[df["mode"] == "p2p"].copy()
    if sub.empty:
        return
    biggest = sub.sort_values("nodes")["net"].iloc[-1]
    s = sub[sub["net"] == biggest]
    heaps = [h for h in ["binary", "binomial", "fibonacci", "binary-lazy"]
             if h in set(s["heap"])]
    labels = [DISPLAY[h] for h in heaps]
    per_settled = [float(s[s["heap"] == h]["cmps"].iloc[0]) /
                   max(1.0, float(s[s["heap"] == h]["settled"].iloc[0])) for h in heaps]
    per_pop = [float(s[s["heap"] == h]["cmps"].iloc[0]) /
               max(1.0, float(s[s["heap"] == h]["pops"].iloc[0])) for h in heaps]
    decs = [float(s[s["heap"] == h]["decrease_keys"].iloc[0]) /
            float(s[s["heap"] == h]["runs"].iloc[0]) for h in heaps]

    fig, axes = plt.subplots(1, 3, figsize=(12.4, 3.6))
    for ax, vals, title, ylab in ((axes[0], per_settled, "key comparisons per settled node", "#comparisons"),
                                  (axes[1], per_pop, "key comparisons per extract-min", "#comparisons"),
                                  (axes[2], decs, "decrease-key calls per query", "#calls")):
        ax.bar(labels, vals, color=[COLOR[h] for h in heaps])
        ax.set_title(title, fontsize=9)
        ax.set_ylabel(ylab, fontsize=8)
        ax.grid(True, axis="y", ls=":", alpha=0.5)
        ax.tick_params(axis="x", labelrotation=18, labelsize=8)
    fig.suptitle("%s: per-operation heap work (1000 point-to-point queries)" % biggest,
                 fontsize=10)
    save(fig, name)


def vs_settled(name, max_points=600):
    """按「单次查询实际确定的最短距离结点数」分桶，比较各堆的耗时。"""
    files = sorted(glob.glob(os.path.join(RES, "perquery", "*.csv")))
    if not files:
        print("  (skip %s: no per-query dumps)" % name)
        return
    data = {}
    for f in files:
        base = os.path.basename(f)[:-4]
        net, heap = base.rsplit(".", 1)
        if heap not in DISPLAY:
            continue
        data[(net, heap)] = pd.read_csv(f)
    if not data:
        return
    # 选择有最多 dump 的（也就是最大的）数据集
    nets = sorted({k[0] for k in data})
    net = nets[-1]

    fig, ax = plt.subplots(figsize=(6.6, 4.4))
    for h in ["binary", "binomial", "fibonacci", "binary-lazy"]:
        if (net, h) not in data:
            continue
        d = data[(net, h)]
        d = d[d["dist"] >= 0]                       # 丢弃不可达的点对
        if d.empty:
            continue
        edges = [0, 5e3, 2e4, 1e5, 3e5, 1e6, 3e6, 1e7, 1e8]
        labels, means = [], []
        for a, b in zip(edges[:-1], edges[1:]):
            sel = d[(d["settled"] >= a) & (d["settled"] < b)]["ms"]
            if len(sel) >= 3:
                labels.append(0.5 * (a + b))
                means.append(float(sel.mean()))
        if labels:
            ax.plot(labels, means, marker=MARKER[h], color=COLOR[h],
                    label=DISPLAY[h], linewidth=1.8, markersize=6)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("nodes settled by the query (search space size)")
    ax.set_ylabel("mean query time (ms)")
    ax.set_title("%s: query time vs. search space size" % net)
    ax.grid(True, which="both", ls=":", alpha=0.5)
    ax.legend(fontsize=8)
    save(fig, name)


def main():
    df = load()
    nets = list(df.sort_values("nodes")["net"].drop_duplicates())
    print("datasets:", " ".join(nets))
    runtime_vs_size(df, "p2p", "fig1_p2p_runtime",
                    "mean time per query (ms)",
                    "Point-to-point Dijkstra: mean query time vs. graph size")
    runtime_vs_size(df, "full", "fig2_full_runtime",
                    "mean time per single-source run (ms)",
                    "Full single-source Dijkstra: mean time vs. graph size")
    relative_bars(df, "p2p", "fig3_relative",
                  "time / fastest implementation",
                  "Relative cost of each heap (point-to-point queries)")
    ops_figure(df, "fig4_ops")
    vs_settled("fig5_vs_settled")
    return 0


if __name__ == "__main__":
    sys.exit(main())
