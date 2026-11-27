# 找 0x363A（设置菜单）的调用者 + 0x27DA 主菜单里对 emumode 的处理 + 0x2CD4/0x2E10 函数头
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify11.txt'
$sb=New-Object System.Text.StringBuilder
function FindCallers($target){
  $res=@()
  for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
    $w=$bytes[$j]+256*$bytes[$j+1]
    if(($w -band 0xF000) -eq 0xD000){
      $off=$w -band 0xFFF; if($off -ge 0x800){$off-=0x1000}
      if(($j+2+2*$off) -eq $target){ $res+=('{0:X6} rcall' -f $j) }
    }
    if($bytes[$j] -eq 0x0E -and $bytes[$j+1] -eq 0x94){
      if((($bytes[$j+2]+256*$bytes[$j+3])*2) -eq $target){ $res+=('{0:X6} call' -f $j) }
    }
    if(($w -band 0xF000) -eq 0xC000){
      $off=$w -band 0xFFF; if($off -ge 0x800){$off-=0x1000}
      if(($j+2+2*$off) -eq $target){ $res+=('{0:X6} rjmp' -f $j) }
    }
  }
  return $res
}
foreach($t in @(0x363A,0x317A,0x2CD4,0x2E10)){
  [void]$sb.AppendLine(('===== callers of 0x{0:X} =====' -f $t))
  foreach($c in (FindCallers $t)){ [void]$sb.AppendLine('  ' + $c) }
}
# 各调用点上下文
$ctx=@(0x2CD4,0x2E10)
foreach($s in $ctx){
  [void]$sb.AppendLine(('===== func head {0:X4} =====' -f $s))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $s) ('--stop-address=0x{0:X}' -f ($s+64)) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'