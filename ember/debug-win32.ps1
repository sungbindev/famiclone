# ------------------------------------------------------------------------
# 名称：debug-win32.ps1
# 说明：启动 Ember 的 Win32 原生外壳开发版：生成 Schema 代码、打包预装插件、构建 Release 版的 ember-win32.exe 并运行。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-19
# 备注：窗口关闭后返回；多出的参数原样交给应用，如 .\debug-win32.ps1 --debug-port=5688 --user-data-dir=D:\tmp\ember；
#       文件需保存为带 BOM 的 UTF-8，否则 Windows PowerShell 5.1 会按 ANSI 读取而使中文乱码。
# ------------------------------------------------------------------------

[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
Set-Location -LiteralPath $PSScriptRoot
# 用 npm.cmd：npm.ps1 会吞掉 "--"，应用参数就传不进去
npm.cmd run win32:start -- @args
