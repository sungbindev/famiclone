// ------------------------------------------------------------------------
// 名称：disasm-command.mjs
// 说明：disasm 命令：把 NES 程序或平铺二进制反汇编成 ca65 源码，并回读校验。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：iNES 文件读取 PRG 与中断向量，自动加入 NES 的 PPU 寄存器符号；平铺二进制必须指定加载地址与入口。
// ------------------------------------------------------------------------
import path from 'node:path';
import { disassemble } from '../../disasm/index.mjs';
import { loadProjectConfig } from '../../config/index.mjs';
import { FcdevError } from '../../shared/index.mjs';
import { BUILTIN_TARGETS_DIR } from '../../targets/index.mjs';
import { locateToolchain } from '../../toolchain/index.mjs';

/** 十六进制地址：可带 $ 或 0x 前缀，最多 4 位 */
const ADDRESS = /^(?:\$|0[xX])?([0-9A-Fa-f]{1,4})$/;

/**
 * 解析命令行里的地址。
 * @param {string} text 地址文本，如 8000、$8000、0x8000
 * @param {string} option 选项名，用于出错提示
 * @returns {number} 地址
 * @throws {FcdevError} 不是合法的 16 位十六进制地址
 */
export function parseAddress(text, option) {
  const match = ADDRESS.exec(text);
  if (!match) throw new FcdevError(`${option} 需要 16 位十六进制地址，实际为“${text}”`, { code: 'ARGUMENT_INVALID' });
  return Number.parseInt(match[1], 16);
}

/**
 * 执行 disasm 命令。
 * @param {{positionals: string[], options: {format?: string, base?: string, cdl?: string, trace?: string, symbols?: string, out?: string, entries: string[]}, context: import('../index.mjs').CliContext}} input 参数与运行环境
 * @returns {Promise<number>} 退出码
 * @throws {FcdevError} 参数不合法、找不到工具链，或回读校验不一致
 */
export async function runDisasmCommand({ positionals, options, context }) {
  if (positionals.length !== 1) {
    throw new FcdevError('disasm 需要一个输入文件', { code: 'ARGUMENT_INVALID', hint: '用法：npx fcdev disasm <文件> [--format ines|binary] [--base 地址] [--entry 地址] [--cdl 文件] [--symbols 文件] [--out 目录]' });
  }
  const inputFile = path.resolve(context.cwd, positionals[0]);
  const config = loadProjectConfig(context.cwd, context.cwd);
  const toolchain = locateToolchain({ configured: config.toolchain, env: context.env });
  const nesRegisters = path.join(BUILTIN_TARGETS_DIR, 'nes', 'asminc', 'fcnes.inc');

  const report = await disassemble({
    inputFile,
    format: options.format,
    base: options.base === undefined ? undefined : parseAddress(options.base, '--base'),
    entries: options.entries.map((entry) => parseAddress(entry, '--entry')),
    cdlFile: options.cdl === undefined ? undefined : path.resolve(context.cwd, options.cdl),
    traceFile: options.trace === undefined ? undefined : path.resolve(context.cwd, options.trace),
    symbolsFile: options.symbols === undefined ? undefined : path.resolve(context.cwd, options.symbols),
    outDir: path.resolve(context.cwd, options.out ?? path.join('build', 'disasm', path.basename(inputFile, path.extname(inputFile)))),
    toolchain,
    includes: [nesRegisters],
  });

  for (const warning of report.warnings.slice(0, 10)) context.stderr.write(`警告：${warning}\n`);
  if (report.warnings.length > 10) context.stderr.write(`警告：还有 ${report.warnings.length - 10} 条，省略\n`);
  context.stdout.write(`${report.description}：${report.size} 字节，指令 ${report.instructions} 条（${report.codeBytes} 字节），其余 ${report.size - report.codeBytes} 字节按数据处理\n`);
  context.stdout.write(`回读校验通过：重新汇编的结果与原映像逐字节一致\n已生成 ${path.relative(context.cwd, report.sourceFile) || report.sourceFile}\n`);
  return 0;
}
