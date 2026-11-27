# 核实 emumode.txt 机制：0x10496 函数、0x317A 调用者、emumode.txt 字符串(0x12D24)引用
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify6.txt'
$sb=New-Object System.Text.StringBuilder
$ranges=@(
  @(0x10496,0x104A6,'func 0x10496 (file read?)'),
  @(0xF8D2,0xF920,'func 0xF8D2'),
  @(0x317A,0x31A2,'mode handler start')
)
foreach($r in $ranges){
  [void]$sb.AppendLine(('===== {0:X4}-{1:X4} {2} =====' -f $r[0],$r[1],$r[2]))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $r[0]) ('--stop-address=0x{0:X}' -f $r[1]) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
# 找 call 0x317A 与 call 0x10496 的调用点（call 编码 0E 94 lo hi，目标字地址*2）
[void]$sb.AppendLine('===== callers of 0x317A and 0x10496 =====')
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  if($bytes[$j] -eq 0x0E -and $bytes[$j+1] -eq 0x94){
    $t=($bytes[$j+2]+256*$bytes[$j+3])*2
    if($t -eq 0x317A -or $t -eq 0x10496){ [void]$sb.AppendLine(('  {0:X6} call 0x{1:X}' -f $j,$t)) }
  }
}
# 找加载 0x2D24 (emumode.txt 低16位) 的 ldi 对，附近设 RAMPZ=1
[void]$sb.AppendLine('===== ldi pairs loading 0x2D24 (emumode.txt lo16) =====')
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  if(($w -band 0xF000) -ne 0xE000){ continue }
  $k=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
  $rd=16+(($w -shr 4) -band 0xF)
  if($k -eq 0x24){
    $w2=$bytes[$j+2]+256*$bytes[$j+3]
    if(($w2 -band 0xF000) -eq 0xE000){
      $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
      $rd2=16+(($w2 -shr 4) -band 0xF)
      if($k2 -eq 0x2D -and $rd2 -eq ($rd+1)){ [void]$sb.AppendLine(('  {0:X6} ldi r{1},0x24 ; ldi r{2},0x2D' -f $j,$rd,$rd2)) }
    }
  }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'