// ------------------------------------------------------------------------
// 名称：diff-png.mjs
// 说明：逐像素比较两张 PNG（用 Windows 的 System.Drawing 解码），打印不同像素的个数与外接矩形。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/diff-png.mjs <甲.png> <乙.png>；只在 Windows 上可用。
// ------------------------------------------------------------------------
import { execFileSync } from 'node:child_process';
import { resolve } from 'node:path';

const [first, second] = process.argv.slice(2).map((path) => resolve(path));
const script = `
Add-Type -AssemblyName System.Drawing
$a = New-Object System.Drawing.Bitmap '${first}'
$b = New-Object System.Drawing.Bitmap '${second}'
if ($a.Width -ne $b.Width -or $a.Height -ne $b.Height) { "尺寸不同：$($a.Width)x$($a.Height) 与 $($b.Width)x$($b.Height)"; exit }
$count = 0; $minX = 9999; $minY = 9999; $maxX = -1; $maxY = -1; $samples = @()
for ($y = 0; $y -lt $a.Height; $y++) { for ($x = 0; $x -lt $a.Width; $x++) {
  if ($a.GetPixel($x, $y).ToArgb() -ne $b.GetPixel($x, $y).ToArgb()) { $count++; if ($samples.Count -lt 4) { $samples += "($x,$y) " + $a.GetPixel($x, $y).Name + ' / ' + $b.GetPixel($x, $y).Name }; if ($x -lt $minX) { $minX = $x }; if ($y -lt $minY) { $minY = $y }; if ($x -gt $maxX) { $maxX = $x }; if ($y -gt $maxY) { $maxY = $y } }
} }
if ($count -eq 0) { '像素完全相同' } else { "不同像素 $count 个，范围 ($minX,$minY)-($maxX,$maxY)"; $samples }
`;
console.log(execFileSync('powershell', ['-NoProfile', '-Command', script], { encoding: 'utf8' }).trim());
