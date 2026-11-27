$b=[IO.File]::ReadAllBytes('D:\GitCode\famiclone\reference\bmow\bin\femu.bin')
# 在 .data 初值区 0x12BE4-0x12DB5 与整个 flash 里找 232960 (00 8E 03 00 LE) 和 266240 (00 10 04 00)
function FindBytes([byte[]]$pat){
  $h=@()
  for($j=0;$j -lt $b.Length-$pat.Length;$j++){
    $ok=$true
    for($k=0;$k -lt $pat.Length;$k++){ if($b[$j+$k] -ne $pat[$k]){ $ok=$false; break } }
    if($ok){ $h+=('{0:X6}' -f $j) }
  }
  return $h
}
Write-Output ('232960 LE: ' + ((FindBytes @(0x00,0x8E,0x03,0x00)) -join ' '))
Write-Output ('266240 LE: ' + ((FindBytes @(0x00,0x10,0x04,0x00)) -join ' '))
Write-Output ('232960 BE: ' + ((FindBytes @(0x00,0x03,0x8E,0x00)) -join ' '))
# 6656 LE 16位: 00 1A
Write-Output ('6656 LE16: ' + ((FindBytes @(0x00,0x1A)) -join ' '))