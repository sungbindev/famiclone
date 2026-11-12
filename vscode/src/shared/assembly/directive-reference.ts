// ------------------------------------------------------------------------
// 名称：directive-reference.ts
// 说明：ca65 汇编指令与常用伪函数的参考资料：写法与作用，供悬停与补全使用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只收常用的指令，不是 ca65 的完整手册；指令名一律小写，匹配时不区分大小写。
// ------------------------------------------------------------------------

/** 一条汇编指令的参考资料。 */
export interface DirectiveDoc {
  /** 指令名，小写，含前导点。 */
  readonly name: string;
  /** 写法。 */
  readonly syntax: string;
  /** 作用说明。 */
  readonly summary: string;
}

/** 构造一条指令资料。 */
function directive(name: string, syntax: string, summary: string): DirectiveDoc {
  return { name, syntax, summary };
}

/** 常用的 ca65 汇编指令与伪函数；指令按名称字母顺序排列，伪函数排在最后。 */
export const DIRECTIVES: readonly DirectiveDoc[] = [
  directive('.addr', '.addr 表达式[, ...]', '定义 16 位地址数据，同 .word，用于存放地址。'),
  directive('.align', '.align 边界[, 填充值]', '把当前位置对齐到指定字节数的整数倍，空出的部分用填充值补齐。'),
  directive('.asciiz', '.asciiz "字符串"[, ...]', '定义以 0 结尾的字符串。'),
  directive('.assert', '.assert 条件, 动作[, 消息]', '断言：条件不成立时按动作（warning、error、ldwarning、lderror）报告；条件含链接期才能确定的值时在链接时检查。'),
  directive('.bss', '.bss', '切换到 BSS 段：只占地址空间，不占文件空间，初值不保证。'),
  directive('.byte', '.byte 表达式或字符串[, ...]', '定义字节数据，可以混写数字与字符串，也可写作 .byt。'),
  directive('.charmap', '.charmap 字符码, 目标码', '设置字符串里的字符到目标字符码的映射。'),
  directive('.code', '.code', '切换到 CODE 段。'),
  directive('.data', '.data', '切换到 DATA 段，放有初值的可写数据。'),
  directive('.dbyt', '.dbyt 表达式[, ...]', '定义 16 位大端（高字节在前）数据。'),
  directive('.define', '.define 名字[(参数)] 替换文字', '定义文本替换：之后出现该名字的地方按替换文字展开。'),
  directive('.dword', '.dword 表达式[, ...]', '定义 32 位小端数据。'),
  directive('.else', '.else', '条件汇编：前面的 .if 条件不成立时汇编这一段。'),
  directive('.elseif', '.elseif 条件', '条件汇编：前面的条件都不成立且本条件成立时汇编这一段。'),
  directive('.endenum', '.endenum', '结束 .enum。'),
  directive('.endif', '.endif', '结束条件汇编。'),
  directive('.endmacro', '.endmacro', '结束 .macro，也可写作 .endmac。'),
  directive('.endproc', '.endproc', '结束 .proc。'),
  directive('.endrepeat', '.endrepeat', '结束 .repeat，也可写作 .endrep。'),
  directive('.endscope', '.endscope', '结束 .scope。'),
  directive('.endstruct', '.endstruct', '结束 .struct。'),
  directive('.enum', '.enum [名字]', '定义枚举：成员依次取 0、1、2……，可用 = 指定值。'),
  directive('.error', '.error "消息"', '输出错误并使汇编失败。'),
  directive('.export', '.export 名字[, ...]', '把符号导出给链接器，供其他文件用 .import 引用。'),
  directive('.exportzp', '.exportzp 名字[, ...]', '导出零页符号，使其他文件按零页寻址使用。'),
  directive('.feature', '.feature 特性[, ...]', '打开或关闭 ca65 的可选特性，如 c_comments、labels_without_colons。'),
  directive('.global', '.global 名字[, ...]', '声明全局符号：本文件定义了就导出，没定义就当作导入。'),
  directive('.globalzp', '.globalzp 名字[, ...]', '声明零页的全局符号。'),
  directive('.hibytes', '.hibytes 表达式[, ...]', '定义各表达式的高字节。'),
  directive('.if', '.if 条件', '条件汇编：条件为真时汇编下面的内容。'),
  directive('.ifblank', '.ifblank 参数', '条件汇编：参数为空时汇编，用在宏里。'),
  directive('.ifconst', '.ifconst 表达式', '条件汇编：表达式是常量时汇编。'),
  directive('.ifdef', '.ifdef 符号', '条件汇编：符号已定义时汇编。'),
  directive('.ifnblank', '.ifnblank 参数', '条件汇编：参数不为空时汇编，用在宏里。'),
  directive('.ifndef', '.ifndef 符号', '条件汇编：符号未定义时汇编。'),
  directive('.import', '.import 名字[, ...]', '引用其他文件用 .export 导出的符号。'),
  directive('.importzp', '.importzp 名字[, ...]', '引用其他文件导出的零页符号。'),
  directive('.incbin', '.incbin "文件"[, 偏移[, 长度]]', '把二进制文件的内容原样放进当前位置，可指定起始偏移与长度。'),
  directive('.include', '.include "文件"', '在这里插入另一个源文件的内容；文件按 -I 指定的目录查找。'),
  directive('.local', '.local 名字[, ...]', '在宏里声明局部符号，每次展开时生成不同的名字。'),
  directive('.lobytes', '.lobytes 表达式[, ...]', '定义各表达式的低字节。'),
  directive('.macro', '.macro 名字 [参数, ...]', '开始定义宏，到 .endmacro 结束，也可写作 .mac。'),
  directive('.org', '.org 地址', '设置当前的程序计数器（只在没有链接配置的绝对模式下使用）。'),
  directive('.out', '.out "消息"', '汇编时输出一条消息。'),
  directive('.pagelength', '.pagelength 行数', '设置列表文件每页的行数。'),
  directive('.popseg', '.popseg', '恢复上一次 .pushseg 保存的段。'),
  directive('.proc', '.proc 名字', '开始一个过程：自带作用域，名字同时是过程入口，到 .endproc 结束。'),
  directive('.pushseg', '.pushseg', '保存当前段，之后用 .popseg 恢复。'),
  directive('.repeat', '.repeat 次数[, 计数器名]', '重复汇编下面的内容，到 .endrepeat 结束。'),
  directive('.res', '.res 字节数[, 填充值]', '保留指定字节数的空间，可指定填充值；在 BSS 与零页段里只占地址。'),
  directive('.rodata', '.rodata', '切换到 RODATA 段，放只读数据。'),
  directive('.scope', '.scope [名字]', '开始一个作用域：里面的符号只在作用域内可见，到 .endscope 结束。'),
  directive('.segment', '.segment "段名"', '切换到指定的段，段在链接配置里分配到内存区。'),
  directive('.setcpu', '.setcpu "cpu"', '设置指令集，如 "6502"、"65C02"。'),
  directive('.smart', '.smart [开关]', '打开或关闭智能模式；6502 系列没有用处。'),
  directive('.struct', '.struct [名字]', '定义结构：成员用 .byte、.word、.res 等声明大小，到 .endstruct 结束。'),
  directive('.undef', '.undef 名字', '取消 .define 的定义。'),
  directive('.warning', '.warning "消息"', '输出警告，汇编继续。'),
  directive('.word', '.word 表达式[, ...]', '定义 16 位小端数据。'),
  directive('.zeropage', '.zeropage', '切换到 ZEROPAGE 段，放零页变量。'),
  directive('.blank', '.blank(参数)', '伪函数：参数为空时为真。'),
  directive('.concat', '.concat(字符串, ...)', '伪函数：连接字符串。'),
  directive('.const', '.const(表达式)', '伪函数：表达式是常量时为真。'),
  directive('.defined', '.defined(符号)', '伪函数：符号已定义时为真。'),
  directive('.hibyte', '.hibyte(表达式)', '伪函数：取表达式的高字节。'),
  directive('.lobyte', '.lobyte(表达式)', '伪函数：取表达式的低字节。'),
  directive('.sizeof', '.sizeof(名字)', '伪函数：取结构、枚举或带大小的符号所占的字节数。'),
  directive('.strlen', '.strlen(字符串)', '伪函数：字符串的长度。')
];

/** 按名称建立的索引。 */
const BY_NAME = new Map(DIRECTIVES.map((doc) => [doc.name, doc]));

/**
 * 按名称查汇编指令资料。
 * @param name 指令名，含前导点，不区分大小写。
 * @returns 指令资料；没有收录时为 undefined。
 */
export function findDirective(name: string): DirectiveDoc | undefined {
  return BY_NAME.get(name.toLowerCase());
}
