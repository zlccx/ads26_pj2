# =============================================================================
#  download.ps1 —— 从 DIMACS Challenge 9 下载 USA 公路网数据
#
#  用法：
#     powershell -ExecutionPolicy Bypass -File tools\download.ps1
#     powershell -ExecutionPolicy Bypass -File tools\download.ps1 -Nets NY,BAY,CAL
#
#  数据来源（第 9 届 DIMACS 实现挑战赛，公共领域）：
#     http://www.diag.uniroma1.it/challenge9/download.shtml
#  下载的 .gr.gz 会存放在 data/gz/ 下，用 tools/decompress.py 解压。
# =============================================================================
param(
    [string[]]$Nets = @('NY', 'BAY', 'COL', 'FLA', 'NW', 'NE', 'CAL'),
    [string]$Kind = 'd'          # 'd' = 物理距离图，'t' = 通行时间图
)

$ErrorActionPreference = 'Continue'
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$gz = Join-Path $root 'data\gz'
New-Item -ItemType Directory -Force -Path $gz | Out-Null

$base = 'http://www.diag.uniroma1.it/challenge9/data/USA-road-' + $Kind +
        '/USA-road-' + $Kind + '.'

foreach ($n in $Nets) {
    $url = $base + $n + '.gr.gz'
    $dst = Join-Path $gz ("USA-road-$Kind.$n.gr.gz")
    if (Test-Path $dst) { Write-Host "SKIP $n (already downloaded)"; continue }
    try {
        Write-Host "GET  $url"
        Invoke-WebRequest -Uri $url -OutFile $dst -TimeoutSec 1800
        $mb = [math]::Round((Get-Item $dst).Length / 1MB, 2)
        Write-Host "OK   $n  ($mb MB)"
    } catch {
        Write-Host "FAIL $n : $($_.Exception.Message)"
    }
}

# 完整的下载页（还包含 .co 坐标、travel-time 版本、以及 E / LKS / W / CTR / USA 等）
Write-Host ""
Write-Host "All downloads: http://www.diag.uniroma1.it/challenge9/download.shtml"
