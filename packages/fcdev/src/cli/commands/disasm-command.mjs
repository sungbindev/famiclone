// ------------------------------------------------------------------------
// 名称：disasm-command.mjs
// 说明：disasm 命令：把 NES 程序或平铺二进制反汇编成 ca65 源码工程，并回读校验。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：iNES 文件读取 PRG 与中断向量，自动加入 NES 的 PPU 寄存器符号；SB-DOS 程序（--format sbdos）用文件头的 JMP 做入口、加入学习机的符号集；平铺二进制必须指定加载地址与入口；输出是一个反汇编工程目录。
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
    throw new FcdevError('disasm 需要一个输入文件', { code: 'ARGUMENT_INVALID', hint: '用法：npx fcdev disasm <文件> [--format ines|flat|sbdos] [--base 地址] [--entry 地址] [--cdl 文件] [--trace 文件] [--symbols 文件] [--out 目录]' });
  }
  const inputFile = path.resolve(context.cwd, positionals[0]);
  const config = loadProjectConfig(context.cwd, context.cwd);
  const toolchain = locateToolchain({ configured: config.toolchain, env: context.env });
  const registers = path.join(BUILTIN_TARGETS_DIR, options.format === 'sbdos' ? 'sb2000' : 'nes', 'asminc', options.format === 'sbdos' ? 'sb2000.inc' : 'fcnes.inc');

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
    symbolSetFiles: [registers],
  });

  for (const warning of report.warnings.slice(0, 10)) context.stderr.write(`警告：${warning}\n`);
  if (report.warnings.length > 10) context.stderr.write(`警告：还有 ${report.warnings.length - 10} 条，省略\n`);
  const { status } = report;
  context.stdout.write(`${report.description}：映像 ${report.size} 字节，其中 ${report.mappedBytes} 字节在 CPU 地址空间里；追踪出指令 ${report.instructions} 条，代码区域 ${report.codeBytes} 字节\n`);
  context.stdout.write(`回读校验通过：重新汇编的结果与原映像逐字节一致\n已生成工程 ${path.relative(context.cwd, report.projectDir) || report.projectDir}\n`);
  context.stdout.write(`符号 ${status.symbols.total} 个（自动命名 ${status.symbols.auto} 个），没有说明的过程 ${status.proceduresWithoutComment} 个，未解析的间接跳转 ${status.unresolved.indirectJumps} 处、跨 bank 转移 ${status.unresolved.crossBank} 处\n`);
  return 0;
}
