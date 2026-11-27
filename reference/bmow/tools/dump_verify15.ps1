# 菜单扩项核实：字符绘制器字体基址、图形函数剩余部分、候选零填充区的引用扫描
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify15.txt'
$sb=New-Object System.Text.StringBuilder
$ranges=@(
  @(0xE4A4,0xE550,'char drawer font base'),
  @(0x2B0E,0x2B80,'graphics func rest'),
  @(0xE9BA,0xEA3C,'string drawer glyph fetch')
)
foreach($r in $ranges){
  [void]$sb.AppendLine(('===== {0:X4}-{1:X4} {2} =====' -f $r[0],$r[1],$r[2]))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $r[0]) ('--stop-address=0x{0:X}' -f $r[1]) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
# ldi 对扫描：目标 16 位值落在候选区附近
$zones=@(@(0x7D0,0x815),@(0x816,0x838),@(0xBC0,0xBF2),@(0x1580,0x15C2))
foreach($z in $zones){
  [void]$sb.AppendLine(('===== ldi pairs value in 0x{0:X}-0x{1:X} =====' -f $z[0],$z[1]))
  for($j=0x1FD8;$j -lt 0x12BE4-4;$j+=2){
    $w=$bytes[$j]+256*$bytes[$j+1]
    if(($w -band 0xF000) -ne 0xE000){ continue }
    $k=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
    $rd=16+(($w -shr 4) -band 0xF)
    for($d=2;$d -le 6;$d+=2){
      $w2=$bytes[$j+$d]+256*$bytes[$j+$d+1]
      if(($w2 -band 0xF000) -ne 0xE000){ continue }
      $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
      $rd2=16+(($w2 -shr 4) -band 0xF)
      if($rd2 -ne $rd+1){ continue }
      $val=$k+256*$k2
      if($val -ge $z[0] -and $val -le $z[1]){
        [void]$sb.AppendLine(('  {0:X6}: r{1}:{2} = 0x{3:X4}' -f $j,$rd2,$rd,$val))
      }
    }
  }
}
# 候选区前后数据
foreach($a in @(0x7C0,0x812,0xBB0,0x1580)){
  [void]$sb.AppendLine(('===== data 0x{0:X} =====' -f $a))
  for($i=$a;$i -lt $a+64;$i+=16){
    $hex=($bytes[$i..($i+15)] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '
    $line=($bytes[$i..($i+15)] | ForEach-Object { if($_ -ge 32 -and $_ -lt 127){[char]$_}else{'.'} }) -join ''
    [void]$sb.AppendLine(('{0:X4}: {1}  {2}' -f $i,$hex,$line))
  }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'