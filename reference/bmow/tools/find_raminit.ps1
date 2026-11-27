$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
# 0x423C: r18=type*4; Z = r18 + 0x382E → 第一张表（0x34C1）
# 0x42B2: Z = r18 + 0x3826 → 第二张表（0x34BD）
# 0x42DC: Z = r18 + 0x383A → 第三张表（0x3405）
# 这些是 flash 里的表（文件偏移 = flash 字节地址，RAMPZ=0 区域 <0x10000? 0x3826 >0x10000? 不，0x3826 < 0x10000）
# 等等：0x3826 是 flash 地址 0x3826，文件偏移就是 0x3826。但上面读出来是乱码 → 说明这些表在 RAM！
# subi 0xD2, sbci 0xC7 → 减 0x37D2? 计算：subi r30,0xD2 即 r30 -= 0xD2; sbci r31,0xC7 即 r31 -= 0xC7 + carry
# 合起来 Z -= 0x37D2（若低字节借位）… 实际是 Z = type*4 - 0x37D2 + ... 不对。
# 正确：subi/sbci 对 = 减去 16 位立即数 0xC7D2。Z = type*4 + (0x10000 - 0xC7D2) = type*4 + 0x382E。对。
# 但 0x382E 是 RAM 地址（ATmega1284 RAM 0x100-0x40FF）！这些表在 RAM，由启动时从 flash 拷贝。
# 找 RAM 0x3826/0x382E/0x383A 的初始化来源：搜 sts 0x3826 等，或 memcpy 到 0x3826。
$hits=@()
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  if(($w -band 0xFE0F) -ne 0x9200){ continue }
  $addr=$b[$j+2]+256*$b[$j+3]
  if($addr -ge 0x3820 -and $addr -le 0x3850){ $hits+=('{0:X6} sts 0x{1:X4}' -f $j,$addr) }
}
Write-Output ($hits -join "`n")
# 也找 ldi 对加载 0x3826/0x382E/0x383A 作为指针
function FindLdiPair([int]$ka,[int]$kc){
  $h=@()
  for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
    $w1=$b[$j]+256*$b[$j+1]
    $w2=$b[$j+2]+256*$b[$j+3]
    if(($w1 -band 0xF000) -ne 0xE000 -or ($w2 -band 0xF000) -ne 0xE000){ continue }
    $r1=16+(($w1 -shr 4) -band 0xF)
    $r2=16+(($w2 -shr 4) -band 0xF)
    if($r2 -ne $r1+1){ continue }
    $k1=($w1 -band 0xF) -bor ((($w1 -shr 8) -band 0xF) -shl 4)
    $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
    if($k1 -eq $ka -and $k2 -eq $kc){ $h+=('{0:X6} r{1}' -f $j,$r1) }
  }
  return $h
}
Write-Output ('ldi 0x3826: ' + ((FindLdiPair 0x26 0x38) -join ' | '))
Write-Output ('ldi 0x382E: ' + ((FindLdiPair 0x2E 0x38) -join ' | '))
Write-Output ('ldi 0x383A: ' + ((FindLdiPair 0x3A 0x38) -join ' | '))