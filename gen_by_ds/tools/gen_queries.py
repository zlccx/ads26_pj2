#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""为每个 USA 公路网生成 1000 组随机 (s, t) 点对查询，写成 DIMACS .p2p 格式。

格式：
    c <network> random point-to-point queries
    p aux sp p2p 1000
    q <s> <t>

随机数种子由网络名派生并固定，因此：
  * 同一个网络的查询集可复现；
  * 不同网络的查询集互不相同（避免所有网络用同一批"编号"导致偏差）。
"""
import glob
import hashlib
import os
import random
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA = os.path.join(ROOT, "data")
QDIR = os.path.join(DATA, "queries")
N_QUERIES = 1000
BASE_SEED = 20241024


def read_node_count(path):
    """从 .gr 文件的问题行 'p sp n m' 中读出 n。"""
    with open(path, "r", errors="ignore") as f:
        for line in f:
            if line.startswith("p"):
                parts = line.split()
                if len(parts) >= 3:
                    return int(parts[2])
    raise RuntimeError("no problem line in " + path)


def main():
    os.makedirs(QDIR, exist_ok=True)
    graphs = sorted(glob.glob(os.path.join(DATA, "USA-road-d.*.gr")))
    if not graphs:
        print("no .gr files; run decompress.py first")
        return 1
    for g in graphs:
        net = os.path.basename(g).replace("USA-road-d.", "").replace(".gr", "")
        n = read_node_count(g)
        seed = BASE_SEED ^ (int(hashlib.md5(net.encode()).hexdigest()[:8], 16))
        rng = random.Random(seed)
        out = os.path.join(QDIR, net + ".p2p")
        with open(out, "w") as f:
            f.write("c %s: %d uniform random point-to-point queries\n" % (net, N_QUERIES))
            f.write("p aux sp p2p %d\n" % N_QUERIES)
            for _ in range(N_QUERIES):
                s = rng.randint(1, n)
                t = rng.randint(1, n)
                while t == s:
                    t = rng.randint(1, n)
                f.write("q %d %d\n" % (s, t))
        print("ok    %-6s n=%-9d seed=%-10d -> %s" % (net, n, seed, out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
