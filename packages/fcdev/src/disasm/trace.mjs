// ------------------------------------------------------------------------
// 名称：trace.mjs
// 说明：读取模拟器 trace 命令记录的指令轨迹：每行以 T 开头，第二列是执行到的指令地址。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：轨迹里的每个地址都是被真实执行过的指令开头，比 CDL 的“代码字节”更精确；用作追踪代码的入口。
// ------------------------------------------------------------------------

/** 轨迹的一行：T、四位十六进制的指令地址，后面是物理地址、字节、寄存器 */
const TRACE_LINE = /^T ([0-9A-Fa-f]{4}) /;

/**
 * 取出轨迹里被执行过的指令地址。
 * @param {string} text 轨迹文件内容
 * @returns {number[]} 不重复的地址，按首次出现的顺序
 */
export function parseTrace(text) {
  const seen = new Set();
  for (const line of text.split('\n')) {
    const match = TRACE_LINE.exec(line);
    if (match) seen.add(Number.parseInt(match[1], 16));
  }
  return [...seen];
}
