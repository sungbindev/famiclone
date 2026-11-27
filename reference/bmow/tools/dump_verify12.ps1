# 找谁把 "emumode.txt" 填进 0x33DF：查 ldi r24,0xDF; ldi r25,0x33 的所有点及其后 24 字节
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify12.txt'
$sb=New-Object System.Text.StringBuilder
for($j=0x1FD8;$j -lt 0x12BE4-4;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  if(($w -band 0xF000) -ne 0xE000){ continue }
  $k=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
  $rd=16+(($w -shr 4) -band 0xF)
  if($k -ne 0xDF -or $rd -ne 24){ continue }
  $w2=$bytes[$j+2]+256*$bytes[$j+3]
  if(($w2 -band 0xF000) -ne 0xE000){ continue }
  $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
  $rd2=16+(($w2 -shr 4) -band 0xF)
  if($k2 -ne 0x33 -or $rd2 -ne 25){ continue }
  [void]$sb.AppendLine(('===== {0:X6} ldi r24,0xDF ; ldi r25,0x33 =====' -f $j))
  $st=[Math]::Max(0x1FD8,$j-20)
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $st) ('--stop-address=0x{0:X}' -f ($j+28)) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'