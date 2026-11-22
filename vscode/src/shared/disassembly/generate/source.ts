// ------------------------------------------------------------------------
// 名称：source.ts
// 说明：生成一片的 ca65 源码文字：逐条输出指令、数据、符号、注释；输出的每个字节都与映像一致，不能
//       写成文字的一律退回 .byte。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：规则：- 指令：用符号名代替地址，绝对寻址而地址小于 $100 时加 a:；非官方操作码、jmp ($xxF
//       F)、跨片的分支、目标没有符号的分支写成 .byte 并在行尾写出反汇编文字；- 目标落在指令中间时写
//       “符号加偏移”，符号本身落在指令或地址表项中间时，在那一项前面写 名字 = * + 偏移；- 数据：.
//       byte 每行若干字节，长重复用 .res，字符串用引号，地址表用 .word（RTS 表写 符号 - 1）；- 每个
//       符号与注释都会在所在的地址处输出，数据行在符号与注释的地址处断开；写不进源码的注释记入警
//       告。
// ------------------------------------------------------------------------
import { formatInstruction, isAssemblable } from '../cpu/index';
import { createAddressSpace } from '../analysis/address-space';
import { scanCode } from '../analysis/code-scan';
import { expectSegment } from '../model/index';
import type { Project } from '../model/project';
import type { AddressSpace } from '../analysis/address-space';
import type { CodeScan, ScannedInstruction } from '../analysis/code-scan';
import type { normalizeHint, ProjectSymbol } from '../model/symbols';
import type { Region } from '../model/regions';
import type { Piece } from './pieces';

/** 数据行默认的每行字节数 */
const BYTES_PER_LINE = 8;

/** 地址表默认的每行字数 */
const WORDS_PER_LINE = 4;

/** 字符串每行最多的字节数 */
const STRING_CHUNK = 32;

/** 重复多少个相同的字节才写成 .res（填充区域更低） */
const MIN_REPEAT = 16;
const MIN_REPEAT_PADDING = 4;

/**
 * 十六进制写出字节。
 * @param value 字节
 * @returns 如 $0A
 */
function hex2(value: number): string {
  return `$${value.toString(16).toUpperCase().padStart(2, '0')}`;
}

/**
 * 十六进制写出地址。
 * @param value 地址
 * @returns 如 $C000
 */
function hex4(value: number): string {
  return `$${value.toString(16).toUpperCase().padStart(4, '0')}`;
}

export interface SourceContext {
  /** 工程 */
  project: Project;
  /** 映像的全部字节 */
  bytes: Uint8Array;
  /** 地址空间 */
  space: AddressSpace;
  /** 指令扫描结果 */
  scan: CodeScan;
  /** 布局内的符号：文件偏移到符号 */
  symbolsAt: Map<number, ProjectSymbol>;
  /** 布局外的符号：地址到符号 */
  externalByAddress: Map<number, ProjectSymbol>;
  /** 符号集里的名字：数值到名字 */
  externalNames: ReadonlyMap<number, string>;
  /** 名字到符号 */
  symbolByName: Map<string, ProjectSymbol>;
  /** 注释：文件偏移到注释 */
  commentsAt: Map<number, {block: string[], inline: string|null}>;
  /** 操作数提示：指令的文件偏移到提示 */
  hintsAt: Map<number, ReturnType<typeof normalizeHint>>;
  /** 排序的断点：符号、注释、区域边界的文件偏移，数据行只能在断点处断开 */
  breaks: number[];
  /** 警告 */
  warnings: Array<{code: string, segmentId: number, address: number, message: string}>;
}

/**
 * 建立生成源码要用的各种索引。
 * @param project 工程
 * @param bytes 映像的全部字节
 * @param externalNames 符号集里的名字（数值到名字）
 * @returns 上下文
 */
