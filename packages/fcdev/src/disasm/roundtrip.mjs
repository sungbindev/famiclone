// ------------------------------------------------------------------------
// 名称：roundtrip.mjs
// 说明：回读校验：把反汇编出的 ca65 源码重新汇编、链接，与原映像逐字节比较。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：链接配置只放一个从映像起始地址开始、大小恰好等于映像的内存区与一个 CODE 段；不一致时报告第一个不同的偏移与地址。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../shared/index.mjs';
import { runTool } from '../toolchain/index.mjs';

/**
 * 生成回读用的链接配置。
 * @param {number} base 起始地址
 * @param {number} size 映像大小
 * @returns {string} ld65 配置内容
 */
export function renderRoundtripConfig(base, size) {
  return `MEMORY { ROM: file = %O, start = $${base.toString(16).toUpperCase()}, size = $${size.toString(16).toUpperCase()}; }
SEGMENTS { CODE: load = ROM, type = ro; }
`;
}

/**
 * 找出两段字节的第一个差异。
 * @param {Buffer} expected 原映像
 * @param {Buffer} actual 回读得到的映像
 * @returns {number} 第一个不同的偏移；完全相同时为 -1
 */
export function findFirstDifference(expected, actual) {
  const common = Math.min(expected.length, actual.length);
  for (let offset = 0; offset < common; offset += 1) {
    if (expected[offset] !== actual[offset]) return offset;
  }
  return expected.length === actual.length ? -1 : common;
}

/**
 * 运行一个工具，失败时抛出带输出的错误。
 * @param {import('../toolchain/index.mjs').Toolchain} toolchain 工具链
 * @param {string} tool 工具名
 * @param {string[]} args 参数
 * @param {string} failure 失败说明
 * @returns {Promise<void>}
 * @throws {FcdevError} 工具退出码非零
 */
async function runOrThrow(toolchain, tool, args, failure) {
  const result = await runTool(toolchain, tool, args);
  if (result.code !== 0) throw new FcdevError(failure, { code: 'DISASM_ROUNDTRIP_FAILED', hint: `${result.stdout}${result.stderr}`.trim() });
}

/**
 * 重新汇编反汇编出的源码并与原映像比较。
 * @param {{toolchain: import('../toolchain/index.mjs').Toolchain, sourceFile: string, expected: Buffer, base: number, workDir: string, name: string}} input 工具链、源码、原映像、起始地址、工作目录与文件名前缀
 * @returns {Promise<{ok: boolean, firstDifference: number}>} 是否一致，以及第一个不同的偏移（一致时为 -1）
 * @throws {FcdevError} 汇编或链接失败
 */
export async function verifyRoundtrip({ toolchain, sourceFile, expected, base, workDir, name }) {
  const objectFile = path.join(workDir, `${name}.roundtrip.o`);
  const configFile = path.join(workDir, `${name}.roundtrip.cfg`);
  const outputFile = path.join(workDir, `${name}.roundtrip.bin`);
  writeFileSync(configFile, renderRoundtripConfig(base, expected.length));
  await runOrThrow(toolchain, 'ca65', [sourceFile, '-o', objectFile], `重新汇编 ${path.basename(sourceFile)} 失败`);
  await runOrThrow(toolchain, 'ld65', ['-C', configFile, '-o', outputFile, objectFile], '重新链接失败');
  const firstDifference = findFirstDifference(expected, readFileSync(outputFile));
  return { ok: firstDifference === -1, firstDifference };
}
