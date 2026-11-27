# 0x307E 的调用者 + 分支到 0x317A/0x3190 的点 + 设置菜单字符串表 0xC8 内容 + 0x27DA 头部
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify14.txt'
$sb=New-Object System.Text.StringBuilder
function FindRefs($target){
  for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
    $w=$bytes[$j]+256*$bytes[$j+1]
    if(($w -band 0xF000) -eq 0xD000){
      $off=$w -band 0xFFF; if($off -ge 0x800){$off-=0x1000}
      if(($j+2+2*$off) -eq $target){ [void]$sb.AppendLine(('  {0:X6} rcall 0x{1:X}' -f $j,$target)) }
    }
    if($bytes[$j] -eq 0x0E -and $bytes[$j+1] -eq 0x94){
      if((($bytes[$j+2]+256*$bytes[$j+3])*2) -eq $target){ [void]$sb.AppendLine(('  {0:X6} call 0x{1:X}' -f $j,$target)) }
    }
    if(($w -band 0xF000) -eq 0xC000){
      $off=$w -band 0xFFF; if($off -ge 0x800){$off-=0x1000}
      if(($j+2+2*$off) -eq $target){ [void]$sb.AppendLine(('  {0:X6} rjmp 0x{1:X}' -f $j,$target)) }
    }
    $op=$w -band 0xFC00
    if($op -eq 0xF000 -or $op -eq 0xF400){
      $off=($w -shr 3) -band 0x7F; if($off -ge 0x40){$off-=0x80}
      if(($j+2+2*$off) -eq $target){ [void]$sb.AppendLine(('  {0:X6} br* 0x{1:X}' -f $j,$target)) }
    }
  }
}
[void]$sb.AppendLine('===== refs to 0x307E ====='); FindRefs 0x307E
[void]$sb.AppendLine('===== refs to 0x317A ====='); FindRefs 0x317A
[void]$sb.AppendLine('===== refs to 0x3190 ====='); FindRefs 0x3190
[void]$sb.AppendLine('===== refs to 0x377E ====='); FindRefs 0x377E
[void]$sb.AppendLine('===== data 0xC8-0x160 (settings menu names) =====')
for($i=0xC8;$i -lt 0x160;$i+=16){
  $line=($bytes[$i..($i+15)] | ForEach-Object { if($_ -ge 32 -and $_ -lt 127){[char]$_}else{'.'} }) -join ''
  [void]$sb.AppendLine(('{0:X4}: {1}' -f $i,$line))
}
[void]$sb.AppendLine('===== func 0x27DA head =====')
$lines=& $od -D -b binary -m avr:51 --start-address=0x27DA --stop-address=0x2830 $b | Select-Object -Skip 7
foreach($l in $lines){ [void]$sb.AppendLine($l) }
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'