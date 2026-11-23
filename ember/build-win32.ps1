# ------------------------------------------------------------------------
# 名称：build-win32.ps1
# 说明：构建 Ember 的 Win32 原生外壳（ember-win32.exe）：安装依赖、编译 Release 版，再运行检查。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-19
# 备注：文件需保存为带 BOM 的 UTF-8，否则 Windows PowerShell 5.1 会按 ANSI 读取而使中文乱码。
#       安装包与便携包（-PackageOnly）在升级与安装包阶段加入，此前这个脚本只到编译与检查为止。
# ------------------------------------------------------------------------

<#
.SYNOPSIS
构建 Ember 的 Win32 原生外壳。
.DESCRIPTION
依次安装依赖、生成 Schema 代码并用 v143 工具集（目标 Windows 7 SP1）编译 Release 版的原生应用与控件库、依赖边界检查、控件库结构检查与导入表审计。
C++ 测试（npm run win32:test）与控件库的网页版对照（npm run win32:reference）不在其中，需要时单独运行。
.PARAMETER SkipCheck
跳过全部检查，只编译。
.EXAMPLE
.\build-win32.ps1
.EXAMPLE
.\build-win32.ps1 -SkipCheck
#>
param(
    [switch]$SkipCheck
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# 原生命令失败不会触发 $ErrorActionPreference，需逐条检查退出代码
function Invoke-Step {
    param(
        [string]$Name,
        [scriptblock]$Command
    )
    Write-Host "==> $Name"
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$Name 失败，退出代码：$LASTEXITCODE。"
    }
}

Push-Location $PSScriptRoot
# Ember 的构建产物都在仓库根目录的 build\ember 下
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\build\ember'))
try {
    Invoke-Step '安装依赖' { npm install }

    Invoke-Step '编译' { npm run win32:build }

    if (-not $SkipCheck) {
        Invoke-Step '依赖边界检查' { npm run boundaries }
        Invoke-Step 'Win32 检查' { npm run win32:check }
    }

    $executable = Join-Path $buildRoot 'win32\app\Release\ember-win32.exe'
    if (-not (Test-Path -LiteralPath $executable)) {
        throw "编译完成但没有找到：$executable"
    }
    Write-Host "已生成：$executable"
}
finally {
    Pop-Location
}
