#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""把 data/gz/USA-road-d.*.gr.gz 解压成 data/USA-road-d.*.gr（纯文本，供 C++ 程序直接读取）。"""
import glob
import gzip
import os
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GZ_DIR = os.path.join(ROOT, "data", "gz")
OUT_DIR = os.path.join(ROOT, "data")


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    files = sorted(glob.glob(os.path.join(GZ_DIR, "USA-road-d.*.gr.gz")))
    if not files:
        print("no .gr.gz found in", GZ_DIR)
        return 1
    for f in files:
        base = os.path.basename(f)[:-3]          # 去掉 .gz
        dst = os.path.join(OUT_DIR, base)
        if os.path.exists(dst) and os.path.getsize(dst) > 0:
            print("skip  %-28s (%d MB)" % (base, os.path.getsize(dst) // 1048576))
            continue
        try:
            with gzip.open(f, "rb") as fi, open(dst, "wb") as fo:
                shutil.copyfileobj(fi, fo, 1 << 20)
        except OSError as e:
            # 例如文件仍在下载中（被写入方占用）：跳过，稍后重跑即可
            print("busy  %-28s (%s)" % (base, e))
            if os.path.exists(dst) and os.path.getsize(dst) == 0:
                os.remove(dst)
            continue
        print("ok    %-28s (%d MB)" % (base, os.path.getsize(dst) // 1048576))
    return 0


if __name__ == "__main__":
    sys.exit(main())
