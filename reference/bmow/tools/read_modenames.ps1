$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
# 模式名表 flash 0x5ED，步长 23，6 项（0x32BA cpi 6）。首字符是模式号 ASCII。
Write-Output '=== mode name table @0x5ED (stride 23, 6 entries) ==='
for($i=0;$i -lt 6;$i++){
  $o=0x5ED+23*$i
  $s=''
  for($k=0;$k -lt 23;$k++){ $c=$b[$o+$k]; if($c -eq 0){break}; $s+=[char]$c }
  Write-Output ('  [{0}] @0x{1:X4}: "{2}"' -f $i,$o,$s)
}
# 也看 0x5ED 前后，确认表起点
Write-Output '=== raw 0x5C0-0x680 ==='
for($i=0x5C0;$i -lt 0x680;$i+=16){
  $line=($b[$i..($i+15)] | ForEach-Object { if($_ -ge 32 -and $_ -lt 127){[char]$_}else{'.'} }) -join ''
  $hex=($b[$i..($i+15)] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '
  Write-Output ('{0:X4}: {1}  {2}' -f $i,$hex,$line)
}