# 采集核心补丁点原始字节，供汇编复现被替换指令
$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
$sites=@(0x283A,0x3342,0x5C34,0x6414,0xC912,0x3DAE,0x3148,0x32B8,0x3114,0x31C8,0x31FA)
foreach($s in $sites){
  $hex=($b[$s..($s+7)] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '
  Write-Output ('{0:X6}: {1}' -f $s,$hex)
}