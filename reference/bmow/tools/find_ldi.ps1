# 正确解码 ldi：K = (w & 0xF) | (((w >> 8) & 0xF) << 4)；Rd = 16 + ((w >> 4) & 0xF)
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
    if($k1 -eq $ka -and $k2 -eq $kc){ $hits+=('{0:X6} r{1}/r{2}' -f $j,$r1,$r2) }
  }
  return $hits
}
Write-Output ('6656 (0x1A00) lo=0 hi=26: ' + ((FindLdiPair 0 26) -join ' | '))
Write-Output ('3328 (0x0D00) lo=0 hi=13: ' + ((FindLdiPair 0 13) -join ' | '))
Write-Output ('910 (0x038E) lo=0x8E hi=3: ' + ((FindLdiPair 0x8E 3) -join ' | '))
Write-Output ('1040 (0x0410) lo=0x10 hi=4: ' + ((FindLdiPair 0x10 4) -join ' | '))