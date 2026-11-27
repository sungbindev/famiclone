# 追 emumode.txt 名字的装填：0x367C-0x3700（事件分支）、0x12428 字符串复制函数、out 0x3b(RAMPZ) 全部写点
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify10.txt'
$sb=New-Object System.Text.StringBuilder
$ranges=@(
  @(0x363A,0x3700,'event loop around branch 0x36FA'),
  @(0x12428,0x12490,'string copy helper'),
  @(0x33B2,0x3420,'func 0x33B2 (open?)')
)
foreach($r in $ranges){
  [void]$sb.AppendLine(('===== {0:X4}-{1:X4} {2} =====' -f $r[0],$r[1],$r[2]))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $r[0]) ('--stop-address=0x{0:X}' -f $r[1]) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
[void]$sb.AppendLine('===== out 0x3b (RAMPZ) sites =====')
for($j=0x1FD8;$j -lt 0x12BE4;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  if(($w -band 0xF800) -eq 0xB800){
    $io=(($w -shr 5) -band 0x30) -bor ($w -band 0xF)
    if($io -eq 0x3B){ [void]$sb.AppendLine(('  {0:X6} out RAMPZ, r{1}' -f $j,(($w -shr 4) -band 0x1F))) }
  }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'