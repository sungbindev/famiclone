// ------------------------------------------------------------------------
// 名称：trace-log.mjs
// 说明：读取 Ember 的 inspector.traceStart（kind 为 instruction）记录的指令轨迹：每行是 nestest 日志的写法，行首是四位十六进制的指令地址。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：轨迹里的每个地址都是被真实执行过的指令开头，比 CDL 的“代码字节”更精确；用作追踪代码的入口。
//       io、irq 与调用记录的行不是指令行，一律忽略。
// ------------------------------------------------------------------------

/** 指令行：四位十六进制的地址、两个空格，后面是至少一个两位十六进制的字节（调用记录的“8123  A:01”不符合） */
const TRACE_LINE = /^([0-9A-F]{4}) {2}[0-9A-F]{2}(?: |$)/;

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
