# ------------------------------------------------------------------------
# 名称：build.ps1
# 说明：构建 Ember 的 Windows x64 版：安装依赖、运行检查，再编译 C++ 原生模块并生成安装程序（或只打包）。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-17
# 备注：文件需保存为带 BOM 的 UTF-8，否则 Windows PowerShell 5.1 会按 ANSI 读取而使中文乱码。
# ------------------------------------------------------------------------

<#
.SYNOPSIS
构建 Ember 的 Windows x64 版。
.DESCRIPTION
依次安装依赖、类型检查、oxlint 检查、依赖边界检查、TypeScript 测试，再生成 Schema 代码、编译 C++ 原生模块并生成安装程序。
C++ 测试与调试 SDK 的检查不在其中，需要时单独运行 npm run core:test、npm run debug-sdk:check，或用 npm run check 全部运行。
.PARAMETER SkipCheck
跳过全部检查，只编译并生成。
.PARAMETER PackageOnly
只打包成可直接运行的目录（out\Ember-win32-x64），不生成安装程序。
.EXAMPLE
.\build.ps1
.EXAMPLE
.\build.ps1 -SkipCheck
.EXAMPLE
.\build.ps1 -PackageOnly
#>
param(
    [switch]$SkipCheck,
    [switch]$PackageOnly
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
try {
    Invoke-Step '安装依赖' { npm install }

    if (-not $SkipCheck) {
        Invoke-Step '类型检查' { npm run typecheck }
        Invoke-Step 'oxlint 检查' { npm run lint }
        Invoke-Step '依赖边界检查' { npm run boundaries }
        Invoke-Step '测试' { npm test }
    }

    if ($PackageOnly) {
        Invoke-Step '编译并打包' { npm run package:win }
        $output = Join-Path $PSScriptRoot 'out\Ember-win32-x64'
        if (-not (Test-Path -LiteralPath $output)) {
            throw "打包完成但没有找到输出目录：$output"
        }
        Write-Host "已生成：$output"
    }
    else {
        # 旧的安装程序不保留，避免误装
        $makeDirectory = Join-Path $PSScriptRoot 'out\make'
        if (Test-Path -LiteralPath $makeDirectory) {
            Remove-Item -LiteralPath $makeDirectory -Recurse -Force
        }

        Invoke-Step '编译并生成安装程序' { npm run make:win }

        $installer = Get-ChildItem -Path $makeDirectory -Filter 'ember-setup.exe' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($null -eq $installer) {
            throw '生成完成但没有找到 ember-setup.exe。'
        }
        Write-Host "已生成：$($installer.FullName)"
    }
}
finally {
    Pop-Location
}
