// ------------------------------------------------------------------------
// 名称：record-operations.ts
// 说明：运行记录导入：importRecord 把 Ember 记录的指令轨迹、调用记录或 CDL 导入工程——执行过的地址
//       成为入口，CDL 里只被当数据读取的字节成为数据区域。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：整批导入，已有的入口不重复添加；记录里不在映像里的地址忽略并汇总；导入后用 trace 重新追踪才
//       会生成代码区域与自动符号。数据区域的来源写 cdl，不盖住已有的手工区域；以后的追踪也不会盖住
//       它们。
// ------------------------------------------------------------------------
import * as path from 'node:path';
import type { DisasmEnvironment } from '../../../contracts/disassembly/disasm-environment';
import type { FileSystem } from '../../../contracts/files/file-system';
import { FcdevError } from '../../../shared/errors/fcdev-error';
import { createAddressSpace, dataRunsFromFlags, entriesFromAddresses, entriesFromCodeFlags, mappedSize } from '../../../shared/disassembly/analysis/index';
import { CDL_CODE, CDL_DATA, CDL_JUMP_TARGET, CDL_SUB_ENTRY, parseCallLog, parseCdl, parseTrace } from '../../../shared/disassembly/formats/index';
import { addressToOffset, applyEdits, listRegions } from '../../../shared/disassembly/model/index';
import type { EditItem } from '../../../shared/disassembly/model/index';
import type { Entry } from '../../../shared/disassembly/model/entries';
import { toBuffer } from '../workspace/project-store';
import { loadContext, saveContext } from './context';
import { invalidParam, optionalEnum, projectDirectory, requireString } from '../../../shared/disassembly/operations/params';
import type { AddressSpace } from '../../../shared/disassembly/analysis/address-space';

/** 汇总里最多列出几个被忽略的地址 */
const MAX_SKIPPED_SHOWN = 5;

/** JSR 指令的操作码 */
const JSR_OPCODE = 0x20;

/** JSR 指令的字节数 */
const JSR_LENGTH = 3;

/** 来源是这些的区域会被追踪重新生成，导入数据区域时可以盖住 */
const REGENERATED_SOURCES = new Set(['trace', 'table']);

/**
 * 读记录文件。
 * @param files 文件系统
 * @param file 文件路径
 * @param binary 是否按二进制读
 * @returns 内容
 * @throws FcdevError 读不到
 */
function readRecord(files: FileSystem, file: string, binary: true): Buffer;
function readRecord(files: FileSystem, file: string, binary: false): string;
function readRecord(files: FileSystem, file: string, binary: boolean): Buffer | string {
  try {
    return binary ? toBuffer(files.readBytes(file)) : files.readText(file);
  } catch (error) {
    throw new FcdevError(`读不到记录文件 ${file}`, { code: 'DISASM_RECORD_MISSING', hint: '记录文件由 Ember 写在它的调试输出目录，路径见 emu_record 停止时返回的 file', cause: error });
  }
}

/**
 * 由调用记录找出已核实的调用者：返回地址减 3 处是 JSR，且操作数就是被记录的地址。
 * @param bytes 映像字节
 * @param space 地址空间
 * @param returns 返回地址对
 * @returns 调用者（JSR 指令）的地址
 */
function verifiedCallers(bytes: Buffer, space: AddressSpace, returns: Array<{target: number, returnAddress: number}>): number[] {
  const callers: number[] = [];
  for (const { target, returnAddress } of returns) {
    const site = (returnAddress - JSR_LENGTH) & 0xffff;
    const resolved = space.resolve(site);
    if (resolved.status !== 'ok') continue;
    const offset = addressToOffset(resolved.segment, site);
    if (offset + JSR_LENGTH > bytes.length) continue;
    if (bytes[offset] === JSR_OPCODE && ((bytes[offset + 1] ?? 0) | ((bytes[offset + 2] ?? 0) << 8)) === target) callers.push(site);
  }
  return callers;
}

/**
 * 汇总没能生成入口的地址。
 * @param skipped 被忽略的地址
 * @returns 个数与前几个的十六进制写法
 */
