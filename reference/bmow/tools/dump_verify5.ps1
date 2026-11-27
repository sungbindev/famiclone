# 第五批：字符串绘制器 0xE902 / strlen 0x123C6 是否支持 RAMPZ + 低区零填充扫描 + 图形函数调用者
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify5.txt'
$sb=New-Object System.Text.StringBuilder
$ranges=@(
  @(0xE902,0xE960,'string drawer 0xE902'),
  @(0x123C6,0x12428,'strlen 0x123C6'),
  @(0x2906,0x2960,'graphics func caller context (mode 3/6 path)')
)
foreach($r in $ranges){
  [void]$sb.AppendLine(('===== {0:X4}-{1:X4} {2} =====' -f $r[0],$r[1],$r[2]))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $r[0]) ('--stop-address=0x{0:X}' -f $r[1]) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
# 低区零填充扫描（>=23 字节连续 0），范围 0x8C..0x12EFF
[void]$sb.AppendLine('===== zero runs >= 23 bytes in 0x8C..0x12EFF =====')
$runStart=-1
for($i=0x8C;$i -le 0x12EFF;$i++){
  if($bytes[$i] -eq 0){
    if($runStart -lt 0){ $runStart=$i }
  } else {
    if($runStart -ge 0){
      $len=$i-$runStart
      if($len -ge 23){ [void]$sb.AppendLine(('  0x{0:X5} len {1}' -f $runStart,$len)) }
      $runStart=-1
    }
  }
}
if($runStart -ge 0){ $len=0x12F00-$runStart; if($len -ge 23){ [void]$sb.AppendLine(('  0x{0:X5} len {1}' -f $runStart,$len)) } }
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'