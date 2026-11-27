/* 补丁蹦床链接到空闲 flash 0x12F00（字节地址），只取 .text 段 */
MEMORY
{
  text (rx) : ORIGIN = 0x12F00, LENGTH = 0x2000
}
SECTIONS
{
  .text : { *(.text) } > text
}