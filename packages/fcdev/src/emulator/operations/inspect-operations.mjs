// ------------------------------------------------------------------------
// 名称：inspect-operations.mjs
// 说明：检查与交互类操作：inspect（状态、内存、反汇编、地址解析）、screenshot（截图）、input（按键、手柄、鼠标、键盘接入）、record（指令跟踪、CDL、调用记录）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：内存读写与寄存器修改只在这里；跟踪与 CDL 的文件由 Ember 写在它的调试输出目录，结果里的 file 是完整路径，交给 disasm 的 importRecord 导入。
//       手柄按键掩码的位序是 A、B、选择、开始、上、下、左、右（第 0 位到第 7 位）。
// ------------------------------------------------------------------------
import { mkdirSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { runFrames, tapKey } from '../sb2000-session.mjs';
import { cpuState } from './program-operations.mjs';
import { invalidParam, optionalAddress, optionalArray, optionalBoolean, optionalEnum, optionalInteger, optionalString, requireString, toAddress } from './params.mjs';
import { currentSession } from './session.mjs';

/** 一次最多读出的内存字节数 */
const MAX_READ = 4096;

/** 每行显示的字节数 */
const ROW_BYTES = 16;

/** 手柄按键名对应的掩码位 */
const PAD_BITS = { a: 1, b: 2, select: 4, start: 8, up: 16, down: 32, left: 64, right: 128 };

/**
 * 十六进制写出数值。
 * @param {number} value 数值
 * @param {number} digits 位数
 * @returns {string} 大写十六进制
 */
function hex(value, digits) {
  return value.toString(16).toUpperCase().padStart(digits, '0');
}

/**
 * 把字节写成每行 16 个的十六进制与字符视图。
 * @param {Uint8Array} data 字节
 * @param {number} address 第一个字节的地址
 * @returns {string} 多行文字
 */
function hexDump(data, address) {
  const lines = [];
  for (let offset = 0; offset < data.length; offset += ROW_BYTES) {
    const row = [...data.subarray(offset, offset + ROW_BYTES)];
    const cells = row.map((value, column) => `${column > 0 && column % 8 === 0 ? ' ' : ''}${hex(value, 2)}`).join(' ');
    const text = row.map((value) => (value >= 0x20 && value <= 0x7e ? String.fromCharCode(value) : '.')).join('');
    lines.push(`${hex(address + offset, 4)}  ${cells.padEnd(ROW_BYTES * 3)} |${text}|`);
  }
  return lines.join('\n');
}

/**
 * 读 32 位以内的地址参数（内存空间不止 64K）。
 * @param {unknown} value 整数，或十六进制文字
 * @param {string} name 参数名
 * @returns {number} 地址
 */
function wideAddress(value, name) {
  let number = Number.NaN;
  if (typeof value === 'number') number = value;
  else if (typeof value === 'string') {
    const match = /^(?:\$|0[xX])?([0-9A-Fa-f]{1,8})$/.exec(value.trim());
    if (match !== null) number = Number.parseInt(match[1], 16);
  }
  if (!Number.isInteger(number) || number < 0 || number > 0xffffffff) invalidParam(`${name} 必须是地址（整数或十六进制文字）`);
  return number;
}

/**
 * 检查：状态、内存读写、反汇编、地址解析、内存空间列表、修改 CPU 状态。
 * @param {Record<string, unknown>} params what：state、memory、write、disassemble、resolve、spaces、setState；其余参数见各项
 * @returns {Promise<object>} 结果
 */
export async function inspect(params) {
  const { client } = currentSession();
  const what = optionalEnum(params, 'what', ['state', 'memory', 'write', 'disassemble', 'resolve', 'spaces', 'setState']);
  if (what === undefined) invalidParam('要给 what：state、memory、write、disassemble、resolve、spaces、setState');
  const space = optionalString(params, 'space') ?? 'cpu';

  switch (what) {
    case 'state': {
      const module = optionalString(params, 'module');
      return client.request('inspector.getState', module === undefined ? {} : { module });
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
      if (!bytes.every((value) => Number.isInteger(value) && value >= 0 && value <= 255)) invalidParam('bytes 里每项必须是 0 到 255 的整数');
      return client.request('inspector.writeMemory', { space, address: wideAddress(params.address, 'address'), bytes });
    }
    case 'disassemble': {
      const address = toAddress(params.address, 'address');
      const { instructions } = await client.request('inspector.disassemble', { address, count: optionalInteger(params, 'count', { min: 1, max: 256 }) ?? 16 });
      return { instructions, text: instructions.map((item) => `${hex(item.address, 4)}  ${item.bytes.padEnd(8)}  ${item.text}${item.unofficial ? '  ; 非官方指令' : ''}`).join('\n') };
    }
    case 'resolve': {
      const bus = optionalEnum(params, 'space', ['cpu', 'ppu'], 'cpu');
      return client.request('inspector.resolve', { space: bus, address: toAddress(params.address, 'address') });
    }
    case 'spaces':
      return client.request('inspector.listSpaces', {});
    default: {
      const fields = optionalArray(params, 'fields');
      if (fields === undefined || fields.length === 0) invalidParam('修改状态要给 fields：[{ name, value }]');
      return client.request('inspector.setState', { module: optionalString(params, 'module') ?? 'cpu', fields });
    }
  }
}

/**
 * 截取最近一帧画面。
 * @param {Record<string, unknown>} params 可选 file（把 PNG 存到这个路径）、inline（返回 base64，默认 true）
 * @returns {Promise<object>} 尺寸、字节数、文件路径与 base64 图像
 */
export async function screenshot(params) {
  const { client } = currentSession();
  const result = await client.request('output.screenshot', { format: 'png' });
  const data = result.data ?? new Uint8Array();
  const file = optionalString(params, 'file');
  let saved;
  if (file !== undefined) {
    saved = path.resolve(file);
    mkdirSync(path.dirname(saved), { recursive: true });
    writeFileSync(saved, data);
  }
  return {
    format: result.format,
    width: result.width,
    height: result.height,
    byteLength: result.byteLength,
    ...(saved === undefined ? {} : { file: saved }),
    ...(optionalBoolean(params, 'inline', true) ? { imageBase64: Buffer.from(data).toString('base64') } : {}),
  };
}

/**
 * 手柄按键名换成掩码。
 * @param {unknown} value 掩码整数，或按键名数组（a、b、select、start、up、down、left、right）
 * @returns {number} 掩码
 */
function padMask(value) {
  if (Number.isInteger(value) && value >= 0 && value <= 255) return value;
  if (Array.isArray(value)) {
    return value.reduce((mask, name) => {
      if (typeof name !== 'string' || !Object.hasOwn(PAD_BITS, name.toLowerCase())) invalidParam(`未知的手柄按键：${String(name)}`, `可用的按键：${Object.keys(PAD_BITS).join('、')}`);
      return mask | PAD_BITS[name.toLowerCase()];
    }, 0);
  }
  return invalidParam('buttons 必须是 0 到 255 的掩码，或按键名数组');
}

/**
 * 输入：按键、手柄、鼠标、Family BASIC 键盘的接入，以及全部松开。
 * @param {Record<string, unknown>} params kind：key、pad、mouse、familyKeyboard、releaseAll；其余参数见各项
 * @returns {Promise<object>} 结果；按键与手柄会运行帧数让机器读到
 */
export async function input(params) {
  const { client } = currentSession();
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
      const event = optionalEnum(params, 'event', ['move', 'down', 'up', 'wheel', 'leave']);
      if (event === undefined) invalidParam('鼠标要给 event：move、down、up、wheel、leave');
      const fields = {};
      for (const name of ['x', 'y', 'dx', 'dy', 'wheel']) if (typeof params[name] === 'number') fields[name] = params[name];
      const buttons = optionalInteger(params, 'buttons', { min: 0, max: 7 });
      await client.request('input.mouse', { kind: event, ...fields, ...(buttons === undefined ? {} : { buttons }) });
      return { kind, event };
    }
    case 'familyKeyboard':
      return client.request('input.setFamilyKeyboard', { connected: optionalBoolean(params, 'connected', true) });
    default:
      await client.request('input.releaseAll', {});
      return { kind };
  }
}

