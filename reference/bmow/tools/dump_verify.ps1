# 实施前核实：main 全段、菜单循环、模式写入点 0x3352/0x3374/0x3388/0xB90E 上下文
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify-ranges.txt'
$ranges=@(
  @(0x30FA,0x317A,'menu table loop'),
  @(0x32C0,0x33A0,'mode name lookup + sts sites'),
  @(0x3B54,0x3EA8,'main full'),
  @(0xB880,0xB960,'sts mode @B90E context')
)
$sb=New-Object System.Text.StringBuilder
foreach($r in $ranges){
  [void]$sb.AppendLine(('===== {0:X4}-{1:X4} {2} =====' -f $r[0],$r[1],$r[2]))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $r[0]) ('--stop-address=0x{0:X}' -f $r[1]) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output ('written {0} lines' -f ($sb.ToString() -split "`n").Count)