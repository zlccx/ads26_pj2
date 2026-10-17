#!/usr/bin/env python3
# =============================================================================
#  analyze_groups.py —— 加分实验 2：查询搜索规模分级分析脚本（纯标准库实现）
#
#  将 1000 次查询按照确定顶点数（Settled Vertices）划分为 Small, Medium, Large
#  三组，统计四种实现在不同搜索范围下的平均查询耗时与相对倍率。
# =============================================================================

import os
import csv

def read_perquery_csv(path):
    rows = []
    with open(path, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for r in reader:
            rows.append({
                "idx": int(r["idx"]),
                "src": int(r["src"]),
                "dst": int(r["dst"]),
                "dist": int(r["dist"]),
                "ms": float(r["ms"]),
                "settled": int(r["settled"]),
                "pops": int(r["pops"])
            })
    return rows

def analyze_dataset(net="CAL", perquery_dir="results/perquery", out_csv="results/query_groups.csv"):
    heaps = ["binary", "binary-lazy", "binomial", "fibonacci"]
    data = {}
    
    for h in heaps:
        path = os.path.join(perquery_dir, f"{net}.{h}.csv")
        if not os.path.exists(path):
            print(f"[Warning] File not found: {path}")
            return
        data[h] = read_perquery_csv(path)
        
    n = len(data["binary"])
    # 按照 binary 跑出来的 settled 顶点数排序
    sorted_indices = sorted(range(n), key=lambda i: data["binary"][i]["settled"])
    
    q1 = n // 3
    q2 = 2 * n // 3
    
    groups = {
        "Small": sorted_indices[:q1],
        "Medium": sorted_indices[q1:q2],
        "Large": sorted_indices[q2:]
    }
    
    records = []
    for gname, idxs in groups.items():
        avg_settled = sum(data["binary"][i]["settled"] for i in idxs) / len(idxs)
        bin_time = sum(data["binary"][i]["ms"] for i in idxs) / len(idxs)
        
        for h in heaps:
            t = sum(data[h][i]["ms"] for i in idxs) / len(idxs)
            ratio = t / bin_time if bin_time > 0 else 1.0
            records.append({
                "net": net,
                "group": gname,
                "heap": h,
                "count": len(idxs),
                "avg_settled": round(avg_settled, 1),
                "avg_ms": round(t, 4),
                "ratio_to_binary": round(ratio, 3)
            })
            
    with open(out_csv, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["net", "group", "heap", "count", "avg_settled", "avg_ms", "ratio_to_binary"])
        writer.writeheader()
        writer.writerows(records)
        
    print(f"[Success] Group analysis for {net} written to {out_csv}")
    print(f"{'Group':<8} | {'Heap':<12} | {'Avg Settled':<12} | {'Avg Ms':<10} | {'Ratio to Binary':<15}")
    print("-" * 65)
    for r in records:
        print(f"{r['group']:<8} | {r['heap']:<12} | {r['avg_settled']:<12} | {r['avg_ms']:<10.4f} | {r['ratio_to_binary']:<15.3f}")

if __name__ == "__main__":
    analyze_dataset("CAL")