export function createSourceContext(project: Project, bytes: Uint8Array, externalNames: ReadonlyMap<number, string>): SourceContext {
  const symbolsAt = new Map<number, ProjectSymbol>();
  const externalByAddress = new Map<number, ProjectSymbol>();
  const symbolByName = new Map<string, ProjectSymbol>();
  const breakSet = new Set<number>();
  for (const symbol of project.symbols) {
    symbolByName.set(symbol.name, symbol);
    if (symbol.segmentId === null) {
      externalByAddress.set(symbol.address, symbol);
      continue;
    }
    const segment = expectSegment(project.layout, symbol.segmentId);
    const offset = segment.fileOffset + (symbol.address - segment.address);
    symbolsAt.set(offset, symbol);
    breakSet.add(offset);
  }
  const commentsAt = new Map<number, {block: string[], inline: string|null}>();
  for (const comment of project.comments) {
    const segment = expectSegment(project.layout, comment.segmentId);
    const offset = segment.fileOffset + (comment.address - segment.address);
    const entry = commentsAt.get(offset) ?? { block: [], inline: null };
    if (comment.kind === 'block') entry.block.push(comment.text);
    else entry.inline = comment.text;
    commentsAt.set(offset, entry);
    breakSet.add(offset);
  }
  const hintsAt = new Map<number, ReturnType<typeof normalizeHint>>();
  for (const hint of project.hints) {
    const segment = expectSegment(project.layout, hint.segmentId);
    hintsAt.set(segment.fileOffset + (hint.address - segment.address), hint);
  }
  for (const region of project.regions.regions) {
    breakSet.add(region.offset);
    breakSet.add(region.offset + region.length);
  }
  return {
    project,
    bytes,
    space: createAddressSpace(project.layout),
    scan: scanCode(project, bytes),
    symbolsAt,
    externalByAddress,
    externalNames,
    symbolByName,
    commentsAt,
    hintsAt,
    breaks: [...breakSet].toSorted((a, b) => a - b),
    warnings: [],
  };
}

/**
 * 下一个比 offset 大的断点。
 * @param context 上下文
 * @param offset 文件偏移
 * @param limit 没有断点时返回的上限
 * @returns 断点，不超过 limit
 */
function nextBreak(context: SourceContext, offset: number, limit: number): number {
  const { breaks } = context;
  let low = 0;
  let high = breaks.length;
  while (low < high) {
    const middle = (low + high) >> 1;
    if (breaks[middle]! <= offset) low = middle + 1;
    else high = middle;
  }
  return low < breaks.length ? Math.min(breaks[low]!, limit) : limit;
}

/**
 * 取包含某个偏移的区域，以及这个偏移之后第一个“区域边界”。
 * @param context 上下文
 * @param offset 文件偏移
 * @returns 区域；没有区域时 end 是下一个区域的开头，没有下一个区域时为 null
 */
function regionInfo(context: SourceContext, offset: number): {region: Region|null, end: number|null} {
  const { regions } = context.project.regions;
  let low = 0;
  let high = regions.length - 1;
  while (low <= high) {
    const middle = (low + high) >> 1;
    const region = regions[middle]!;
    if (offset < region.offset) high = middle - 1;
    else if (offset >= region.offset + region.length) low = middle + 1;
    else return { region, end: region.offset + region.length };
  }
  return { region: null, end: low < regions.length ? regions[low]!.offset : null };
}

/**
 * 记一条警告。
 * @param context 上下文
 * @param code 警告码
 * @param segmentId 段号
 * @param address CPU 地址
 * @param message 说明
 */
function warn(context: SourceContext, code: string, segmentId: number, address: number, message: string) {
  context.warnings.push({ code, segmentId, address, message: `${hex4(address)}：${message}` });
}

/**
 * 把 CPU 地址写成符号表达式。
 * @param context 上下文
 * @param address 被引用的 CPU 地址
 * @param fromSegmentId 引用处所在的段
 * @returns 表达式、被引用字节的文件偏移、符号所在的文件偏移；没有符号可用时为 null
 */
