# 核实 emumode 文件名：0x38F6/0x38F7 指针的写入者 + flash 里 EMUMODE 字符串
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify7.txt'
$sb=New-Object System.Text.StringBuilder
# 1. flash 里搜 EMUMODE（ASCII，不分大小写）
[void]$sb.AppendLine('===== EMUMODE strings in image =====')
$pat=[Text.Encoding]::ASCII.GetBytes('emumode')
for($i=0;$i -lt $bytes.Length-7;$i++){
  $ok=$true
  for($k=0;$k -lt 7;$k++){
    $c=$bytes[$i+$k]
    if($c -ge 65 -and $c -le 90){ $c+=32 }
    if($c -ne $pat[$k]){ $ok=$false; break }
  }
  if($ok){
    $s=''
    for($k=0;$k -lt 24;$k++){ $c=$bytes[$i+$k]; if($c -eq 0){break}; if($c -ge 32 -and $c -lt 127){$s+=[char]$c}else{$s+='.'} }
    [void]$sb.AppendLine(('  0x{0:X5}: "{1}"' -f $i,$s))
  }
}
# 2. sts 0x38F6 / 0x38F7 写入点
[void]$sb.AppendLine('===== sts 0x38F6/0x38F7 sites =====')
$stsSites=@()
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  if(($w -band 0xFE0F) -ne 0x9200){ continue }
  $addr=$bytes[$j+2]+256*$bytes[$j+3]
  if($addr -eq 0x38F6 -or $addr -eq 0x38F7){
    $stsSites+=$j
    [void]$sb.AppendLine(('  {0:X6} sts r{1}, 0x{2:X4}' -f $j,(($w -shr 4) -band 0x1F),$addr))
  }
}
# 3. 每个写入点前的上下文（往前 16 字节反汇编）
foreach($s in $stsSites){
  [void]$sb.AppendLine(('===== context before {0:X6} =====' -f $s))
  $st=[Math]::Max(0x1FD8,$s-16)
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $st) ('--stop-address=0x{0:X}' -f ($s+4)) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
# 4. 0x317A 的调用者（rcall 与 call）
[void]$sb.AppendLine('===== callers of 0x317A =====')
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  if(($w -band 0xF000) -eq 0xD000){
    $off=$w -band 0xFFF
    if($off -ge 0x800){ $off-=0x1000 }
    $t=$j+2+2*$off
    if($t -eq 0x317A){ [void]$sb.AppendLine(('  {0:X6} rcall 0x317A' -f $j)) }
  }
  if($bytes[$j] -eq 0x0E -and $bytes[$j+1] -eq 0x94){
    $t=($bytes[$j+2]+256*$bytes[$j+3])*2
    if($t -eq 0x317A){ [void]$sb.AppendLine(('  {0:X6} call 0x317A' -f $j)) }
  }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'