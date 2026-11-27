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
# 232960 = 0x38E00 → 低16位 0x8E00: lo=0x00 hi=0x8E
Write-Output ('ldi 0x8E00: ' + ((FindLdiPair 0x00 0x8E) -join ' | '))
# 266240 = 0x41000 → 低16位 0x1000: lo=0x00 hi=0x10
Write-Output ('ldi 0x1000: ' + ((FindLdiPair 0x00 0x10) -join ' | '))
# 高16位 3 和 4 的 ldi 对（r+1 高=0）
Write-Output ('ldi 3: ' + ((FindLdiPair 3 0) -join ' | '))
# cpi/sbci 对：0x00,0x8E（232960 低16位）与 0x00,0x10
function FindPairImm([int]$op2,[int]$k1,[int]$k2){
  $hits=@()
  for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
    $w1=$b[$j]+256*$b[$j+1]
    $w2=$b[$j+2]+256*$b[$j+3]
    if(($w1 -band 0xF000) -ne 0x3000){ continue }
    if(($w2 -band 0xF000) -ne $op2){ continue }
    $r1=16+(($w1 -shr 4) -band 0xF)
    $r2=16+(($w2 -shr 4) -band 0xF)
    if($r2 -ne $r1+1){ continue }
    $a=($w1 -band 0xF) -bor ((($w1 -shr 8) -band 0xF) -shl 4)
    $c=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
    if($a -eq $k1 -and $c -eq $k2){ $hits+=('{0:X6} r{1}' -f $j,$r1) }
  }
  return $hits
}
Write-Output ('cpi/sbci 0x00,0x8E: ' + ((FindPairImm 0x4000 0x00 0x8E) -join ' | '))
Write-Output ('cpi/sbci 0x00,0x10: ' + ((FindPairImm 0x4000 0x00 0x10) -join ' | '))
Write-Output ('cpi/subi 0x00,0x8E: ' + ((FindPairImm 0x5000 0x00 0x8E) -join ' | '))
Write-Output ('cpi/subi 0x00,0x10: ' + ((FindPairImm 0x5000 0x00 0x10) -join ' | '))