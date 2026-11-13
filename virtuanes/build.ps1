# ------------------------------------------------------------------------
# 名称：build.ps1
# 说明：用 MSBuild 编译模拟器（Win32），可选编译后启动。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-10
# 备注：用法 .\build.ps1 [-Configuration Debug|Release] [-Rebuild] [-Run]；
#       需要 Visual Studio 2026（含“使用 C++ 的桌面开发”与 v145 工具集）；
#       输出在仓库根目录的 build\virtuanes\<配置>\bin\VirtuaNES.exe，中间文件在 build\virtuanes\<配置>\obj\；
#       只支持 Win32，工程里的 x64 配置缺少可用的 DirectInput 库；
#       文件需保存为带 BOM 的 UTF-8，否则 Windows PowerShell 5.1 会按 ANSI 读取而使中文乱码。
# ------------------------------------------------------------------------

param(
    # 编译配置，Debug 带调试信息，Release 为发布配置
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    # 清理后完整重新编译
    [switch]$Rebuild,

    # 编译完成后启动程序
    [switch]$Run
)

$ErrorActionPreference = 'Stop'

$sourceRoot = $PSScriptRoot
$projectPath = Join-Path $sourceRoot 'VirtuaNES\VirtuaNES.vcxproj'
$exeName = 'VirtuaNES.exe'
$outDir = Join-Path (Split-Path $sourceRoot -Parent) "build\virtuanes\$Configuration\bin"
$exePath = Join-Path $outDir $exeName

# 经 vswhere 查找最新的 MSBuild，避免依赖开发者命令提示符
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswherePath)) {
    throw '未找到 vswhere.exe，请先安装 Visual Studio。'
}
$msbuildPath = & $vswherePath -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuildPath) {
    throw '未找到 MSBuild，请在 Visual Studio 中安装“使用 C++ 的桌面开发”。'
}

# 正在运行的程序会占用输出文件。调试实例（调试服务监听 127.0.0.1 端口）是 fcdev 或脚本启动的，编译前结束；
# 其余实例是用户自己打开的，不结束，有它们占用输出文件时报错，由用户关闭。
$running = @(Get-Process -Name ([IO.Path]::GetFileNameWithoutExtension($exeName)) -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($outDir, [System.StringComparison]::OrdinalIgnoreCase) })
$userInstances = @()
foreach ($process in $running) {
    $debugPort = Get-NetTCPConnection -State Listen -OwningProcess $process.Id -ErrorAction SilentlyContinue |
        Where-Object { $_.LocalAddress -eq '127.0.0.1' }
    if ($debugPort) {
        Stop-Process -Id $process.Id -Force
    } else {
        $userInstances += $process
    }
}
if ($userInstances.Count -gt 0) {
    $ids = ($userInstances | ForEach-Object { $_.Id }) -join '、'
    throw "有非调试的模拟器正在运行并占用输出文件（进程 $ids），请先关闭它们再编译。"
}

$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }

Write-Host "编译 $Configuration|Win32（$target）..." -ForegroundColor Cyan
& $msbuildPath $projectPath -nologo -m -v:minimal -t:$target -p:Configuration=$Configuration -p:Platform=Win32
if ($LASTEXITCODE -ne 0) { throw "编译失败，退出码 $LASTEXITCODE" }

if (-not (Test-Path -LiteralPath $exePath)) {
    throw "编译完成但未找到 $exePath"
}
Write-Host "编译完成：$exePath" -ForegroundColor Green

if ($Run) {
    Write-Host "启动 $exePath" -ForegroundColor Green
    # 语言包用相对文件名加载，工作目录必须是 exe 所在目录
    Start-Process -FilePath $exePath -WorkingDirectory $outDir
}
