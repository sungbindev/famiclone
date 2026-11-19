# ------------------------------------------------------------------------
# 名称：debug-electron.ps1
# 说明：启动 Ember 的 Electron 外壳开发版：生成 Schema 代码、编译 C++ 原生模块，再用 Electron Forge 启动应用。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-13
# 备注：运行后按 q 或 Ctrl+C 停止；原来叫 debug.ps1。文件需保存为带 BOM 的 UTF-8，
#       否则 Windows PowerShell 5.1 会按 ANSI 读取而使中文乱码。
# ------------------------------------------------------------------------

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Set-Location -LiteralPath $PSScriptRoot
npm start