function summarizeSkipped(skipped: Array<{address: number}>): {count: number, examples: string[]} {
  return { count: skipped.length, examples: skipped.slice(0, MAX_SKIPPED_SHOWN).map((item) => `$${item.address.toString(16).toUpperCase().padStart(4, '0')}`) };
}

/**
 * 导入运行记录。
 * @param environment 运行环境
 * @param params projectDir、kind（trace 指令轨迹、callLog 调用记录、cdl 代码数据日志）、file（记录
 *   文件的路径）
 * @returns 记录种类、新增与已有的入口数、新增的数据区域数、被忽略的地址与下一步提示
 * @throws FcdevError 参数不合法、记录读不到，或记录与映像对不上
 */
export function importRecord({ files }: DisasmEnvironment, params: Record<string, unknown>): Record<string, unknown> {
  const context = loadContext(files, projectDirectory(params));
  const kind = optionalEnum(params, 'kind', ['trace', 'callLog', 'cdl']);
  if (kind === undefined) invalidParam('要给 kind：trace（指令轨迹）、callLog（调用记录）、cdl（代码数据日志）');
  const file = path.resolve(requireString(params, 'file'));
  const { project, bytes } = context;
  const space = createAddressSpace(project.layout);

  let found: Array<Readonly<Entry>> = [];
  let skipped: Array<{address: number}> = [];
  let dataRuns: ReturnType<typeof dataRunsFromFlags> = [];
  let callers = 0;
  if (kind === 'cdl') {
    const flags = parseCdl(readRecord(files, file, true), mappedSize(project.layout), CDL_CODE | CDL_DATA | CDL_JUMP_TARGET | CDL_SUB_ENTRY);
    found = entriesFromCodeFlags(project.layout, flags, { source: 'cdl', description: '代码数据日志', startBits: CDL_JUMP_TARGET | CDL_SUB_ENTRY });
    dataRuns = dataRunsFromFlags(project.layout, flags);
  } else if (kind === 'trace') {
    ({ entries: found, skipped } = entriesFromAddresses(space, parseTrace(readRecord(files, file, false)), { source: 'trace', description: '指令轨迹' }));
  } else {
    const parsed = parseCallLog(readRecord(files, file, false));
    const callerAddresses = verifiedCallers(bytes, space, parsed.returns);
    callers = callerAddresses.length;
    ({ entries: found, skipped } = entriesFromAddresses(space, [...parsed.addresses, ...callerAddresses], { source: 'call-record', description: '调用记录' }));
  }

  const known = new Set(project.entries.map((entry) => `${entry.segmentId}:${entry.address}`));
  const fresh = found.filter((entry) => !known.has(`${entry.segmentId}:${entry.address}`));
  const edits = fresh.map((entry): EditItem => ({ op: 'addEntry', address: entry.address, segmentId: entry.segmentId, source: entry.source, description: entry.description }));
  const regionEdits = dataRuns
    .filter((run) => listRegions(project.regions, run.offset, run.length).every((region) => region.type === 'unknown' || REGENERATED_SOURCES.has(region.source)))
    .map((run): EditItem => ({ op: 'setRegion', address: run.address, segmentId: run.segmentId, length: run.length, type: 'bytes', source: 'cdl' }));

  if (found.length === 0 && regionEdits.length === 0) {
    throw new FcdevError(kind === 'cdl' ? 'CDL 里没有被执行过的字节' : '记录里没有能导入的地址', {
      code: 'DISASM_RECORD_EMPTY',
      hint: kind === 'cdl' ? '确认记录期间程序运行过，且 CDL 是用同一个映像记录的' : '确认记录期间程序运行过，且地址在这个映像里',
    });
  }
  const { project: next } = applyEdits(project, [...edits, ...regionEdits]);
  saveContext(files, context, next);
  return {
    kind,
    file,
    entriesAdded: fresh.length,
    entriesExisting: found.length - fresh.length,
    ...(kind === 'callLog' ? { callersVerified: callers } : {}),
    ...(kind === 'cdl' ? { dataRegionsAdded: regionEdits.length, dataRegionsSkipped: dataRuns.length - regionEdits.length } : {}),
    skipped: summarizeSkipped(skipped),
    next: '用 disasm_trace 重新追踪，让新入口生成代码区域与自动符号',
  };
}
