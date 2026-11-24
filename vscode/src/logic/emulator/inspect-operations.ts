// ------------------------------------------------------------------------
// 名称：inspect-operations.ts
// 说明：检查与交互类操作：inspect（状态、内存、反汇编、地址解析）、screenshot（截图）、input（按
//       键、手柄、鼠标、键盘接入）、record（指令跟踪、CDL、调用记录）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：内存读写与寄存器修改只在这里；跟踪与 CDL 的文件由 Ember 写在它的调试输出目录，结果里的 file
//       是完整路径，交给反汇编的 importRecord 导入。手柄按键掩码的位序是 A、B、选择、开始、上、下、
//       左、右（第 0 位到第 7 位）。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { InputMouseParams, InspectorStateField } from '../../contracts/debug-client/generated/api';
import type { OperationContext } from './operation-context';
import { invalidParam, optionalAddress, optionalArray, optionalBoolean, optionalEnum, optionalInteger, optionalString, requireString, toAddress } from './params';
import { cpuState } from './program-operations';
import { runFrames, tapKey } from './sb2000-session';

/** 一次最多读出的内存字节数。 */
const MAX_READ = 4096;

/** 每行显示的字节数。 */
const ROW_BYTES = 16;

/** 调色板每行显示的颜色数。 */
const PALETTE_ROW_COLORS = 8;

/** 手柄按键名对应的掩码位。 */
const PAD_BITS: Readonly<Record<string, number>> = { a: 1, b: 2, select: 4, start: 8, up: 16, down: 32, left: 64, right: 128 };

/**
 * 十六进制写出数值。
 * @param value 数值。
 * @param digits 位数。
 * @returns 大写十六进制。
 */
function hex(value: number, digits: number): string {
  return value.toString(16).toUpperCase().padStart(digits, '0');
}

/**
 * 把字节写成每行 16 个的十六进制与字符视图。
 * @param data 字节。
 * @param address 第一个字节的地址。
 * @returns 多行文字。
 */
function hexDump(data: Uint8Array, address: number): string {
  const lines: string[] = [];
  for (let offset = 0; offset < data.length; offset += ROW_BYTES) {
    const row = [...data.subarray(offset, offset + ROW_BYTES)];
    const cells = row.map((value, column) => `${column > 0 && column % 8 === 0 ? ' ' : ''}${hex(value, 2)}`).join(' ');
    const text = row.map((value) => (value >= 0x20 && value <= 0x7e ? String.fromCodePoint(value) : '.')).join('');
    lines.push(`${hex(address + offset, 4)}  ${cells.padEnd(ROW_BYTES * 3)} |${text}|`);
  }
  return lines.join('\n');
}

/**
 * 读 32 位以内的地址参数（内存空间不止 64K）。
 * @param value 整数，或十六进制文字。
 * @param name 参数名。
 * @returns 地址。
 */
function wideAddress(value: unknown, name: string): number {
  let number = Number.NaN;
  if (typeof value === 'number') number = value;
  else if (typeof value === 'string') {
    const match = /^(?:\$|0x)?([0-9a-f]{1,8})$/i.exec(value.trim());
    if (match?.[1] !== undefined) number = Number.parseInt(match[1], 16);
  }
  if (!Number.isInteger(number) || number < 0 || number > 0xffffffff) invalidParam(`${name} 必须是地址（整数或十六进制文字）`);
  return number;
}

/**
 * 把调色板（每项 4 个字节 R、G、B、A）写成每行 8 个颜色的文字：“编号=#RRGGBB”。
 * @param data 调色板字节。
 * @returns 多行文字。
 */
function paletteText(data: Uint8Array): string {
  const cells: string[] = [];
  for (let index = 0; index + 3 < data.length; index += 4) {
    cells.push(`${hex(index / 4, 2)}=#${hex(data[index] ?? 0, 2)}${hex(data[index + 1] ?? 0, 2)}${hex(data[index + 2] ?? 0, 2)}`);
  }
  const rows: string[] = [];
  for (let offset = 0; offset < cells.length; offset += PALETTE_ROW_COLORS) rows.push(cells.slice(offset, offset + PALETTE_ROW_COLORS).join(' '));
  return rows.join('\n');
}