function operandRef(context: SourceContext, address: number, fromSegmentId: number): {text: string, offset: number|null, labelOffset: number|null}|null {
  const resolved = context.space.resolve(address, fromSegmentId);
  if (resolved.status === 'ambiguous') return null;
  if (resolved.status === 'unmapped') {
    const name = context.externalByAddress.get(address)?.name ?? context.externalNames.get(address);
    return name === undefined ? null : { text: name, offset: null, labelOffset: null };
  }
  const { segment } = resolved;
  const offset = segment.fileOffset + (address - segment.address);
  const direct = context.symbolsAt.get(offset);
  if (direct !== undefined) return { text: direct.name, offset, labelOffset: offset };
  const inside = context.scan.containing(offset);
  const start = inside === null ? undefined : context.symbolsAt.get(inside.offset);
  if (inside !== null && start !== undefined) return { text: `${start.name}+${offset - inside.offset}`, offset, labelOffset: inside.offset };
  return null;
}

/**
 * 按操作数提示写出操作数表达式。
 * @param context 上下文
 * @param item 指令
 * @param hint 提示
 * @returns 表达式；提示与字节对不上、不适用时为 undefined，调用者退回默认写法并留下警告
 */
function hintExpression(context: SourceContext, item: ScannedInstruction, hint: ReturnType<typeof normalizeHint>): string|undefined {
  const { instruction } = item;
  const operand = instruction.operand;
  if (operand === null || instruction.mode === 'relative') return undefined;
  const mismatch = (reason: string) => {
    warn(context, 'hint-ignored', item.segmentId, item.address, `操作数提示（${hint.form}）没有采用：${reason}`);
    return undefined;
  };
  if (hint.form === 'decimal') return String(operand);
  if (hint.form === 'binary') return `%${operand.toString(2).padStart(instruction.size === 3 ? 16 : 8, '0')}`;
  if (hint.form === 'char') {
    return operand >= 0x20 && operand <= 0x7e && operand !== 0x27 ? `'${String.fromCharCode(operand)}'` : mismatch('操作数不是可写成字符的值');
  }
  const symbol = hint.symbol === null ? undefined : context.symbolByName.get(hint.symbol);
  if (symbol === undefined) return mismatch(`符号 ${String(hint.symbol)} 不存在`);
  const value = symbol.address + hint.offset;
  const text = hint.offset === 0 ? symbol.name : `${symbol.name}${hint.offset > 0 ? '+' : '-'}${Math.abs(hint.offset)}`;
  if (hint.form === 'symbol') return value === operand ? text : mismatch(`${text} 的值是 ${hex4(value)}，与指令里的 ${hex4(operand)} 不一致`);
  if (instruction.mode !== 'immediate') return mismatch('低字节、高字节只用于立即数');
  const part = hint.form === 'low' ? value & 0xff : (value >> 8) & 0xff;
  if (part !== operand) return mismatch(`${text} 的${hint.form === 'low' ? '低' : '高'}字节是 ${hex2(part)}，与指令里的 ${hex2(operand)} 不一致`);
  const wrapped = hint.offset === 0 ? symbol.name : `(${text})`;
  return `${hint.form === 'low' ? '<' : '>'}${wrapped}`;
}

/**
 * 写出一条指令的文字；不能写成文字时返回 null。
 * @param context 上下文
 * @param piece 所在的片
 * @param item 指令
 * @returns 指令文字；null 表示要退回 .byte
 */
function instructionText(context: SourceContext, piece: Piece, item: ScannedInstruction): string|null {
  const { instruction } = item;
  if (!isAssemblable(instruction)) return null;
  let expression: string | undefined;
  const hint = context.hintsAt.get(item.offset);
  if (hint !== undefined) expression = hintExpression(context, item, hint);
  if (expression === undefined) {
    if (instruction.mode === 'relative') {
      const ref = operandRef(context, instruction.target!, item.segmentId);
      const inPiece = (offset: number | null) => offset !== null && offset >= piece.start && offset < piece.end;
      if (ref === null || !inPiece(ref.offset) || !inPiece(ref.labelOffset)) {
        warn(context, 'branch-bytes', item.segmentId, item.address, `分支目标 ${hex4(instruction.target!)} 没有符号或不在同一片里，写成 .byte`);
        return null;
      }
      expression = ref.text;
    } else if (instruction.reference !== null) {
      expression = operandRef(context, instruction.reference, item.segmentId)?.text;
    }
  }
  return formatInstruction(instruction, expression === undefined ? {} : { expression });
}

