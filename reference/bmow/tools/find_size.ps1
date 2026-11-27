# 找 cpi/sbci/subi 中的 16 位常量对：0x38E3=14563（35×416? no）… 主要找 232960=0x38E00 与 266240=0x41000 的高16位 0x38E / 0x410
# 232960 = 0x00038E00，266240 = 0x00041000。32 位比较会拆成 cpi/sbci 序列，高字节 0x03 / 0x04。
# 更实际：找 cpi rX,0x8E + sbci rY,0x03（232960 的 bits8-23? no: 0x38E00 → byte1=0x8E, byte2=0x03）
# 以及 byte1=0x10, byte2=0x04（266240）。
$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
# cpi Rd,K : 0011 KKKK dddd KKKK, Rd=r16..31 → (w>>4)&0xF = Rd-16
# sbci/subi Rd,K: 0100 KKKK dddd KKKK (subi=0x5000, sbci=0x4000)
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
Write-Output ('cpi/sbci 0x8E,0x03 (232960 mid bytes): ' + ((FindPairImm 0x4000 0x8E 0x03) -join ' | '))
Write-Output ('cpi/sbci 0x10,0x04 (266240 mid bytes): ' + ((FindPairImm 0x4000 0x10 0x04) -join ' | '))
Write-Output ('cpi/subi 0x8E,0x03: ' + ((FindPairImm 0x5000 0x8E 0x03) -join ' | '))
Write-Output ('cpi/subi 0x10,0x04: ' + ((FindPairImm 0x5000 0x10 0x04) -join ' | '))
# 磁道数 35=0x23, 40=0x28, 140=0x8C, 160=0xA0 单字节 cpi（任何 r16-31）
function FindCpi([int]$k){
  $hits=@()
  for($j=0x1FD8;$j -lt 0x12BE4;$j+=2){
    $w=$b[$j]+256*$b[$j+1]
    if(($w -band 0xF000) -ne 0x3000){ continue }
    $a=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
    if($a -eq $k){ $hits+=('{0:X6}' -f $j) }
  }
  return $hits
}
Write-Output ('cpi 35: ' + ((FindCpi 35) -join ' '))
Write-Output ('cpi 40: ' + ((FindCpi 40) -join ' '))
Write-Output ('cpi 140: ' + ((FindCpi 140) -join ' '))
Write-Output ('cpi 160: ' + ((FindCpi 160) -join ' '))