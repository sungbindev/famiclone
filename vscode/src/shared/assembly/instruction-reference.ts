// ------------------------------------------------------------------------
// 名称：instruction-reference.ts
// 说明：6502 官方指令的参考资料：名称、作用、影响的标志位与支持的寻址方式，供悬停与补全使用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：只含官方指令；NES 的 2A03 没有十进制模式，涉及十进制标志的指令在说明里注明。
// ------------------------------------------------------------------------

/** 寻址方式的写法与指令字节数。 */
export interface AddressingMode {
  /** 寻址方式的中文名。 */
  readonly name: string;
  /** 操作数的写法，隐含寻址为空字符串。 */
  readonly syntax: string;
  /** 整条指令的字节数。 */
  readonly bytes: number;
}

/** 一条指令的参考资料。 */
export interface InstructionDoc {
  /** 助记符，小写。 */
  readonly mnemonic: string;
  /** 指令的中文名。 */
  readonly title: string;
  /** 作用说明。 */
  readonly summary: string;
  /** 影响的标志位，如 `N Z C`；不影响任何标志位时为“无”。 */
  readonly flags: string;
  /** 支持的寻址方式。 */
  readonly modes: readonly AddressingMode[];
}

/** 全部寻址方式，键用于下面的指令表。 */
const MODES = {
  imp: { name: '隐含', syntax: '', bytes: 1 },
  acc: { name: '累加器', syntax: 'A', bytes: 1 },
  imm: { name: '立即数', syntax: '#$nn', bytes: 2 },
  zp: { name: '零页', syntax: '$nn', bytes: 2 },
  zpx: { name: '零页，X 变址', syntax: '$nn,X', bytes: 2 },
  zpy: { name: '零页，Y 变址', syntax: '$nn,Y', bytes: 2 },
  abs: { name: '绝对', syntax: '$nnnn', bytes: 3 },
  abx: { name: '绝对，X 变址', syntax: '$nnnn,X', bytes: 3 },
  aby: { name: '绝对，Y 变址', syntax: '$nnnn,Y', bytes: 3 },
  ind: { name: '间接', syntax: '($nnnn)', bytes: 3 },
  izx: { name: '先变址间接', syntax: '($nn,X)', bytes: 2 },
  izy: { name: '后变址间接', syntax: '($nn),Y', bytes: 2 },
  rel: { name: '相对', syntax: '标签', bytes: 2 }
} as const satisfies Record<string, AddressingMode>;

/** 寻址方式的键。 */
type ModeKey = keyof typeof MODES;

/** 常见的八种通用寻址方式（ADC、AND、CMP、EOR、LDA、ORA、SBC）。 */
const GENERAL_MODES = 'imm zp zpx abs abx aby izx izy';

/** 构造一条指令：寻址方式用空格分隔的键书写。 */
function instruction(mnemonic: string, title: string, summary: string, flags: string, modes: string): InstructionDoc {
  return {
    mnemonic,
    title,
    summary,
    flags,
    modes: modes.split(' ').map((key) => MODES[key as ModeKey])
  };
}

