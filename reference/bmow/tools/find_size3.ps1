$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
# 搜 flash 里 232960 的各种表示：
# 0x38E00 = 232960。32位 LE: 00 8E 03 00（已搜，无）
# 但 NIB 每道 6656，35 道。6656 = 0x1A00。
# 也许大小判定用 6656*35 在运行时算，或用 910 扇区 (232960/256=910=0x38E)。
# 搜 910 LE16: 8E 03；1040 LE16: 10 04
function FindBytes([byte[]]$pat,[int]$lo,[int]$hi){
  $h=@()
  for($j=$lo;$j -lt $hi-$pat.Length;$j++){
    $ok=$true
    for($k=0;$k -lt $pat.Length;$k++){ if($b[$j+$k] -ne $pat[$k]){ $ok=$false; break } }
    if($ok){ $h+=('{0:X6}' -f $j) }
  }
  return $h
}
Write-Output ('910 LE16 (8E 03) in code: ' + ((FindBytes @(0x8E,0x03) 0x1FD8 0x12BE4) -join ' '))
Write-Output ('1040 LE16 (10 04) in code: ' + ((FindBytes @(0x10,0x04) 0x1FD8 0x12BE4) -join ' '))
Write-Output ('6656 LE16 (00 1A) in code: ' + ((FindBytes @(0x00,0x1A) 0x1FD8 0x12BE4) -join ' '))
# 也搜 35*6656 的中间量：232960/4=58240=0xE380, /16=14560=0x38E0
Write-Output ('14560 LE16 (E0 38): ' + ((FindBytes @(0xE0,0x38) 0x1FD8 0x12BE4) -join ' '))
Write-Output ('58240 LE16 (80 E3): ' + ((FindBytes @(0x80,0xE3) 0x1FD8 0x12BE4) -join ' '))