/**
 * 找出 start 起连续相同的字节数，不超过 end。
 * @param bytes 字节
 * @param start 起点
 * @param end 终点（不含）
 * @returns 相同的字节数
 */
function repeatLength(bytes: Uint8Array, start: number, end: number): number {
  let index = start + 1;
  while (index < end && bytes[index] === bytes[start]) index += 1;
  return index - start;
}

/**
 * 写出一段字节：开头有长重复时写 .res，否则写一行 .byte；调用者循环调用直到写完。
 * @param context 上下文
 * @param offset 起点
 * @param limit 这一项不能超过的终点（不含）
 * @param perLine 每行字节数
 * @param minRepeat 重复多少个字节才写 .res
 * @returns 项
 */
function bytesItem(context: SourceContext, offset: number, limit: number, perLine: number, minRepeat: number): {length: number, lines: string[]} {
  const { bytes } = context;
  const end = nextBreak(context, offset, limit);
  const run = repeatLength(bytes, offset, end);
  if (run >= minRepeat) return { length: run, lines: [`  .res ${run}, ${hex2(bytes[offset] ?? 0)}`] };
  let stop = Math.min(end, offset + perLine);
  for (let index = offset + 1; index < stop; index += 1) {
    if (repeatLength(bytes, index, end) >= minRepeat) {
      stop = index;
      break;
    }
  }
  const values: string[] = [];
  for (let index = offset; index < stop; index += 1) values.push(hex2(bytes[index] ?? 0));
  return { length: stop - offset, lines: [`  .byte ${values.join(', ')}`] };
}

/**
 * 写出字符串区域的一行：可见 ASCII 用引号，其余与引号本身写成数值，最高位置位的结尾字符写成 'X' |
 * $80。
 * @param context 上下文
 * @param offset 起点
 * @param limit 区域终点（不含）
 * @param region 字符串区域
 * @returns 项
 */
function stringItem(context: SourceContext, offset: number, limit: number, region: Region): {length: number, lines: string[]} {
  const { bytes } = context;
  const end = Math.min(nextBreak(context, offset, limit), offset + STRING_CHUNK);
  const tokens: string[] = [];
  let text = '';
  const flush = () => {
    if (text !== '') tokens.push(`"${text}"`);
    text = '';
  };
  const regionEnd = region.offset + region.length;
  for (let index = offset; index < end; index += 1) {
    const value = bytes[index] ?? 0;
    const printable = value >= 0x20 && value <= 0x7e && value !== 0x22;
    const highBit = region.params.terminator === 'high-bit' && index === regionEnd - 1 && value >= 0xa0 && value <= 0xfe && value !== 0xa7;
    if (highBit) {
      flush();
      tokens.push(`'${String.fromCharCode(value & 0x7f)}' | $80`);
    } else if (printable) {
      text += String.fromCharCode(value);
    } else {
      flush();
      tokens.push(hex2(value));
    }
  }
  flush();
  return { length: end - offset, lines: [`  .byte ${tokens.join(', ')}`] };
}

/**
 * 写出地址表区域的一行 .word。
 * @param context 上下文
 * @param offset 起点
 * @param limit 这一项不能超过的终点（不含）
 * @param region 地址表区域
 * @param segmentId 所在的段
 * @returns 项
 */
