#!/usr/bin/env python3
"""下载全部 24 份 DIMACS 路网并解压到 data/assets/d 和 data/assets/t。"""

import argparse
import gzip
import shutil
import time
import urllib.request
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


NETWORKS = ['NY', 'BAY', 'COL', 'FLA', 'NW', 'NE', 'CAL', 'LKS', 'E', 'W', 'CTR', 'USA']
BASE = 'http://www.diag.uniroma1.it/challenge9/data'
DEFAULT_OUT = Path(__file__).resolve().parent


def download(kind, net, output, retries):
    name = 'USA-road-%s.%s.gr.gz' % (kind, net)
    target = output / name
    partial = output / (name + '.part')
    if target.is_file() and target.stat().st_size > 0:
        return 'SKIP ' + name
    url = '%s/USA-road-%s/%s' % (BASE, kind, name)
    for attempt in range(1, retries + 1):
        try:
            request = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
            with urllib.request.urlopen(request, timeout=120) as response:
                expected = response.headers.get('Content-Length')
                with partial.open('wb') as file:
                    shutil.copyfileobj(response, file, length=1024 * 1024)
            size = partial.stat().st_size
            if not size or (expected is not None and size != int(expected)):
                raise ValueError('Incomplete download: %d bytes' % size)
            with partial.open('rb') as file:
                if file.read(2) != b'\x1f\x8b':
                    raise ValueError('Response is not a gzip archive')
            partial.replace(target)
            return 'OK   %s (%.1f MiB)' % (name, size / 1048576)
        except Exception as error:
            if attempt == retries:
                raise RuntimeError('%s: %s' % (name, error)) from error
            time.sleep(min(2 ** attempt, 10))


def prepare(kind, net, output, assets, retries):
    status = download(kind, net, output, retries)
    name = 'USA-road-%s.%s' % (kind, net)
    archive = output / (name + '.gr.gz')
    folder = assets / kind
    folder.mkdir(parents=True, exist_ok=True)
    target = folder / (name + '.gr')
    partial = folder / (name + '.gr.part')
    try:
        # 完整读取 gzip，校验 CRC；成功后才替换已有文件。
        with gzip.open(archive, 'rb') as source, partial.open('wb') as file:
            shutil.copyfileobj(source, file, length=1024 * 1024)
        with partial.open('rb') as file:
            for line in file:
                if line.startswith(b'p sp '):
                    fields = line.split()
                    if len(fields) != 4 or int(fields[2]) < 2 or int(fields[3]) < 0:
                        raise ValueError('Invalid graph header: ' + name)
                    break
            else:
                raise ValueError('Missing graph header: ' + name)
        partial.replace(target)
    finally:
        if partial.exists():
            partial.unlink()
    return status + ' -> ' + str(target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--workers', type=int, default=4, help='并行下载数，默认 4')
    parser.add_argument('--retries', type=int, default=3, help='每个文件最多尝试次数，默认 3')
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT, help='输出目录，默认项目 tmp/')
    parser.add_argument('--assets', type=Path, default=DEFAULT_OUT.parent / 'data' / 'assets', help='解压输出目录')
    args = parser.parse_args()
    if args.workers < 1 or args.retries < 1:
        parser.error('--workers and --retries must be positive')
    args.out.mkdir(parents=True, exist_ok=True)
    print('Output:', args.out.resolve(), flush=True)
    networks = NETWORKS
    total = 2 * len(networks)
    print('%d archives; existing nonempty files are skipped.' % total, flush=True)
    failures = 0
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        jobs = [pool.submit(prepare, kind, net, args.out, args.assets, args.retries)
                for kind in ['d', 't'] for net in networks]
        for completed, job in enumerate(as_completed(jobs), 1):
            try:
                print('[%d/%d] %s' % (completed, total, job.result()), flush=True)
            except Exception as error:
                failures += 1
                print('[%d/%d] FAIL %s' % (completed, total, error), flush=True)
    print('Finished: %d failed. Rerun to retry failures.' % failures, flush=True)
    return 1 if failures else 0


if __name__ == '__main__':
    raise SystemExit(main())
