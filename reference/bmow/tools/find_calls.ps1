$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
function S([int]$a,[int]$n){ ($b[$a..($a+$n-1)] | ForEach-Object { [char]$_ }) -join '' }
Write-Output ('0x1089: ' + (S 0x1089 20))
Write-Output ('0x1059: ' + (S 0x1059 20))
Write-Output ('0x1067: ' + (S 0x1067 20))
# 找 call 0x653e（字节地址 0x653E → call 操作数 = 0x653E/2 = 0x329F）
$t=0x329F
$lo=$t -band 0xFF; $hi=($t -shr 8) -band 0xFF
$w1=0x9400 -bor ((($hi -band 0xF) -shl 4)) -bor (($t -shr 12) -band 0x1F0) # call: 1001 010k kkkk 110k
# 直接按编码拼：1001010 KKKKK 110 KKKKKKKK? 标准 call: 1001 010k kkkk 110k, k=22位目标字地址的高… 用简单法：搜字节 0E 94 xx yy 使 (yy<<8|xx)*2 == 0x653E
$hits=@()
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  if($b[$j] -eq 0x0E -and $b[$j+1] -eq 0x94){
    $w2=$b[$j+2]+256*$b[$j+3]
    if($w2*2 -eq 0x653E){ $hits+=('{0:X6}' -f $j) }
  }
}
Write-Output ('call 0x653E sites: ' + ($hits -join ' '))
# 同样找 call 0x4490（NIB 处理函数）
$h2=@()
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  if($b[$j] -eq 0x0E -and $b[$j+1] -eq 0x94){
    $w2=$b[$j+2]+256*$b[$j+3]
    if($w2*2 -eq 0x4490){ $h2+=('{0:X6}' -f $j) }
  }
}
Write-Output ('call 0x4490 sites: ' + ($h2 -join ' '))