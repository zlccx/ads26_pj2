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
#    fig5_vs_settled       : CAL 数据集上单次查询耗时 vs 实际搜索规模散点图
#    fig6_synth_decs       : [加分实验3] 人工三层图在零减键与高减键压力下的耗时对比
#    fig7_reset_ablation   : [加分实验4] Full Reset vs Partial Reset 在不同搜索范围的耗时消融
#    fig8_search_size_groups: [加分实验2] Small / Medium / Large 搜索规模分组下的相对倍率
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

# 5. 耗时 vs 单次查询实际搜索规模（CAL 数据集）
def plot_fig5():
    cal_bin = os.path.join(RES, "perquery", "CAL.binary.csv")
    cal_fib = os.path.join(RES, "perquery", "CAL.fibonacci.csv")
    if not (os.path.exists(cal_bin) and os.path.exists(cal_fib)):
        return

    db = pd.read_csv(cal_bin)
    df = pd.read_csv(cal_fib)

    fig, ax = plt.subplots(figsize=(6.8, 4.2))
    ax.scatter(db["settled"] / 1e3, db["ms"], s=10, alpha=0.45, color=COLOR["binary"], label="Binary Heap")
    ax.scatter(df["settled"] / 1e3, df["ms"], s=10, alpha=0.45, color=COLOR["fibonacci"], label="Fibonacci Heap")
    ax.set_xlabel("Settled Vertices in Query (thousands, 10³)")
    ax.set_ylabel("Query Runtime (ms)")
    ax.set_title("Runtime vs. Search Scope on CAL Network")
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend(frameon=True)
    save(fig, "fig5_vs_settled")

# 6. [加分实验 3] 人工三层图高低减键频率对比柱状图
def plot_fig6():
    synth_csv = os.path.join(RES, "synth_results.csv")
    if not os.path.exists(synth_csv):
        return
    df = pd.read_csv(synth_csv)
    
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(11.0, 4.2))
    heaps = ["BinaryHeap", "LazyBinaryHeap", "BinomialHeap", "FibonacciHeap"]
    labels = ["Binary", "Lazy Binary", "Binomial", "Fibonacci"]
    
    # 提取 LowDec 与 HighDec 的 avg_ms
    low_ms = [df[(df["heap"] == h) & (df["case"] == "LowDec")]["avg_ms"].values[0] for h in heaps]
    high_ms = [df[(df["heap"] == h) & (df["case"] == "HighDec")]["avg_ms"].values[0] for h in heaps]
    
    x = np.arange(len(heaps))
    w = 0.35
    ax1.bar(x - w/2, low_ms, w, label="Case A: Low-Dec (0 decs)", color="#4575b4")
    ax1.bar(x + w/2, high_ms, w, label="Case B: High-Dec (897k decs)", color="#d73027")
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels, rotation=15)
    ax1.set_ylabel("Execution Time (ms)")
    ax1.set_title("Runtime under Controlled Decrease-Key Frequency")
    ax1.legend()
    ax1.grid(True, axis="y", linestyle="--", alpha=0.5)
    
    # 对比比较次数 cmps（对数坐标轴）
    low_cmps = [df[(df["heap"] == h) & (df["case"] == "LowDec")]["cmps"].values[0] for h in heaps]
    high_cmps = [df[(df["heap"] == h) & (df["case"] == "HighDec")]["cmps"].values[0] for h in heaps]
    
    ax2.bar(x - w/2, low_cmps, w, label="Case A (0 decs)", color="#4575b4")
    ax2.bar(x + w/2, high_cmps, w, label="Case B (897k decs)", color="#d73027")
    ax2.set_yscale("log")
    ax2.set_xticks(x)
    ax2.set_xticklabels(labels, rotation=15)
    ax2.set_ylabel("Key Comparisons (log scale)")
    ax2.set_title("Comparisons: Lazy Binary Heap Explodes under High-Dec")
    ax2.legend()
    ax2.grid(True, axis="y", linestyle="--", alpha=0.5)
    
    plt.tight_layout()
    save(fig, "fig6_synth_decs")

# 7. [加分实验 4] Reset 消融实验柱状图
def plot_fig7():
    reset_csv = os.path.join(RES, "reset_results.csv")
    if not os.path.exists(reset_csv):
        return
    df = pd.read_csv(reset_csv)
    
    fig, ax = plt.subplots(figsize=(6.2, 4.0))
    scopes = df["scope"].tolist()
    full_times = df["full_reset_ms"].tolist()
    partial_times = df["partial_reset_ms"].tolist()
    
    x = np.arange(len(scopes))
    w = 0.35
    ax.bar(x - w/2, full_times, w, label="Full Reset (O(V))", color="#9970ab")
    ax.bar(x + w/2, partial_times, w, label="Partial Reset (O(Settled))", color="#5aae61")
    ax.set_xticks(x)
    ax.set_xticklabels([f"{s} Search Scope" for s in scopes])
    ax.set_ylabel("Average Query Time (ms)")
    ax.set_title("Ablation Study: Full Reset vs. Partial Reset (NY Graph)")
    ax.grid(True, axis="y", linestyle="--", alpha=0.5)
    
    # 标注加速比
    for i in range(len(scopes)):
        ratio = df["speedup_ratio"].iloc[i]
        h = max(full_times[i], partial_times[i])
        ax.text(x[i], h * 1.05, f"{ratio:.2f}x Speedup", ha="center", weight="bold", color="#1b7837")
    
    ax.legend()
    save(fig, "fig7_reset_ablation")

# 8. [加分实验 2] Small / Medium / Large 搜索规模分组下的相对倍率
def plot_fig8():
    qg_csv = os.path.join(RES, "query_groups.csv")
    if not os.path.exists(qg_csv):
        return
    df = pd.read_csv(qg_csv)
    
    fig, ax = plt.subplots(figsize=(7.2, 4.2))
    groups = ["Small", "Medium", "Large"]
    x = np.arange(len(groups))
    
    for h in ("binary-lazy", "binomial", "fibonacci"):
        ratios = [df[(df["group"] == g) & (df["heap"] == h)]["ratio_to_binary"].values[0] for g in groups]
        ax.plot(x, ratios, marker=MARKER[h], color=COLOR[h], label=DISPLAY[h], linewidth=2.0, markersize=7)
        
    ax.axhline(1.0, color="#1f77b4", linestyle="--", linewidth=1.2, label="Binary Heap (1.0x baseline)")
    ax.set_xticks(x)
    ax.set_xticklabels(["Small (<33% settled)", "Medium (33%-66%)", "Large (>66% settled)"])
    ax.set_xlabel("Query Search Size Classification (CAL Graph)")
    ax.set_ylabel("Runtime Ratio vs. Binary Heap")
    ax.set_title("Relative Slowdown across Different Search Sizes")
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend(frameon=True)
    save(fig, "fig8_search_size_groups")

def main():
    df = load_results()
    print("Generating figures from experimental results...")
    plot_fig1(df)
    plot_fig2(df)
    plot_fig3(df)
    plot_fig4(df)
    plot_fig5()
    plot_fig6()
    plot_fig7()
    plot_fig8()
    print("All 8 figures successfully generated in results/figs/")

if __name__ == "__main__":
    main()
