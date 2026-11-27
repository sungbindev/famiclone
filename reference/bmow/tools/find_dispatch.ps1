# 定位所有"按镜像类型分发"的判定点，以及类型号→名称/参数主表。
# 新增模式需要：1) 新类型号 2) 名称表项 3) 参数表项 4) 所有 cpi 分支挂接。
$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
$codeLo=0x1FD8; $codeHi=0x12BE4

# 1) 找所有 lds rX, 0x38B6（读镜像类型号）
Write-Output '=== lds 0x38B6 (读类型号) ==='
for($j=$codeLo;$j -lt $codeHi-2;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  if(($w -band 0xFE0F) -ne 0x9000){ continue }  # lds = 1001 000d dddd 0000
  $addr=$b[$j+2]+256*$b[$j+3]
  if($addr -eq 0x38B6){ Write-Output ('  {0:X6} lds r{1}' -f $j,(($w -shr 4) -band 0x1F)) }
}

# 2) 找所有 cpi 0x13 (19=NIB) 与 cpi 0x17/0x18 (23/24=WOZ) 等类型常量
Write-Output '=== cpi 类型常量 (0x0F-0x18) ==='
for($j=$codeLo;$j -lt $codeHi;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  if(($w -band 0xF000) -ne 0x3000){ continue }  # cpi
  $k=($w -band 0xF) -bor ((($w -shr 8) -band 0xF) -shl 4)
  if($k -ge 0x0F -and $k -le 0x18){
    $rd=16+(($w -shr 4) -band 0xF)
    Write-Output ('  {0:X6} cpi r{1}, 0x{2:X2}' -f $j,$rd,$k)
  }
}