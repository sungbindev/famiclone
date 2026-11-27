# 查 0x1580 附近 6502 代码块的引用，判断 0x1596 零填充是否安全
$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify17.txt'
$sb=New-Object System.Text.StringBuilder
# ldi 对值在 0x1560-0x15C0
[void]$sb.AppendLine('===== ldi pairs 0x1560-0x15C0 =====')
for($j=0x1FD8;$j -lt 0x12BE4-4;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  if(($w -band 0xF000) -ne 0xE000){ continue }
  $k=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
  $rd=16+(($w -shr 4) -band 0xF)
  for($d=2;$d -le 6;$d+=2){
    $w2=$b[$j+$d]+256*$b[$j+$d+1]
    if(($w2 -band 0xF000) -ne 0xE000){ continue }
    $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
    $rd2=16+(($w2 -shr 4) -band 0xF)
    if($rd2 -ne $rd+1){ continue }
    $val=$k+256*$k2
    if($val -ge 0x1560 -and $val -le 0x15C0){ [void]$sb.AppendLine(('  {0:X6}: r{1}:{2}=0x{3:X4}' -f $j,$rd2,$rd,$val)) }
  }
}
# 16 位 LE 表项搜索 0x1580/0x1596 (80 15 / 96 15) 在数据区
[void]$sb.AppendLine('===== LE word 0x1580/0x1596 in 0x8C-0x1FD7 & 0x12BE4-0x12EFF =====')
foreach($t in @(0x1580,0x1596)){
  $lo=$t -band 0xFF; $hi=($t -shr 8) -band 0xFF
  foreach($rg in @(@(0x8C,0x1FD7),@(0x12BE4,0x12EFF))){
    for($i=$rg[0];$i -lt $rg[1];$i++){
      if($b[$i] -eq $lo -and $b[$i+1] -eq $hi){ [void]$sb.AppendLine(('  0x{0:X5}: {1:X2} {2:X2} (=0x{3:X4})' -f $i,$b[$i],$b[$i+1],$t)) }
    }
  }
}
[void]$sb.AppendLine('===== data 0x15C0-0x1620 =====')
for($i=0x15C0;$i -lt 0x1620;$i+=16){
  $hex=($b[$i..($i+15)] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '
  $line=($b[$i..($i+15)] | ForEach-Object { if($_ -ge 32 -and $_ -lt 127){[char]$_}else{'.'} }) -join ''
  [void]$sb.AppendLine(('{0:X4}: {1}  {2}' -f $i,$hex,$line))
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'