function wordsItem(context: SourceContext, offset: number, limit: number, region: Region, segmentId: number): {length: number, lines: string[]} {
  const { bytes } = context;
  const adjust = (region.params.adjust as number | undefined) ?? 0;
  const perLine = (region.params.perLine as number | undefined) ?? WORDS_PER_LINE;
  const expressions: string[] = [];
  let position = offset;
  while (position + 1 < limit && expressions.length < perLine) {
    if (expressions.length > 0 && (nextBreak(context, position - 1, limit) < position + 2 || context.symbolsAt.has(position + 1) || context.commentsAt.has(position + 1))) break;
    const value = (bytes[position] ?? 0) | ((bytes[position + 1] ?? 0) << 8);
    const target = value + adjust;
    const ref = target <= 0xffff ? operandRef(context, target, segmentId) : null;
    expressions.push(ref === null ? hex4(value) : adjust === 1 ? `${ref.text} - 1` : ref.text);
    position += 2;
  }
  return { length: position - offset, lines: [`  .word ${expressions.join(', ')}`] };
}

/**
 * 决定 offset 处输出什么：一条指令，或一行数据。
 * @param context 上下文
 * @param piece 所在的片
 * @param offset 文件偏移
 * @returns 项
 */
function nextItem(context: SourceContext, piece: Piece, offset: number): {length: number, lines: string[]} {
  const scanned = context.scan.byOffset.get(offset);
  if (scanned !== undefined && offset + scanned.size <= piece.end) {
    const text = instructionText(context, piece, scanned);
    if (text !== null) return { length: scanned.size, lines: [`  ${text}`] };
    const fallback = bytesItem(context, offset, offset + scanned.size, BYTES_PER_LINE, Infinity);
    fallback.lines[0] = `${fallback.lines[0] ?? ''}  ; ${formatInstruction(scanned.instruction)}`;
    return fallback;
  }
  const { region, end } = regionInfo(context, offset);
  const limit = Math.min(piece.end, end ?? piece.end);
  const segmentId = piece.segmentId;
  if (region === null) return bytesItem(context, offset, limit, BYTES_PER_LINE * 2, MIN_REPEAT);
  if (region.type === 'words' && offset + 2 <= limit) return wordsItem(context, offset, limit, region, segmentId);
  if (region.type === 'string') return stringItem(context, offset, limit, region);
  if (region.type === 'padding') return bytesItem(context, offset, limit, (region.params.perLine as number | undefined) ?? BYTES_PER_LINE * 2, MIN_REPEAT_PADDING);
  return bytesItem(context, offset, limit, (region.params.perLine as number | undefined) ?? BYTES_PER_LINE, MIN_REPEAT);
}

/**
 * 生成一片的源码行（不含 .segment 行）。
 * @param context 上下文
 * @param piece 片
 * @returns 源码行
 */
export function emitPiece(context: SourceContext, piece: Piece): string[] {
  const lines: string[] = [];
  const segment = expectSegment(context.project.layout, piece.segmentId);
  const addressOf = (offset: number) => segment.address + (offset - segment.fileOffset);
  let offset = piece.start;
  while (offset < piece.end) {
    const item = nextItem(context, piece, offset);
    const comments = context.commentsAt.get(offset);
    const symbol = context.symbolsAt.get(offset);
    if ((symbol?.kind === 'procedure' || (comments?.block.length ?? 0) > 0) && lines.length > 0 && lines.at(-1) !== '') lines.push('');
    for (const block of comments?.block ?? []) for (const text of block.split('\n')) lines.push(`; ${text}`);
    if (symbol !== undefined) {
      if (symbol.description !== '') lines.push(`; ${symbol.description.replace(/\n/g, ' ')}`);
      lines.push(`${symbol.name}:`);
    }
    for (let inner = 1; inner < item.length; inner += 1) {
      const innerSymbol = context.symbolsAt.get(offset + inner);
      if (innerSymbol !== undefined) lines.push(`${innerSymbol.name} = * + ${inner}`);
      if (context.commentsAt.has(offset + inner)) warn(context, 'comment-inside', piece.segmentId, addressOf(offset + inner), '注释所在的地址在一条指令或一项数据的中间，没有写进源码');
    }
    if (comments?.inline) item.lines[0] = `${item.lines[0] ?? ''}  ; ${comments.inline}`;
    lines.push(...item.lines);
    offset += item.length;
  }
  return lines;
}
