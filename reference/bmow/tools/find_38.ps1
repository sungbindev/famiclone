$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
# .data 初值 210 字节 0x12BE4-0x12DB5 → RAM 0x100-0x1D1。RAM 0x3826 不在其中。
# 0x423C 的表基址 0x3826/0x382E/0x383A 是 RAM，但上面读 flash 同偏移是乱码 → 这些 RAM 表由代码运行时填充。
# 找填充循环：搜 ldi 对 0x3826/0x382E/0x383A 没找到 → 可能用 subi/sbci 从别的基址算，或 memcpy。
# 换个思路：0x423C 里 Z = type*4 + 0x382E。type*4 在 r18:r19。0x382E 是 RAM。
# 但等等——ATmega1284 的 RAM 是 0x100-0x40FF，0x382E 确实在 RAM。
# 这些表可能是 main 里用循环从 flash 表拷来的。找 call 0x12508 (memcpy) 且目标 0x3826 附近。
# 更直接：搜整个代码里 ldi 加载 0x26/0x2E/0x3A 低字节 + 0x38 高字节的所有组合（包括非相邻寄存器对）
function FindLdi([int]$k){
  $h=@()
  for($j=0x1FD8;$j -lt 0x12BE4;$j+=2){
    $w=$b[$j]+256*$b[$j+1]
    if(($w -band 0xF000) -ne 0xE000){ continue }
    $kk=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
    if($kk -eq $k){ $h+=('{0:X6} r{1}' -f $j,(16+(($w -shr 4) -band 0xF))) }
  }
  return $h
}
Write-Output ('ldi 0x38: ' + ((FindLdi 0x38) -join ' | '))