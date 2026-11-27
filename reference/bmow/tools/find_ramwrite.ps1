$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
# 0x673E 里 0x34C1 与 0x34BD 是 32 位量，来自 0x423C 的 RAM 表 0x382E/0x3826。
# 这些 RAM 表由 main 初始化。找 main 0x3B54 里对 0x3826 区域的填充。
# 搜 ldi 加载 0x26 低字节（r26/r28/r30 等）后跟 0x38 高字节，但之前没找到相邻对。
# 换：搜 sts 0x3826..0x3840 范围（上面只搜到 0x3837/0x3839）。
# 这些表可能用 memcpy 从 flash 拷。找 call 0x12508 且 r24/r25=0x3826 附近。
# 更直接：0x423C 的表基址 0x382E 是 RAM，但 flash 0x382E 处是代码。
# 实际上这些"表"可能在 flash 的高段（RAMPZ=1），通过 0x12308 蹦床读。
# 但 0x423C 用的是 ld（RAM 读），不是 lpm。所以确实是 RAM。
# RAM 0x3826 区域由谁写？搜所有 sts 到 0x3826-0x3840：
$hits=@()
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  if(($w -band 0xFE0F) -ne 0x9200){ continue }
  $addr=$b[$j+2]+256*$b[$j+3]
  if($addr -ge 0x3826 -and $addr -le 0x3840){ $hits+=('{0:X6} sts 0x{1:X4} r{2}' -f $j,$addr,(($w -shr 4) -band 0x1F)) }
}
Write-Output ($hits -join "`n")
# 也找 st X/Y/Z 到这些地址的循环（通过 ldi 指针）。搜 ldi 0x26 后跟 ldi 0x38 到相邻寄存器（可能间隔几条指令）
# 简单：找所有 ldi rX,0x26 和 ldi rX+1,0x38 在 8 字节窗口内
for($j=0x1FD8;$j -lt 0x12BE4-8;$j+=2){
  $w1=$b[$j]+256*$b[$j+1]
  if(($w1 -band 0xF000) -ne 0xE000){ continue }
  $k1=($w1 -band 0xF) -bor ((($w1 -shr 8) -band 0xF) -shl 4)
  if($k1 -ne 0x26){ continue }
  $r1=16+(($w1 -shr 4) -band 0xF)
  for($d=2;$d -le 8;$d+=2){
    $w2=$b[$j+$d]+256*$b[$j+$d+1]
    if(($w2 -band 0xF000) -ne 0xE000){ continue }
    $k2=($w2 -band 0xF) -bor ((($w2 -shr 8) -band 0xF) -shl 4)
    $r2=16+(($w2 -shr 4) -band 0xF)
    if($k2 -eq 0x38 -and $r2 -eq $r1+1){ Write-Output ('{0:X6} ldi r{1},0x26 ... +{2} ldi r{3},0x38' -f $j,$r1,$d,$r2) }
  }
}