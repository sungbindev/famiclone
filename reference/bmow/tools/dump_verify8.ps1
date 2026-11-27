# 找 emumode.txt 字符串(字节地址 0x12D24)的加载点：ldi 对 0x24,0x2D 且附近 out 0x3b(RAMPZ)=1
# 0x12D24: RAMPZ=1, Z=0x2D24。也检查经蹦床 0x12308 的调用（字地址 0x9692 → ldi 0x92,0x96? 蹦床收 Z=字节地址>>1?）
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify8.txt'
$sb=New-Object System.Text.StringBuilder
# ldi rN,0x24 ; ldi rN+1,0x2D 对（任意顺序，窗口 8 字节内）
[void]$sb.AppendLine('===== ldi 0x24/0x2D pairs =====')
for($j=0x1FD8;$j -lt 0x12BE4-4;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  if(($w -band 0xF000) -ne 0xE000){ continue }
  $k=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
  $rd=16+(($w -shr 4) -band 0xF)
  if($k -ne 0x24 -and $k -ne 0x2D){ continue }
  for($d=2;$d -le 8;$d+=2){
    $w2=$bytes[$j+$d]+256*$bytes[$j+$d+1]
    if(($w2 -band 0xF000) -ne 0xE000){ continue }
    $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
    $rd2=16+(($w2 -shr 4) -band 0xF)
    if(($k -eq 0x24 -and $k2 -eq 0x2D -and $rd2 -eq $rd+1) -or ($k -eq 0x2D -and $k2 -eq 0x24 -and $rd -eq $rd2+1)){
      [void]$sb.AppendLine(('  {0:X6}: r{1}=0x{2:X2}, r{3}=0x{4:X2} (d={5})' -f $j,$rd,$k,$rd2,$k2,$d))
      $st=[Math]::Max(0x1FD8,$j-8)
      $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $st) ('--stop-address=0x{0:X}' -f ($j+$d+12)) $b | Select-Object -Skip 7
      foreach($l in $lines){ [void]$sb.AppendLine('    ' + $l) }
    }
  }
}
# 0x317A 唯一调用者 0x3758 的上下文
[void]$sb.AppendLine('===== context 0x3700-0x377E (caller of 0x317A) =====')
$lines=& $od -D -b binary -m avr:51 --start-address=0x3700 --stop-address=0x377E $b | Select-Object -Skip 7
foreach($l in $lines){ [void]$sb.AppendLine($l) }
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'