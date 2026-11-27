# 实施核实第二批：不确定点上下文 + 0x2830 分发目标 + 启动 BSS 边界 + RAM 0x3D0-0x3E0 引用扫描
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify2.txt'
$sb=New-Object System.Text.StringBuilder
$ranges=@(
  @(0x2820,0x28C0,'mode dispatch 0x2830'),
  @(0x2C14,0x2C70,'dispatch targets 2C14/2C20/2C2C/2C38/2C42/2C5A'),
  @(0x4400,0x4470,'site 4410 context'),
  @(0x44A0,0x4530,'site 44A6 context'),
  @(0x7A50,0x7A90,'site 7A5A context'),
  @(0xE0E0,0xE140,'site E0F0 context'),
  @(0x1FDE,0x2036,'startup BSS/data bounds')
)
foreach($r in $ranges){
  [void]$sb.AppendLine(('===== {0:X4}-{1:X4} {2} =====' -f $r[0],$r[1],$r[2]))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $r[0]) ('--stop-address=0x{0:X}' -f $r[1]) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
# RAM 0x3D0-0x3E0 直接 lds/sts 引用
[void]$sb.AppendLine('===== direct lds/sts refs to RAM 0x3D0-0x3E0 =====')
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  $op=$w -band 0xFE0F
  if($op -ne 0x9000 -and $op -ne 0x9200){ continue }
  $addr=$bytes[$j+2]+256*$bytes[$j+3]
  if($addr -ge 0x3D0 -and $addr -le 0x3E0){
    $kind=if($op -eq 0x9000){'lds'}else{'sts'}
    [void]$sb.AppendLine(('  {0:X6} {1} r{2}, 0x{3:X4}' -f $j,$kind,(($w -shr 4) -band 0x1F),$addr))
  }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'