#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""在多个 USA 公路网上并行运行 bench.exe。

每个数据集单独写一份 CSV 到 results/raw/<NET>.csv，避免多进程同时追加同一文件，
最后用 merge_results.py 合并。

注意：同一次运行的 4 种堆实现一定在同一个进程里先后执行，因此它们之间的对比
是在完全相同的数据、完全相同的内存状态与几乎相同的机器负载下得到的。
"""
import argparse
import os
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BENCH = os.path.join(ROOT, "build", "bench.exe")


def graph_path(net):
    return os.path.join(ROOT, "data", "USA-road-d.%s.gr" % net)


def run_one(net, full_sources, heaps, timeout=None):
    g = graph_path(net)
    q = os.path.join(ROOT, "data", "queries", net + ".p2p")
    if not os.path.exists(g):
        return net, "missing graph", 0.0
    if not os.path.exists(q):
        return net, "missing queries", 0.0
    out = os.path.join(ROOT, "results", "raw", net + ".csv")
    log = os.path.join(ROOT, "results", "logs", net + ".log")
    dump = os.path.join(ROOT, "results", "perquery", net)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    os.makedirs(os.path.dirname(log), exist_ok=True)
    os.makedirs(os.path.dirname(dump), exist_ok=True)
    cmd = [BENCH, "--graph", g, "--net", net, "--p2p", q,
           "--heaps", heaps, "--full-sources", str(full_sources),
           "--out", out, "--dump", dump]
    t0 = time.time()
    with open(log, "w") as lf:
        subprocess.run(cmd, stdout=lf, stderr=subprocess.STDOUT, timeout=timeout)
    return net, "ok", time.time() - t0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("nets", nargs="*", help="数据集名字，如 NY BAY COL；留空表示自动探测")
    ap.add_argument("-j", "--jobs", type=int, default=3, help="并行度")
    ap.add_argument("--full-sources", type=int, default=16)
    ap.add_argument("--heaps", default="bin,binom,fib,lazy")
    ap.add_argument("--timeout", type=float, default=None)
    args = ap.parse_args()

    nets = args.nets
    if not nets:
        nets = []
        for f in sorted(os.listdir(os.path.join(ROOT, "data"))):
            if f.startswith("USA-road-d.") and f.endswith(".gr"):
                nets.append(f[len("USA-road-d."):-3])
    if not nets:
        print("no datasets found; run decompress.py first")
        return 1

    print("running %d dataset(s), parallelism=%d: %s" % (len(nets), args.jobs, " ".join(nets)))
    with ThreadPoolExecutor(max_workers=args.jobs) as ex:
        futs = [ex.submit(run_one, n, args.full_sources, args.heaps, args.timeout) for n in nets]
        for f in futs:
            net, status, dt = f.result()
            print("  %-6s %-16s %8.1f s" % (net, status, dt), flush=True)

    # 合并
    merge = os.path.join(ROOT, "tools", "merge_results.py")
    subprocess.run([sys.executable, merge])
    return 0


if __name__ == "__main__":
    sys.exit(main())