/**
 * 检查：状态、内存读写、反汇编、地址解析、内存空间列表、修改 CPU 状态、调用栈、当前调色板。
 * @param context 上下文。
 * @param params what：state、memory、write、disassemble、resolve、spaces、setState、callStack、palette；其余参数见各
 *   项。
 * @returns 结果。
 */
export async function inspect(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { client } = context.store.currentSession();
  const what = optionalEnum(params, 'what', ['state', 'memory', 'write', 'disassemble', 'resolve', 'spaces', 'setState', 'callStack', 'palette']);
  if (what === undefined) invalidParam('要给 what：state、memory、write、disassemble、resolve、spaces、setState、callStack、palette');
  const space = optionalString(params, 'space') ?? 'cpu';

  switch (what) {
    case 'state': {
      const module = optionalString(params, 'module');
      return { ...(await client.request('inspector.getState', module === undefined ? {} : { module })) };
    }
    case 'memory': {
      const address = wideAddress(params.address, 'address');
      const length = optionalInteger(params, 'length', { min: 1, max: MAX_READ }) ?? 64;
      const result = await client.request('inspector.readMemory', { space, address, length });
      return { space, address, length: result.length, text: hexDump(result.data ?? new Uint8Array(), address) };
    }
    case 'write': {
      const bytes = optionalArray(params, 'bytes');
      if (bytes === undefined || bytes.length === 0) invalidParam('写内存要给 bytes（字节数组）');
      if (!bytes.every((value): value is number => Number.isInteger(value) && (value as number) >= 0 && (value as number) <= 255)) {
        invalidParam('bytes 里每项必须是 0 到 255 的整数');
      }
      return { ...(await client.request('inspector.writeMemory', { space, address: wideAddress(params.address, 'address'), bytes })) };
    }
    case 'disassemble': {
      const address = toAddress(params.address, 'address');
      const { instructions } = await client.request('inspector.disassemble', { address, count: optionalInteger(params, 'count', { min: 1, max: 256 }) ?? 16 });
      return {
        instructions,
        text: instructions.map((item) => `${hex(item.address, 4)}  ${item.bytes.padEnd(8)}  ${item.text}${item.unofficial ? '  ; 非官方指令' : ''}`).join('\n')
      };
    }
    case 'resolve': {
      const bus = optionalEnum(params, 'space', ['cpu', 'ppu'], 'cpu') as 'cpu' | 'ppu';
      return { ...(await client.request('inspector.resolve', { space: bus, address: toAddress(params.address, 'address') })) };
    }
    case 'spaces':
      return { ...(await client.request('inspector.listSpaces', {})) };
    case 'callStack':
      return { ...(await client.request('inspector.callStack', {})) };
    case 'palette': {
      const result = await client.request('inspector.getPalette', {});
      return { count: result.count, text: paletteText(result.data ?? new Uint8Array()) };
    }
    default: {
      const fields = optionalArray(params, 'fields');
      if (fields === undefined || fields.length === 0) invalidParam('修改状态要给 fields：[{ name, value }]');
      return { ...(await client.request('inspector.setState', { module: optionalString(params, 'module') ?? 'cpu', fields: fields as InspectorStateField[] })) };
    }
  }
}

/**
 * 截取最近一帧画面。
 * @param context 上下文。
 * @param params 可选 file（把 PNG 存到这个路径）、inline（返回 base64，默认 true）。
 * @returns 尺寸、字节数、文件路径与 base64 图像。
 */
