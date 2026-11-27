$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
# 0x423C 用的表基址：Z = type*4 + 0x382E (subi 0xD2, sbci 0xC7 → 0x382E)
# 以及 0x34BD 表：Z = type*4 + 0x3826 (subi 0xDA, sbci 0xC7 → 0x3826)
# 0x3405 表：Z = type*4 + 0x383A (subi 0xC6, sbci 0xC7 → 0x383A)
foreach($base in @(0x3826,0x382E,0x383A)){
  Write-Output ('base 0x{0:X4}:' -f $base)
  for($t=0;$t -lt 26;$t++){
    $o=$base+4*$t
    $v=[uint32]$b[$o]+256*[uint32]$b[$o+1]+65536*[uint32]$b[$o+2]+16777216*[uint32]$b[$o+3]
    Write-Output ('  type {0,2}: {1,10} 0x{2:X8}' -f $t,$v,$v)
  }
}