# 每个 lds 0x33FE 点后反汇编 24 字节，看模式比较常量
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$bytes=[IO.File]::ReadAllBytes($b)
$sites=@()
for($j=0x1FD8;$j -lt 0x12BE4-2;$j+=2){
  $w=$bytes[$j]+256*$bytes[$j+1]
  if(($w -band 0xFE0F) -ne 0x9000){ continue }
  $addr=$bytes[$j+2]+256*$bytes[$j+3]
  if($addr -eq 0x33FE){ $sites+=$j }
}
$out='D:\GitCode\famiclone\reference\bmow\disasm\mode-sites.txt'
$sb=New-Object System.Text.StringBuilder
foreach($s in $sites){
  [void]$sb.AppendLine(('--- site {0:X6} ---' -f $s))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $s) ('--stop-address=0x{0:X}' -f ($s+24)) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output ('{0} sites written' -f $sites.Count)