export async function screenshot(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { client } = context.store.currentSession();
  const result = await client.request('output.screenshot', { format: 'png' });
  const data = result.data ?? new Uint8Array();
  const file = optionalString(params, 'file');
  let saved: string | undefined;
  if (file !== undefined) {
    saved = path.resolve(file);
    context.env.files.ensureDirectory(path.dirname(saved));
    context.env.files.writeBytes(saved, data);
  }
  return {
    format: result.format,
    width: result.width,
    height: result.height,
    byteLength: result.byteLength,
    ...(saved === undefined ? {} : { file: saved }),
    ...(optionalBoolean(params, 'inline', true) ? { imageBase64: Buffer.from(data).toString('base64') } : {})
  };
}

/**
 * 手柄按键名换成掩码。
 * @param value 掩码整数，或按键名数组（a、b、select、start、up、down、left、right）。
 * @returns 掩码。
 */
function padMask(value: unknown): number {
  if (typeof value === 'number' && Number.isInteger(value) && value >= 0 && value <= 255) return value;
  if (Array.isArray(value)) {
    let mask = 0;
    for (const name of value) {
      const bit = typeof name === 'string' ? PAD_BITS[name.toLowerCase()] : undefined;
      if (bit === undefined) invalidParam(`未知的手柄按键：${String(name)}`, `可用的按键：${Object.keys(PAD_BITS).join('、')}`);
      mask |= bit;
    }
    return mask;
  }
  return invalidParam('buttons 必须是 0 到 255 的掩码，或按键名数组');
}

/**
 * 输入：按键、手柄、鼠标、Family BASIC 键盘的接入，以及全部松开。
 * @param context 上下文。
 * @param params kind：key、pad、mouse、familyKeyboard、releaseAll；其余参数见各项。
 * @returns 结果；按键与手柄会运行帧数让机器读到。
 */
export async function input(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { client } = context.store.currentSession();
  const kind = optionalEnum(params, 'kind', ['key', 'pad', 'mouse', 'familyKeyboard', 'releaseAll']);
  if (kind === undefined) invalidParam('要给 kind：key、pad、mouse、familyKeyboard、releaseAll');
  const downFrames = optionalInteger(params, 'downFrames', { min: 1, max: 600 }) ?? 8;
  const upFrames = optionalInteger(params, 'upFrames', { min: 1, max: 600 }) ?? 4;

  switch (kind) {
    case 'key': {
      const code = requireString(params, 'code');
      const action = optionalEnum(params, 'action', ['tap', 'down', 'up'], 'tap');
      if (action === 'tap') await tapKey(client, code, downFrames, upFrames);
      else await client.request('input.key', { code, down: action === 'down' });
      return { kind, code, action, cpu: await cpuState(client) };
    }
    case 'pad': {
      const port = optionalInteger(params, 'port', { min: 0, max: 7 }) ?? 0;
      const buttons = padMask(params.buttons);
      await client.request('input.pad', { port, buttons });
      if (optionalBoolean(params, 'hold', false)) return { kind, port, buttons, held: true };
      await runFrames(client, downFrames);
      await client.request('input.pad', { port, buttons: 0 });
      await runFrames(client, upFrames);
      return { kind, port, buttons, held: false, cpu: await cpuState(client) };
    }
    case 'mouse': {
      const event = optionalEnum(params, 'event', ['move', 'down', 'up', 'wheel', 'leave']) as InputMouseParams['kind'] | undefined;
      if (event === undefined) invalidParam('鼠标要给 event：move、down、up、wheel、leave');
      const fields: Record<string, number> = {};
      for (const name of ['x', 'y', 'dx', 'dy', 'wheel']) {
        const value = params[name];
        if (typeof value === 'number') fields[name] = value;
      }
      const buttons = optionalInteger(params, 'buttons', { min: 0, max: 7 });
      await client.request('input.mouse', { kind: event, ...fields, ...(buttons === undefined ? {} : { buttons }) });
      return { kind, event };
    }
    case 'familyKeyboard':
      return { ...(await client.request('input.setFamilyKeyboard', { connected: optionalBoolean(params, 'connected', true) })) };
    default:
      await client.request('input.releaseAll', {});
      return { kind };
  }
}