/**
 * 记录：start 开始、stop 结束并取得文件。
 * @param {Record<string, unknown>} params action（start、stop）、kind（instruction、io、irq、cdl、callLog）；callLog 开始时要 address；停止跟踪与调用记录时要 id（只有一个时可省略）
 * @returns {Promise<object>} 开始时的编号，或结束时的文件路径与行数
 */
export async function record(params) {
  const session = currentSession();
  const { client } = session;
  const action = optionalEnum(params, 'action', ['start', 'stop']);
  const kind = optionalEnum(params, 'kind', ['instruction', 'io', 'irq', 'cdl', 'callLog']);
  if (action === undefined || kind === undefined) invalidParam('要给 action（start、stop）与 kind（instruction、io、irq、cdl、callLog）');
  const file = optionalString(params, 'file');
  const maxLines = optionalInteger(params, 'maxLines', { min: 1, max: 1_000_000 });
  const extra = { ...(file === undefined ? {} : { file }), ...(maxLines === undefined ? {} : { maxLines }) };

  if (kind === 'cdl') {
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
        address, ...(stackBytes === undefined ? {} : { stackBytes }), ...(params.ranges === undefined ? {} : { ranges: params.ranges }), ...(params.skip === undefined ? {} : { skip: params.skip }), ...extra,
      });
      session.records.set(id, { kind });
      return { kind, id, started: true };
    }
    const unique = optionalBoolean(params, 'unique', false);
    const { id } = await client.request('inspector.traceStart', { kind, ...(kind === 'instruction' && unique ? { unique } : {}), ...extra });
    session.records.set(id, { kind });
    return { kind, id, started: true };
  }

  let id = optionalInteger(params, 'id', { min: 1 });
  if (id === undefined) {
    const candidates = [...session.records].filter(([, entry]) => entry.kind === kind).map(([key]) => key);
    if (candidates.length !== 1) invalidParam(`要给 id（正在进行的 ${kind} 记录有 ${candidates.length} 个）`);
    id = candidates[0];
  }
  const stopped = await client.request(kind === 'callLog' ? 'inspector.callLogStop' : 'inspector.traceStop', { id });
  session.records.delete(id);
  return { kind, id, stopped: true, file: stopped.file, lines: stopped.lines, truncated: stopped.truncated };
}
