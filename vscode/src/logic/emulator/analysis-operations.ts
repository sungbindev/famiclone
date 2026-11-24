// ------------------------------------------------------------------------
// 名称：analysis-operations.ts
// 说明：分析类操作：profile（子程序周期统计）与 events（PPU 与中断事件记录）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-24
// 备注：统计与事件记录都由 Ember 在机器运行时累计；get 不打断记录，stop 结束并返回最后的结果。
// ------------------------------------------------------------------------

import type { InspectorDebugEvent, InspectorProfileEntry } from '../../contracts/debug-client/generated/api';
import type { OperationContext } from './operation-context';
import { optionalEnum, optionalInteger } from './params';

/**
 * 十六进制写出数值，前面加 $。
 * @param value 数值。
 * @param digits 位数。
 * @returns 如 $C000。
 */
function hexText(value: number, digits: number): string {
  return `$${value.toString(16).toUpperCase().padStart(digits, '0')}`;
}

/**
 * 把一项子程序统计写成一行。
 * @param entry 统计项。
 * @returns 文字。
 */
function profileLine(entry: InspectorProfileEntry): string {
  const where = entry.kind === 'root' ? 'root' : `${entry.kind} ${hexText(entry.address, 4)}`;
  return `${where}  调用 ${entry.calls} 次  含子程序 ${entry.inclusiveCycles} 周期  不含子程序 ${entry.exclusiveCycles} 周期`;
}

/**
 * 把一个事件写成一行。
 * @param event 事件。
 * @returns 文字。
 */
function eventLine(event: InspectorDebugEvent): string {
  const place = `帧 ${event.frame} 行 ${event.scanline} 点 ${event.dot}`;
  const access = event.kind === 'read' || event.kind === 'write';
  const detail = access
    ? `${event.kind} ${hexText(event.address, 4)}=${hexText(event.value, 2)} 指令 ${hexText(event.pc, 4)}`
    : `${event.kind} 处理程序 ${hexText(event.address, 4)} 返回 ${hexText(event.pc, 4)}`;
  return `${place}  ${detail}  周期 ${event.cycles}`;
}

/**
 * 子程序周期统计：start 开始，get 读取当前结果（统计继续），stop 结束并返回结果。
 * @param context 上下文。
 * @param params action（start、get、stop，默认 get）。
 * @returns start 返回 started；get 与 stop 返回总周期数、各项统计与文字。
 */
export async function profile(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { client } = context.store.currentSession();
  const action = optionalEnum(params, 'action', ['start', 'get', 'stop'], 'get');
  if (action === 'start') {
    await client.request('inspector.profilerStart', {});
    return { action, started: true };
  }
  const result = action === 'stop' ? await client.request('inspector.profilerStop', {}) : await client.request('inspector.profilerGet', {});
  return {
    action,
    active: result.active,
    totalCycles: result.totalCycles,
    entries: result.entries.map((entry) => ({ ...entry, address: entry.kind === 'root' ? undefined : hexText(entry.address, 4) })),
    text: result.entries.map(profileLine).join('\n')
  };
}

/**
 * PPU 与中断事件记录：start 开始，get 读取，stop 结束。
 * @param context 上下文。
 * @param params action（start、get、stop，默认 get）；start 可给 maxEvents（保留的事件数）；get 可给
 *   fromFrame、maxEvents（读取的事件数）。
 * @returns start 与 stop 返回状态；get 返回事件的文字与个数、被丢弃的个数。
 */
export async function events(context: OperationContext, params: Record<string, unknown>): Promise<Record<string, unknown>> {
  const { client } = context.store.currentSession();
  const action = optionalEnum(params, 'action', ['start', 'get', 'stop'], 'get');
  const maxEvents = optionalInteger(params, 'maxEvents', { min: 1, max: 1_000_000 });
  if (action === 'start') {
    await client.request('inspector.eventsStart', maxEvents === undefined ? {} : { maxEvents });
    return { action, started: true };
  }
  if (action === 'stop') {
    await client.request('inspector.eventsStop', {});
    return { action, stopped: true };
  }
  const fromFrame = optionalInteger(params, 'fromFrame', { min: 0 });
  const result = await client.request('inspector.eventsGet', { ...(fromFrame === undefined ? {} : { fromFrame }), ...(maxEvents === undefined ? {} : { maxEvents }) });
  return { action, active: result.active, frame: result.frame, dropped: result.dropped, count: result.events.length, text: result.events.map(eventLine).join('\n') };
}