/**
 * 记录：start 开始、read 边记边读、stop 结束并取得文件。
 * @param context 上下文。
 * @param params action（start、read、stop）、kind（instruction、io、irq、cdl、callLog）；callLog 开始时要
 *   address；跟踪开始时可给 ring（环形，满了丢弃最早的行）；read 可给 from、maxLines；
 *   read 与 stop 要 id（只有一个时可省略）。
 * @returns 开始时的编号，读到的行，或结束时的文件路径与行数。
 */
export async function record(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const session = context.store.currentSession();
  const { client } = session;
  const action = optionalEnum(params, 'action', ['start', 'read', 'stop']);
  const kind = optionalEnum(params, 'kind', ['instruction', 'io', 'irq', 'cdl', 'callLog']);
  if (action === undefined || kind === undefined) invalidParam('要给 action（start、read、stop）与 kind（instruction、io、irq、cdl、callLog）');
  const file = optionalString(params, 'file');
  const maxLines = optionalInteger(params, 'maxLines', { min: 1, max: 1_000_000 });
  const extra = { ...(file === undefined ? {} : { file }), ...(maxLines === undefined ? {} : { maxLines }) };

  if (kind === 'cdl') {
    if (action === 'read') invalidParam('cdl 没有文字行可读，结束时取得文件');
    if (action === 'start') {
      const { size } = await client.request('inspector.cdlStart', {});
      session.flags.cdl = true;
      return { kind, started: true, size };
    }
    const stopped = await client.request('inspector.cdlStop', {});
    const saved = await client.request('inspector.cdlSave', file === undefined ? {} : { file });
    session.flags.cdl = false;
    return { kind, stopped: true, file: saved.file, size: saved.size, codeBytes: stopped.codeBytes, dataBytes: stopped.dataBytes };
  }

  if (action === 'start') {
    if (kind === 'callLog') {
      const address = optionalAddress(params, 'address');
      if (address === undefined) invalidParam('调用记录要给 address（要记录的指令地址）');
      const stackBytes = optionalInteger(params, 'stackBytes', { min: 0, max: 32 });
      const { id } = await client.request('inspector.callLogStart', {
        address,
        ...(stackBytes === undefined ? {} : { stackBytes }),
        ...(params.ranges === undefined ? {} : { ranges: params.ranges as never }),
        ...(params.skip === undefined ? {} : { skip: params.skip as never }),
        ...extra
      });
      session.records.set(id, { kind });
      return { kind, id, started: true };
    }
    const unique = optionalBoolean(params, 'unique', false);
    const ring = optionalBoolean(params, 'ring', false);
    const { id } = await client.request('inspector.traceStart', {
      kind: kind as 'instruction' | 'io' | 'irq',
      ...(kind === 'instruction' && unique ? { unique } : {}),
      ...(ring ? { ring } : {}),
      ...extra
    });
    session.records.set(id, { kind });
    return { kind, id, started: true };
  }

  let id = optionalInteger(params, 'id', { min: 1 });
  if (id === undefined) {
    const candidates = [...session.records].filter(([, entry]) => entry.kind === kind).map(([key]) => key);
    if (candidates.length !== 1 || candidates[0] === undefined) invalidParam(`要给 id（正在进行的 ${kind} 记录有 ${candidates.length} 个）`);
    id = candidates[0];
  }
  if (action === 'read') {
    const from = optionalInteger(params, 'from', { min: 0 });
    const lines = optionalInteger(params, 'maxLines', { min: 1, max: 1_000_000 });
    const result = await client.request('inspector.traceRead', { id, ...(from === undefined ? {} : { from }), ...(lines === undefined ? {} : { maxLines: lines }) });
    return { kind, id, ...result };
  }
  const stopped = await client.request(kind === 'callLog' ? 'inspector.callLogStop' : 'inspector.traceStop', { id });
  session.records.delete(id);
  return { kind, id, stopped: true, file: stopped.file, lines: stopped.lines, truncated: stopped.truncated };
}