/** 6502 官方指令，按助记符字母顺序排列。 */
export const INSTRUCTIONS: readonly InstructionDoc[] = [
  instruction('adc', '带进位加法', 'A = A + 操作数 + C。NES 的 2A03 没有十进制模式，D 标志对它无效。', 'N V Z C', GENERAL_MODES),
  instruction('and', '按位与', 'A = A AND 操作数。', 'N Z', GENERAL_MODES),
  instruction('asl', '算术左移', '左移一位，最高位进入 C，最低位补 0；对累加器或内存操作。', 'N Z C', 'acc zp zpx abs abx'),
  instruction('bcc', 'C 为 0 时分支', '进位标志 C 为 0 时跳转到相对地址（-128 到 +127 字节）。', '无', 'rel'),
  instruction('bcs', 'C 为 1 时分支', '进位标志 C 为 1 时跳转到相对地址。', '无', 'rel'),
  instruction('beq', 'Z 为 1 时分支', '零标志 Z 为 1（结果为 0 或两数相等）时跳转到相对地址。', '无', 'rel'),
  instruction('bit', '位测试', 'A AND 操作数只影响 Z；操作数的第 7、6 位分别复制到 N、V。A 不变。', 'N V Z', 'zp abs'),
  instruction('bmi', 'N 为 1 时分支', '负标志 N 为 1 时跳转到相对地址。', '无', 'rel'),
  instruction('bne', 'Z 为 0 时分支', '零标志 Z 为 0（结果不为 0 或两数不等）时跳转到相对地址。', '无', 'rel'),
  instruction('bpl', 'N 为 0 时分支', '负标志 N 为 0 时跳转到相对地址。', '无', 'rel'),
  instruction('brk', '软件中断', '把 PC+2 与状态寄存器（B 置 1）压栈，置 I，再跳转到 $FFFE/$FFFF 的 IRQ 向量。', 'I', 'imp'),
  instruction('bvc', 'V 为 0 时分支', '溢出标志 V 为 0 时跳转到相对地址。', '无', 'rel'),
  instruction('bvs', 'V 为 1 时分支', '溢出标志 V 为 1 时跳转到相对地址。', '无', 'rel'),
  instruction('clc', '清进位', 'C = 0。', 'C', 'imp'),
  instruction('cld', '清十进制模式', 'D = 0。NES 的 2A03 没有十进制模式，但仍可修改该标志位。', 'D', 'imp'),
  instruction('cli', '清中断禁止', 'I = 0，允许 IRQ 中断。', 'I', 'imp'),
  instruction('clv', '清溢出', 'V = 0。', 'V', 'imp'),
  instruction('cmp', '比较累加器', '计算 A - 操作数并只设置标志，不保存结果：A 大于等于操作数时 C = 1，相等时 Z = 1。', 'N Z C', GENERAL_MODES),
  instruction('cpx', '比较 X', '计算 X - 操作数并只设置标志，不保存结果。', 'N Z C', 'imm zp abs'),
  instruction('cpy', '比较 Y', '计算 Y - 操作数并只设置标志，不保存结果。', 'N Z C', 'imm zp abs'),
  instruction('dec', '内存减一', '内存单元的值减 1。', 'N Z', 'zp zpx abs abx'),
  instruction('dex', 'X 减一', 'X = X - 1。', 'N Z', 'imp'),
  instruction('dey', 'Y 减一', 'Y = Y - 1。', 'N Z', 'imp'),
  instruction('eor', '按位异或', 'A = A XOR 操作数。', 'N Z', GENERAL_MODES),
  instruction('inc', '内存加一', '内存单元的值加 1。', 'N Z', 'zp zpx abs abx'),
  instruction('inx', 'X 加一', 'X = X + 1。', 'N Z', 'imp'),
  instruction('iny', 'Y 加一', 'Y = Y + 1。', 'N Z', 'imp'),
  instruction('jmp', '跳转', '无条件跳转到指定地址；间接寻址时从该地址读出 16 位目标地址。间接寻址有页边界缺陷：地址低字节为 $FF 时，高字节取自同一页的开头。', '无', 'abs ind'),
  instruction('jsr', '调用子程序', '把返回地址减 1 压栈，再跳转到指定地址；子程序用 RTS 返回。', '无', 'abs'),
  instruction('lda', '加载累加器', 'A = 操作数。', 'N Z', GENERAL_MODES),
  instruction('ldx', '加载 X', 'X = 操作数。', 'N Z', 'imm zp zpy abs aby'),
  instruction('ldy', '加载 Y', 'Y = 操作数。', 'N Z', 'imm zp zpx abs abx'),
  instruction('lsr', '逻辑右移', '右移一位，最低位进入 C，最高位补 0；对累加器或内存操作。', 'N Z C', 'acc zp zpx abs abx'),
  instruction('nop', '空操作', '什么也不做，只占用时间。', '无', 'imp'),
  instruction('ora', '按位或', 'A = A OR 操作数。', 'N Z', GENERAL_MODES),
  instruction('pha', 'A 压栈', '把 A 压入栈。', '无', 'imp'),
  instruction('php', '状态寄存器压栈', '把状态寄存器（B 位置 1）压入栈。', '无', 'imp'),
  instruction('pla', '弹栈到 A', '从栈弹出一个字节到 A。', 'N Z', 'imp'),
  instruction('plp', '弹栈到状态寄存器', '从栈弹出一个字节到状态寄存器，B 位被忽略。', '全部', 'imp'),
  instruction('rol', '循环左移', '经 C 循环左移一位：C 进入最低位，最高位进入 C。', 'N Z C', 'acc zp zpx abs abx'),
  instruction('ror', '循环右移', '经 C 循环右移一位：C 进入最高位，最低位进入 C。', 'N Z C', 'acc zp zpx abs abx'),
  instruction('rti', '从中断返回', '先弹出状态寄存器，再弹出返回地址（不加 1）。', '全部', 'imp'),
  instruction('rts', '从子程序返回', '弹出返回地址，加 1 后跳转。', '无', 'imp'),
  instruction('sbc', '带借位减法', 'A = A - 操作数 - (1 - C)。NES 的 2A03 没有十进制模式，D 标志对它无效。', 'N V Z C', GENERAL_MODES),
  instruction('sec', '置进位', 'C = 1。', 'C', 'imp'),
  instruction('sed', '置十进制模式', 'D = 1。NES 的 2A03 没有十进制模式，但仍可修改该标志位。', 'D', 'imp'),
  instruction('sei', '置中断禁止', 'I = 1，禁止 IRQ 中断。', 'I', 'imp'),
  instruction('sta', '存储累加器', '内存单元 = A。', '无', 'zp zpx abs abx aby izx izy'),
  instruction('stx', '存储 X', '内存单元 = X。', '无', 'zp zpy abs'),
  instruction('sty', '存储 Y', '内存单元 = Y。', '无', 'zp zpx abs'),
  instruction('tax', 'A 传送到 X', 'X = A。', 'N Z', 'imp'),
  instruction('tay', 'A 传送到 Y', 'Y = A。', 'N Z', 'imp'),
  instruction('tsx', '栈指针传送到 X', 'X = S。', 'N Z', 'imp'),
  instruction('txa', 'X 传送到 A', 'A = X。', 'N Z', 'imp'),
  instruction('txs', 'X 传送到栈指针', 'S = X。', '无', 'imp'),
  instruction('tya', 'Y 传送到 A', 'A = Y。', 'N Z', 'imp')
];

/** 按助记符建立的索引。 */
const BY_MNEMONIC = new Map(INSTRUCTIONS.map((doc) => [doc.mnemonic, doc]));

/**
 * 按助记符查指令资料。
 * @param mnemonic 助记符，不区分大小写。
 * @returns 指令资料；不是官方指令时为 undefined。
 */
export function findInstruction(mnemonic: string): InstructionDoc | undefined {
  return BY_MNEMONIC.get(mnemonic.toLowerCase());
}
