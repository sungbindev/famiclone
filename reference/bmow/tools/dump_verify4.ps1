# 第四批：0x2A4C 第二处表尾引用上下文 + 宽 RAM 扫描 0x3B06-0x3C00 + 0x2A40-0x2B00
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify4.txt'
$sb=New-Object System.Text.StringBuilder
$ranges=@(
  @(0x2A40,0x2B10,'second table-end ref 0x2A4C'),
  @(0x3390,0x33B0,'name lookup result mul23')
)
foreach($r in $ranges){
  [void]$sb.AppendLine(('===== {0:X4}-{1:X4} {2} =====' -f $r[0],$r[1],$r[2]))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $r[0]) ('--stop-address=0x{0:X}' -f $r[1]) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
[void]$sb.AppendLine('===== direct lds/sts refs to RAM 0x3B06-0x3C00 =====')
$cnt=0
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  $op=$w -band 0xFE0F
  if($op -ne 0x9000 -and $op -ne 0x9200){ continue }
  $addr=$bytes[$j+2]+256*$bytes[$j+3]
  if($addr -ge 0x3B06 -and $addr -le 0x3C00){
    $kind=if($op -eq 0x9000){'lds'}else{'sts'}
    [void]$sb.AppendLine(('  {0:X6} {1} r{2}, 0x{3:X4}' -f $j,$kind,(($w -shr 4) -band 0x1F),$addr))
    $cnt++
  }
}
[void]$sb.AppendLine(('  total: {0}' -f $cnt))
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'