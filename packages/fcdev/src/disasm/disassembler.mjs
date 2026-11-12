// ------------------------------------------------------------------------
// 名称：disassembler.mjs
// 说明：反汇编主流程：读取映像、追踪代码、生成 da65 信息文件、运行 da65、回读校验。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：产物放在输出目录：<名>.bin（映像）、<名>.info（信息文件）、<名>.s（ca65 源码）；回读不一致时仍保留这些文件，并报错说明第一个差异的位置。
// ------------------------------------------------------------------------
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../shared/index.mjs';
import { runTool } from '../toolchain/index.mjs';
import { KIND_UNKNOWN, analyzeCode } from './analysis.mjs';
import { parseCdl } from './cdl.mjs';
import { buildRanges, renderInfo } from './info-writer.mjs';
import { loadBinary, loadInes } from './loaders.mjs';
import { verifyRoundtrip } from './roundtrip.mjs';
import { parseSymbols } from './symbols.mjs';
import { parseTrace } from './trace.mjs';

/**
 * @typedef {object} DisassemblyReport
 * @property {string} sourceFile 生成的 ca65 源码
 * @property {string} infoFile 生成的 da65 信息文件
 * @property {string} imageFile 反汇编的映像
 * @property {string} description 输入的来源说明
 * @property {number} size 映像字节数
 * @property {number} codeBytes 属于指令的字节数
 * @property {number} instructions 指令条数
 * @property {string[]} warnings 追踪中放弃某一路的原因
 * @property {boolean} verified 回读与原映像逐字节一致
 */

/**
 * 判断输入格式。
 * @param {Buffer} buffer 文件内容
 * @param {string|undefined} format 用户指定的格式
 * @returns {'ines'|'binary'} 格式
 * @throws {FcdevError} 指定了不认识的格式
 */
function detectFormat(buffer, format) {
  if (format === 'ines' || format === 'binary') return format;
  if (format !== undefined) throw new FcdevError(`不认识的输入格式：${format}`, { code: 'ARGUMENT_INVALID', hint: '可用的格式：ines、binary' });
  return buffer.subarray(0, 4).toString('latin1') === 'NES\x1a' ? 'ines' : 'binary';
}

/**
 * 反汇编一个文件，并回读校验。
 * @param {{inputFile: string, format?: string, base?: number, entries?: number[], cdlFile?: string, traceFile?: string, symbolsFile?: string, outDir: string, toolchain: import('../toolchain/index.mjs').Toolchain, includes?: string[]}} input 输入文件、格式、加载地址、入口、CDL、模拟器轨迹、符号文件、输出目录、工具链与 da65 要读的符号定义文件
 * @returns {Promise<DisassemblyReport>} 报告
 * @throws {FcdevError} 输入不合法、da65 失败，或回读校验不一致
 */
export async function disassemble({ inputFile, format, base, entries = [], cdlFile, traceFile, symbolsFile, outDir, toolchain, includes = [] }) {
  const buffer = readFileSync(inputFile);
  const kind = detectFormat(buffer, format);
  if (kind === 'binary' && base === undefined) {
    throw new FcdevError('平铺二进制需要指定加载地址', { code: 'ARGUMENT_MISSING', hint: '用 --base <十六进制地址> 指定，如 --base 8000' });
  }
  const image = kind === 'ines' ? loadInes(buffer) : loadBinary(buffer, { base });

  const vectorEntries = image.vectors ? [image.vectors.reset, image.vectors.nmi, image.vectors.irq] : [];
  const cdl = cdlFile ? parseCdl(readFileSync(cdlFile), image.bytes.length) : null;
  const traced = traceFile ? parseTrace(readFileSync(traceFile, 'utf8')) : [];
  const allEntries = [...new Set([...vectorEntries, ...entries, ...traced])];
  if (allEntries.length === 0 && cdl === null) {
    throw new FcdevError('没有入口可供追踪代码', { code: 'ARGUMENT_MISSING', hint: '用 --entry <十六进制地址> 指定入口，或用 --cdl、--trace 提供运行记录' });
  }
  const analysis = analyzeCode({ bytes: image.bytes, base: image.base, entries: allEntries, cdl });

  const symbols = symbolsFile ? parseSymbols(readFileSync(symbolsFile, 'utf8')) : [];
  const vectorLabels = image.vectors
    ? [{ name: 'reset', address: image.vectors.reset }, { name: 'nmi', address: image.vectors.nmi }, { name: 'irq', address: image.vectors.irq }]
    : [];
  const name = path.basename(inputFile, path.extname(inputFile)).replace(/[^A-Za-z0-9_-]/g, '_') || 'image';

  mkdirSync(outDir, { recursive: true });
  const imageFile = path.join(outDir, `${name}.bin`);
  const infoFile = path.join(outDir, `${name}.info`);
  const sourceFile = path.join(outDir, `${name}.s`);
  writeFileSync(imageFile, image.bytes);
  writeFileSync(infoFile, renderInfo({
    inputName: path.basename(imageFile),
    outputName: path.basename(sourceFile),
    base: image.base,
    ranges: buildRanges({ kinds: analysis.kinds, base: image.base, hasVectors: image.vectors !== null }),
    labels: [...symbols, ...vectorLabels],
    includes,
  }));

  const da65 = await runTool(toolchain, 'da65', ['-i', path.basename(infoFile), '--multi-pass'], { cwd: outDir });
  if (da65.code !== 0) throw new FcdevError('da65 反汇编失败', { code: 'DISASM_DA65_FAILED', hint: `${da65.stdout}${da65.stderr}`.trim() });

  const roundtrip = await verifyRoundtrip({ toolchain, sourceFile, expected: image.bytes, base: image.base, workDir: outDir, name });
  if (!roundtrip.ok) {
    const address = image.base + roundtrip.firstDifference;
    throw new FcdevError(`回读校验不一致：偏移 ${roundtrip.firstDifference}（地址 $${address.toString(16).toUpperCase()}）起与原映像不同`, {
      code: 'DISASM_ROUNDTRIP_MISMATCH',
      hint: `源码保留在 ${sourceFile}，可对照 ${imageFile} 检查`,
    });
  }

  const codeBytes = analysis.kinds.reduce((count, value) => count + (value === KIND_UNKNOWN ? 0 : 1), 0);
  return {
    sourceFile,
    infoFile,
    imageFile,
    description: image.description,
    size: image.bytes.length,
    codeBytes,
    instructions: analysis.instructions,
    warnings: analysis.warnings,
    verified: true,
  };
}
