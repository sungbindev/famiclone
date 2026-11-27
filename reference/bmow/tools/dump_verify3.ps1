# 第三批：size 判定精确字节 + 0xCA62 接受路径 + 名表尾部 + 0x3B00-0x3B30 引用 + 0x677 引用
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify3.txt'
$sb=New-Object System.Text.StringBuilder
$ranges=@(
  @(0xC900,0xC970,'size detect precise'),
  @(0xCA62,0xCAA0,'NIB accept path'),
  @(0x30E4,0x30FA,'menu end ptr load')
)
foreach($r in $ranges){
  [void]$sb.AppendLine(('===== {0:X4}-{1:X4} {2} =====' -f $r[0],$r[1],$r[2]))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $r[0]) ('--stop-address=0x{0:X}' -f $r[1]) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
[void]$sb.AppendLine('===== data 0x677-0x6C0 (mode table tail) =====')
for($i=0x677;$i -lt 0x6C0;$i+=16){
  $hex=($bytes[$i..($i+15)] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '
  $line=($bytes[$i..($i+15)] | ForEach-Object { if($_ -ge 32 -and $_ -lt 127){[char]$_}else{'.'} }) -join ''
  [void]$sb.AppendLine(('{0:X4}: {1}  {2}' -f $i,$hex,$line))
}
[void]$sb.AppendLine('===== direct lds/sts refs to RAM 0x3B00-0x3B30 =====')
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  $op=$w -band 0xFE0F
  if($op -ne 0x9000 -and $op -ne 0x9200){ continue }
  $addr=$bytes[$j+2]+256*$bytes[$j+3]
  if($addr -ge 0x3B00 -and $addr -le 0x3B30){
    $kind=if($op -eq 0x9000){'lds'}else{'sts'}
    [void]$sb.AppendLine(('  {0:X6} {1} r{2}, 0x{3:X4}' -f $j,$kind,(($w -shr 4) -band 0x1F),$addr))
  }
}
[void]$sb.AppendLine('===== ldi pairs loading 0x0677 (menu table end) =====')
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  if(($w -band 0xF000) -ne 0xE000){ continue }
  $k=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
  $rd=16+(($w -shr 4) -band 0xF)
  if($k -eq 0x77){
    $w2=$bytes[$j+2]+256*$bytes[$j+3]
    if(($w2 -band 0xF000) -eq 0xE000){
      $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
      $rd2=16+(($w2 -shr 4) -band 0xF)
      if($k2 -eq 0x06 -and $rd2 -eq ($rd+1)){ [void]$sb.AppendLine(('  {0:X6} ldi r{1},0x77 ; ldi r{2},0x06' -f $j,$rd,$rd2)) }
    }
  }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'