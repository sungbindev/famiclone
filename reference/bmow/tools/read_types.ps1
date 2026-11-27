$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
function S([int]$a,[int]$n){ $s=''; for($i=$a;$i -lt $a+$n;$i++){ $c=$b[$i]; if($c -eq 0){break}; $s+=[char]$c }; return $s }
foreach($p in @(0x1212,0x11D8,0x11CC,0x11BF,0x11B3,0x11AA,0x1202,0x11F2,0x11E5,0x119E,0x1192)){
  Write-Output ('{0:X4}: {1}' -f $p,(S $p 30))
}
# 类型 19 (0x13) 是 "140K NIB"? 表索引：0x12C80 + 2*type
# 0x12C96 表第 type 字节。type 19 → 0x12C96+19 = 0x12CA9 = 0x25? 数一下：01 03 05 07 09 0E 10 12 14 16 18 1C 1E 25 75 00 → 索引13=0x25(37), 14=0x75(117)
# 但表只有16字节，type 19 超界 → 说明 NIB 类型号不是 19。找 "140K NIB" 指针 0x11AA 在表里的索引：(0x11AA 是第6个, 索引5)
Write-Output ('param[5] = {0:X2}' -f $b[0x12C96+5])
Write-Output ('param[13] = {0:X2}' -f $b[0x12C96+13])
Write-Output ('param[14] = {0:X2}' -f $b[0x12C96+14])