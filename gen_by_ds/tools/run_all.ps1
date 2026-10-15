# =============================================================================
#  run_all.ps1 —— 一键复现整个项目（下载 -> 实验 -> 出图 -> 编译报告）
#
#  用法：
#     powershell -ExecutionPolicy Bypass -File tools\run_all.ps1
#     powershell -ExecutionPolicy Bypass -File tools\run_all.ps1 -Nets NY,BAY,COL,FLA,NW,NE,CAL -Jobs 3
# =============================================================================
param(
    [string[]]$Nets = @('NY', 'BAY', 'COL', 'FLA', 'NW', 'NE', 'CAL'),
    [int]$Jobs = 3,
    [int]$FullSources = 16
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
Set-Location $root

function Step($msg) { Write-Host ""; Write-Host "=== $msg ===" -ForegroundColor Cyan }

Step "1/8 编译"
New-Item -ItemType Directory -Force -Path build | Out-Null
g++ -O2 -std=c++17 -static -o build/selftest.exe src/selftest.cpp
g++ -O2 -std=c++17 -static -o build/bench.exe    src/bench.cpp

Step "2/8 下载数据（已存在的会跳过）"
powershell -ExecutionPolicy Bypass -File tools\download.ps1 -Nets $Nets

Step "3/8 解压"
python tools\decompress.py

Step "4/8 生成查询"
python tools\gen_queries.py

Step "5/8 正确性测试"
New-Item -ItemType Directory -Force -Path results\logs | Out-Null
cmd /c "build\selftest.exe > results\logs\selftest.log 2>&1"

Step "6/8 跑性能实验（并行 $Jobs）"
python tools\run_bench.py -j $Jobs --full-sources $FullSources

Step "7/8 交叉验证 + 画图 + 生成报告表格"
python tools\verify_answers.py
python tools\plot.py
python tools\make_report_tables.py

Step "8/8 编译报告"
typst compile --root . report\report.typ report\report.pdf

Write-Host ""
Write-Host "完成：report\report.pdf" -ForegroundColor Green
