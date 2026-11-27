$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
# 找所有 sts 0x38B6（镜像类型号）：sts = 1001 001d dddd 0000 + 16位地址
$hits=@()
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  if(($w -band 0xFE0F) -ne 0x9200){ continue }
  $addr=$b[$j+2]+256*$b[$j+3]
  if($addr -eq 0x38B6){ $hits+=('{0:X6} r{1}' -f $j,(($w -shr 4) -band 0x1F)) }
}
Write-Output ('sts 0x38B6: ' + ($hits -join ' | '))
# 找 cpi 0x13 (19) 与 cpi 0x17/0x18 附近
$h2=@()
for($j=0x1FD8;$j -lt 0x12BE4;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  if(($w -band 0xF000) -ne 0x3000){ continue }
  $k=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
  if($k -eq 0x13){ $h2+=('{0:X6}' -f $j) }
}
Write-Output ('cpi 19: ' + ($h2 -join ' '))