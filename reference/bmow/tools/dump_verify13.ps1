# 读字符串 0x1EA/0x1F7/0x240 + 找所有 ldi r22,0x40; ldi r23,0x02（emumode 字符串ID 0x240 的引用）+ 0x26xx 函数头与调用者
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify13.txt'
$sb=New-Object System.Text.StringBuilder
foreach($a in @(0x1EA,0x1F7,0x240,0x2CE)){
  $s=''
  for($k=0;$k -lt 32;$k++){ $c=$bytes[$a+$k]; if($c -eq 0){break}; if($c -ge 32 -and $c -lt 127){$s+=[char]$c}else{$s+='.'} }
  [void]$sb.AppendLine(('string 0x{0:X3}: "{1}"' -f $a,$s))
}
[void]$sb.AppendLine('===== ldi r22,0x40 ; ldi r23,0x02 sites =====')
for($j=0x1FD8;$j -lt 0x12BE4-4;$j+=2){
  if($bytes[$j] -eq 0x60 -and $bytes[$j+1] -eq 0xE4 -and $bytes[$j+2] -eq 0x72 -and $bytes[$j+3] -eq 0xE0){
    [void]$sb.AppendLine(('  {0:X6}' -f $j))
    $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $j) ('--stop-address=0x{0:X}' -f ($j+16)) $b | Select-Object -Skip 7
    foreach($l in $lines){ [void]$sb.AppendLine('    ' + $l) }
  }
}
[void]$sb.AppendLine('===== func head 0x2600-0x2680 =====')
$lines=& $od -D -b binary -m avr:51 --start-address=0x2600 --stop-address=0x2680 $b | Select-Object -Skip 7
foreach($l in $lines){ [void]$sb.AppendLine($l) }
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'