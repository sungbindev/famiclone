# 顶层模拟模式号在 RAM 0x33FE。枚举所有读写点与 cpi 比较，弄清模式菜单/分发。
$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
$codeLo=0x1FD8; $codeHi=0x12BE4
Write-Output '=== lds 0x33FE (读模式号) ==='
for($j=$codeLo;$j -lt $codeHi-2;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  if(($w -band 0xFE0F) -ne 0x9000){ continue }
  $addr=$b[$j+2]+256*$b[$j+3]
  if($addr -eq 0x33FE){ Write-Output ('  {0:X6} lds r{1}' -f $j,(($w -shr 4) -band 0x1F)) }
}
Write-Output '=== sts 0x33FE (写模式号) ==='
for($j=$codeLo;$j -lt $codeHi-2;$j+=2){
  $w=$b[$j]+256*$b[$j+1]
  if(($w -band 0xFE0F) -ne 0x9200){ continue }
  $addr=$b[$j+2]+256*$b[$j+3]
  if($addr -eq 0x33FE){ Write-Output ('  {0:X6} sts r{1}' -f $j,(($w -shr 4) -band 0x1F)) }
}