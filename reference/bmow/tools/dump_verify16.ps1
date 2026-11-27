# 查候选区附近指针引用的实际用法（读多少字节、lpm 还是 ld）
$od='D:\GitCode\famiclone\reference\bmow\tools\avr-gcc-16.1.0-x64-windows\bin\avr-objdump.exe'
$b='D:\GitCode\famiclone\reference\bmow\bin\femu.bin'
$out='D:\GitCode\famiclone\reference\bmow\disasm\verify16.txt'
$sb=New-Object System.Text.StringBuilder
$sites=@(
  @(0xB110,0xB140,'0x7DF user B118'),
  @(0xBE90,0xBEC0,'0x7DF user BE98 (Z ptr)'),
  @(0xF07C,0xF0A8,'0x800 user F084'),
  @(0x102E4,0x10310,'0x800 user 102EC'),
  @(0xD16A,0xD198,'0xBDF user D172'),
  @(0xD57A,0xD5A8,'0xBDF user D582'),
  @(0x23A6,0x23D0,'0x816 user 23AE')
)
foreach($s in $sites){
  [void]$sb.AppendLine(('===== {0:X4}-{1:X4} {2} =====' -f $s[0],$s[1],$s[2]))
  $lines=& $od -D -b binary -m avr:51 ('--start-address=0x{0:X}' -f $s[0]) ('--stop-address=0x{0:X}' -f $s[1]) $b | Select-Object -Skip 7
  foreach($l in $lines){ [void]$sb.AppendLine($l) }
}
[IO.File]::WriteAllText($out,$sb.ToString())
Write-Output 'done'