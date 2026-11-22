// ------------------------------------------------------------------------
// 名称：call-log.ts
// 说明：读取 Ember 的 inspector.callLogStart 记录的调用记录：每行是“指令地址  寄存器 | STACK 栈顶
//       字节 | 内存段”，指令地址是被记录的那条指令（子程序入口）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：栈顶的前两个字节按 JSR 压入的返回地址减 1 解释（低字节在前）；是否真由 JSR 进入由调用者对照
//       映像核实，这里只做换算。
// ------------------------------------------------------------------------

/** 记录行：四位十六进制的指令地址、两个空格、寄存器 */
const CALL_LINE = /^([0-9A-F]{4}) {2}A:/;

/** 栈顶的前两个字节 */
const STACK_TOP = /\| STACK ([0-9A-F]{2}) ([0-9A-F]{2})/;

/**
 * 取出调用记录里的指令地址与返回地址。
 * @param text 记录文件内容
 * @returns 不重复的被记录的指令地址；不重复的（被记录的地址，栈顶换算出的返回地址）对
 */
export function parseCallLog(text: string): {addresses: number[], returns: Array<{target: number, returnAddress: number}>} {
  const addresses = new Set<number>();
  const returns = new Map<string, {target: number, returnAddress: number}>();
  for (const line of text.split('\n')) {
    const match = CALL_LINE.exec(line);
    if (!match) continue;
    const target = Number.parseInt(match[1]!, 16);
    addresses.add(target);
    const stack = STACK_TOP.exec(line);
    if (stack) {
      const returnAddress = (((Number.parseInt(stack[2]!, 16) << 8) | Number.parseInt(stack[1]!, 16)) + 1) & 0xffff;
      returns.set(`${target}:${returnAddress}`, { target, returnAddress });
    }
  }
  return { addresses: [...addresses], returns: [...returns.values()] };
}
