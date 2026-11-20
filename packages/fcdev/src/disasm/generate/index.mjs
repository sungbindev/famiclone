// ------------------------------------------------------------------------
// 名称：index.mjs
// 说明：由反汇编工程生成源码工程：每个模块一个 .s 文件，include 目录里是符号声明、常量与符号集，另有链接配置；返回文件内容，不写盘。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：生成的工程用 ca65 -I include 汇编每个 src/*.s，再用 ld65 -C link.cfg 链接，得到每个段一个 seg<段号>.bin，与映像逐字节一致。
//       没有归入模块的字节属于 main 模块。符号集的内容由调用者读好传入（名称到文本），这里不读文件。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { indexByValue, parseSymbolSet } from '../profile/index.mjs';
import { renderLinkConfig, segmentOutputName } from './link-config.mjs';
import { DEFAULT_MODULE, buildPieces } from './pieces.mjs';
import { createSourceContext, emitPiece } from './source.mjs';

/** 每行 .global 列出的符号个数 */
const GLOBALS_PER_LINE = 6;

/**
 * @typedef {object} GeneratedProject
 * @property {Map<string, string>} files 相对工程目录的路径到文件内容
 * @property {Array<{code: string, segmentId: number, address: number, message: string}>} warnings 警告：没写进源码的注释、没采用的提示、退回 .byte 的分支
 * @property {import('./pieces.mjs').Piece[]} pieces 片
 * @property {Array<{id: number, output: string, fileOffset: number, length: number}>} segments 每个段的输出文件与它对应的映像字节
 * @property {string[]} modules 生成了源文件的模块名
 */

/**
 * 把一组符号名写成 .global 行。
 * @param {string} directive .global 或 .globalzp
 * @param {string[]} names 符号名
 * @returns {string[]} 行
 */
function globalLines(directive, names) {
  const lines = [];
  for (let index = 0; index < names.length; index += GLOBALS_PER_LINE) lines.push(`${directive} ${names.slice(index, index + GLOBALS_PER_LINE).join(', ')}`);
  return lines;
}

/**
 * 生成源码工程。
 * @param {import('../model/project.mjs').Project} project 工程
 * @param {Uint8Array} bytes 映像的全部字节
 * @param {{symbolSets?: ReadonlyMap<string, string>}} [options] 符号集的内容：文件名到文本，工程里登记的每个符号集都要有
 * @returns {GeneratedProject} 生成结果
 * @throws {FcdevError} 映像与工程不符、缺少符号集内容、符号名与符号集里的常量重名
 */
export function generateProject(project, bytes, { symbolSets = new Map() } = {}) {
  if (bytes.length !== project.image.size) {
    throw new FcdevError(`映像是 ${bytes.length} 字节，工程记录的是 ${project.image.size} 字节`, { code: 'DISASM_IMAGE_MISMATCH', hint: '映像文件变了，工程已经过期' });
  }
  const files = new Map();
  const constants = [];
  for (const set of project.symbolSets) {
    const text = symbolSets.get(set.name);
    if (text === undefined) throw new FcdevError(`缺少符号集 ${set.name} 的内容`, { code: 'DISASM_SYMBOL_SET_MISSING', hint: `来源：${set.path}` });
    files.set(`include/${set.name}`, text);
    constants.push(...parseSymbolSet(text));
  }
  const setNames = new Set(constants.map((constant) => constant.name));
  for (const symbol of project.symbols) {
    if (setNames.has(symbol.name)) throw new FcdevError(`符号 ${symbol.name} 与符号集里的常量重名`, { code: 'DISASM_SYMBOL_DUPLICATE' });
  }

  const context = createSourceContext(project, bytes, indexByValue(constants));
  const pieces = buildPieces(project);
  const moduleNames = [...new Set(pieces.map((piece) => piece.module))];
  const order = [DEFAULT_MODULE, ...project.modules.map((module) => module.name)];
  moduleNames.sort((a, b) => order.indexOf(a) - order.indexOf(b));
  for (const name of moduleNames) {
    const description = project.modules.find((module) => module.name === name)?.description ?? (name === DEFAULT_MODULE ? '没有归入其他模块的字节' : '');
    const lines = [`; 模块：${name}`];
    if (description !== '') lines.push(`; ${description.replace(/\n/g, ' ')}`);
    lines.push('', '.setcpu "6502"', '.include "constants.inc"', '.include "globals.inc"');
    for (const piece of pieces.filter((item) => item.module === name)) {
      const segment = project.layout.segments.find((item) => item.id === piece.segmentId);
      const first = segment.address + (piece.start - segment.fileOffset);
      const last = segment.address + (piece.end - segment.fileOffset) - 1;
      lines.push('', `.segment "${piece.name}"`, `; 段 ${segment.id}${segment.bank === null ? '' : `，bank ${segment.bank}`}，$${first.toString(16).toUpperCase().padStart(4, '0')}-$${last.toString(16).toUpperCase().padStart(4, '0')}`);
      lines.push(...emitPiece(context, piece));
    }
    files.set(`src/${name}.s`, `${lines.join('\n')}\n`);
  }

  const inLayout = project.symbols.filter((symbol) => symbol.segmentId !== null).sort((a, b) => a.segmentId - b.segmentId || a.address - b.address);
  const zeroPage = inLayout.filter((symbol) => symbol.address < 0x100).map((symbol) => symbol.name);
  const absolute = inLayout.filter((symbol) => symbol.address >= 0x100).map((symbol) => symbol.name);
  files.set('include/globals.inc', `; 布局内的符号：在定义它的模块里是导出，在其他模块里是导入。\n${[...globalLines('.global', absolute), ...globalLines('.globalzp', zeroPage)].join('\n')}\n`);

  const externals = project.symbols.filter((symbol) => symbol.segmentId === null).sort((a, b) => a.address - b.address);
  const constantLines = ['; 布局外的地址（寄存器、RAM 变量）：符号集与工程里定义的常量。'];
  for (const set of project.symbolSets) constantLines.push(`.include "${set.name}"`);
  for (const symbol of externals) {
    constantLines.push(`${symbol.name} = $${symbol.address.toString(16).toUpperCase().padStart(4, '0')}${symbol.description === '' ? '' : `  ; ${symbol.description.replace(/\n/g, ' ')}`}`);
  }
  files.set('include/constants.inc', `${constantLines.join('\n')}\n`);
  files.set('link.cfg', renderLinkConfig(project, pieces));

  return {
    files,
    warnings: context.warnings,
    pieces,
    segments: project.layout.segments.map((segment) => ({ id: segment.id, output: segmentOutputName(segment.id), fileOffset: segment.fileOffset, length: segment.length })),
    modules: moduleNames,
  };
}
