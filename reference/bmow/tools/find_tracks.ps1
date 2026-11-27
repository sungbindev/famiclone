# 搜 ldi 对：35 (0x23), 140 (0x8C), 160 (0xA0) 低字节, 0 高字节；以及 cpi 0x8C 前后带 subi 的
$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
function FindLdiPair([int]$ka,[int]$kc){
  $hits=@()
  for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
    $w1=$b[$j]+256*$b[$j+1]
    $w2=$b[$j+2]+256*$b[$j+3]
    if(($w1 -band 0xF000) -ne 0xE000 -or ($w2 -band 0xF000) -ne 0xE000){ continue }
    $r1=16+(($w1 -shr 4) -band 0xF)
    $r2=16+(($w2 -shr 4) -band 0xF)
    if($r2 -ne $r1+1){ continue }
    $k1=($w1 -band 0xF) -bor ((($w1 -shr 8) -band 0xF) -shl 4)
    $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
    if($k1 -eq $ka -and $k2 -eq $kc){ $hits+=('{0:X6} r{1}' -f $j,$r1) }
  }
  return $hits
}
Write-Output ('ldi 35: ' + ((FindLdiPair 0x23 0) -join ' | '))
Write-Output ('ldi 140: ' + ((FindLdiPair 0x8C 0) -join ' | '))
Write-Output ('ldi 160: ' + ((FindLdiPair 0xA0 0) -join ' | '))
Write-Output ('ldi 40: ' + ((FindLdiPair 0x28 0) -join ' | '))
# 0x12C96 参数表（RAM 0x389C 的初值）
Write-Output ('0x12C96: ' + (($b[0x12C96..0x12CB5] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '))
# 0x12C80 类型名指针表
$ptrs=@()
for($j=0x12C80;$j -lt 0x12C96;$j+=2){ $ptrs+=('{0:X4}' -f ($b[$j]+256*$b[$j+1])) }
Write-Output ('0x12C80 ptrs: ' + ($ptrs -join ' '))