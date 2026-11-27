# emumode.txt 在 0x12D24（RAMPZ=1 区）。可能的引用编码：
# a) ldi 对 0x24,0xAD（高位bit15表示RAMPZ=1，0x12D24-0x8000=0xAD24）
# b) ldi 对 0x92,0x96（字地址 0x9692，经蹦床）
# c) 表里的 16 位值 24 AD / 24 2D
$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify9.txt'
$sb=New-Object System.Text.StringBuilder
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$targets=@(@(0x24,0xAD),@(0x24,0x2D),@(0x92,0x96),@(0x10,0xAD),@(0x10,0x2D))
foreach($t in $targets){
  [void]$sb.AppendLine(('===== ldi pair lo=0x{0:X2} hi=0x{1:X2} =====' -f $t[0],$t[1]))
  for($j=0x1FD8;$j -lt 0x12BE4-4;$j+=2){
    $w=$b[$j]+256*$b[$j+1]
    if(($w -band 0xF000) -ne 0xE000){ continue }
    $k=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
    $rd=16+(($w -shr 4) -band 0xF)
    if($k -ne $t[0]){ continue }
    for($d=2;$d -le 6;$d+=2){
      $w2=$b[$j+$d]+256*$b[$j+$d+1]
      if(($w2 -band 0xF000) -ne 0xE000){ continue }
      $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
      $rd2=16+(($w2 -shr 4) -band 0xF)
      if($k2 -eq $t[1] -and $rd2 -eq $rd+1){
        [void]$sb.AppendLine(('  {0:X6}: r{1}:{2}=0x{3:X2}{4:X2}' -f $j,$rd2,$rd,$k2,$k))
        $st=[Math]::Max(0x1FD8,$j-12)
        $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $st) ('--stop-address=0x{0:X}' -f ($j+$d+16)) $b | Select-Object -Skip 7
        foreach($l in $lines){ [void]$sb.AppendLine('    ' + $l) }
      }
    }
  }
}
# 分支到 0x3758 的地方（brne/breq/rjmp 目标）
[void]$sb.AppendLine('===== branches to 0x3758 =====')
for($j=0x1FD8;$j -lt 0x12BE4;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  $op=$w -band 0xF000
  $t=-1
  if($op -eq 0xC000){ $off=$w -band 0xFFF; if($off -ge 0x800){$off-=0x1000}; $t=$j+2+2*$off }
  elseif(($w -band 0xFC00) -eq 0xF000 -or ($w -band 0xFC00) -eq 0xF400){ $off=($w -shr 3) -band 0x7F; if($off -ge 0x40){$off-=0x80}; $t=$j+2+2*$off }
  if($t -eq 0x3758){ [void]$sb.AppendLine(('  {0:X6} branch' -f $j)) }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'