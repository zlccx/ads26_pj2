#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# =============================================================================
#  plot.py —— 实验结果统一绘图脚本（生成高质量 SVG + PNG 图表）
#
#  图表清单：
#    fig1_p2p_runtime      : 点对查询（P2P）平均耗时随道路网络规模变化曲线
#    fig2_full_runtime     : 完整单源最短路（Full SSSP）耗时随规模变化曲线
#    fig3_relative         : 各数据集上三种堆相对于二叉堆的耗时倍率
#    fig4_ops              : 核心堆操作次数（Pops, Pushes, Decs）与比较次数分布
# =============================================================================

import os
import sys
import glob
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, "results")
FIG = os.path.join(RES, "figs")

DISPLAY = {
    "binary": "Binary heap",
    "binomial": "Binomial heap",
    "fibonacci": "Fibonacci heap",
    "binary-lazy": "Binary heap (lazy)",
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
        fig.savefig(os.path.join(FIG, f"{name}.{ext}"), dpi=250, bbox_inches="tight")
    plt.close(fig)
    print(f"  [Plot] Generated {name}.png and {name}.svg")

def load_results():
    csv_path = os.path.join(RES, "results.csv")
    if not os.path.exists(csv_path):
        print(f"[Error] Missing {csv_path}")
        sys.exit(1)
    df = pd.read_csv(csv_path)
    for c in ("nodes", "arcs", "runs", "pops", "pushes", "decrease_keys",
              "arc_scans", "cmps", "settled", "unreachable"):
        if c in df:
            df[c] = pd.to_numeric(df[c], errors="coerce")
    df["total_ms"] = pd.to_numeric(df["total_ms"], errors="coerce")
    df["avg_ms"] = pd.to_numeric(df["avg_ms"], errors="coerce")
    return df

# 1. P2P 耗时随规模变化曲线
def plot_fig1(df):
    sub = df[df["mode"] == "p2p"].copy()
    fig, ax = plt.subplots(figsize=(6.8, 4.2))
    for h in ("binary-lazy", "binary", "binomial", "fibonacci"):
        d = sub[sub["heap"] == h].sort_values("nodes")
        if d.empty:
            continue
        ax.plot(d["nodes"] / 1e3, d["avg_ms"],
                marker=MARKER[h], color=COLOR[h], label=DISPLAY[h],
                linewidth=1.8, markersize=5.5)
    ax.set_xlabel("Vertices (|V| in thousands, 10³)")
    ax.set_ylabel("Average Query Time (ms)")
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend(frameon=True)
    save(fig, "fig1_p2p_runtime")

# 2. 全图单源最短路耗时随规模变化曲线
def plot_fig2(df):
    sub = df[df["mode"] == "full"].copy()
    fig, ax = plt.subplots(figsize=(6.8, 4.2))
    for h in ("binary-lazy", "binary", "binomial", "fibonacci"):
        d = sub[sub["heap"] == h].sort_values("nodes")
        if d.empty:
            continue
        ax.plot(d["nodes"] / 1e3, d["avg_ms"],
                marker=MARKER[h], color=COLOR[h], label=DISPLAY[h],
                linewidth=1.8, markersize=5.5)
    ax.set_xlabel("Vertices (|V| in thousands, 10³)")
    ax.set_ylabel("Full SSSP Runtime (ms)")
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend(frameon=True)
    save(fig, "fig2_full_runtime")

# 3. 相对二叉堆的耗时倍率
def plot_fig3(df):
    sub = df[df["mode"] == "p2p"].copy()
    nets = list(sub["net"].drop_duplicates())
    base = sub[sub["heap"] == "binary"].set_index("net")["avg_ms"].to_dict()

    x = np.arange(len(nets))
    width = 0.22
    fig, ax = plt.subplots(figsize=(8.0, 4.2))

    for idx, h in enumerate(("binary-lazy", "binomial", "fibonacci")):
        vals = []
        for n in nets:
            row = sub[(sub["net"] == n) & (sub["heap"] == h)]
            if not row.empty and n in base and base[n] > 0:
                vals.append(row["avg_ms"].values[0] / base[n])
            else:
                vals.append(np.nan)
        ax.bar(x + (idx - 1) * width, vals, width, label=DISPLAY[h], color=COLOR[h])

    ax.axhline(1.0, color="#1f77b4", linestyle="--", linewidth=1.2, label="Binary heap (baseline 1.0x)")
    ax.set_xticks(x)
    ax.set_xticklabels(nets)
    ax.set_xlabel("DIMACS USA Road Network")
    ax.set_ylabel("Slowdown Ratio vs. Binary Heap")
    ax.grid(True, axis="y", linestyle="--", alpha=0.5)
    ax.legend(frameon=True)
    save(fig, "fig3_relative")

# 4. 操作次数与比较次数
def plot_fig4(df):
    sub = df[df["mode"] == "p2p"].copy()
    nets = list(sub["net"].drop_duplicates())
    dec_rates = []
    cmp_per_pop_bin = []
    cmp_per_pop_fib = []

    for n in nets:
        b = sub[(sub["net"] == n) & (sub["heap"] == "binary")]
        f = sub[(sub["net"] == n) & (sub["heap"] == "fibonacci")]
        if not b.empty:
            r = b.iloc[0]
            dec_rate = (r["decrease_keys"] / r["settled"] * 100.0) if r["settled"] > 0 else 0
            dec_rates.append(dec_rate)
            cmp_per_pop_bin.append(r["cmps"] / r["pops"] if r["pops"] > 0 else 0)
        else:
            dec_rates.append(0)
            cmp_per_pop_bin.append(0)
        if not f.empty:
            r = f.iloc[0]
            cmp_per_pop_fib.append(r["cmps"] / r["pops"] if r["pops"] > 0 else 0)
        else:
            cmp_per_pop_fib.append(0)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10.5, 4.0))
    x = np.arange(len(nets))
    ax1.bar(x, dec_rates, color="#3b528b", width=0.55)
    ax1.set_xticks(x)
    ax1.set_xticklabels(nets)
    ax1.set_ylabel("Decrease-Key per 100 Settled Vertices (%)")
    ax1.set_title("Decrease-Key Frequency on Road Networks")
    ax1.grid(True, axis="y", linestyle="--", alpha=0.5)

    w = 0.35
    ax2.bar(x - w / 2, cmp_per_pop_bin, w, label="Binary Heap", color=COLOR["binary"])
    ax2.bar(x + w / 2, cmp_per_pop_fib, w, label="Fibonacci Heap", color=COLOR["fibonacci"])
    ax2.set_xticks(x)
    ax2.set_xticklabels(nets)
    ax2.set_ylabel("Comparisons per Pop")
    ax2.set_title("Internal Key Comparisons per Extract-Min")
    ax2.legend()
    ax2.grid(True, axis="y", linestyle="--", alpha=0.5)

    plt.tight_layout()
    save(fig, "fig4_ops")

def main():
    df = load_results()
    print("Generating figures from experimental results...")
    plot_fig1(df)
    plot_fig2(df)
    plot_fig3(df)
    plot_fig4(df)
    print("All 4 figures successfully generated in results/figs/")

if __name__ == "__main__":
    main()
