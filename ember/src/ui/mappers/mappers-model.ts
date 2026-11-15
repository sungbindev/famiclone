/*
 * ------------------------------------------------------------------------
 * 名称：mappers-model.ts
 * 说明：mapper 插件对话框的纯规则：来源与主机型号的显示名、匹配条件的文字、按搜索词与来源筛选、清单摘要的各项文字。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：不依赖 React，便于单独测试；文字规则见 docs/05-界面.md 第 4.4 节。
 * ------------------------------------------------------------------------
 */

import type { MapperDetail, MapperMatchRule, MapperSummary } from '../../contracts/mappers/generated/mappers';

/** 来源筛选的取值：全部，或某一种来源。 */
export type SourceFilter = 'all' | MapperSummary['source'];

/** 清单摘要里的一项：标签与若干行文字。 */
export interface DetailFact {
  readonly label: string;
  readonly lines: readonly string[];
}

/** 用户可见的文字。 */
const TEXT = {
  source: { bundled: '预装', user: '用户安装' },
  machines: { fc: '经典 FC', sb97: '小霸王 97', sb2000: '小霸王 2000', bbk10: '步步高 1.0', bbk20: '步步高 2.0' } as Readonly<Record<string, string>>,
  devices: { mouse: '鼠标' } as Readonly<Record<string, string>>,
  rule: {
    mapper: (value: number) => `mapper ${value}`,
    variant: (value: number) => `变体 ${value}`,
    prg: (bytes: number) => `PRG ${bytes / 1024} KB`,
    chr: (bytes: number) => `CHR ${bytes / 1024} KB`,
    crcPrg: (value: string) => `PRG 校验 ${value.toUpperCase()}`,
    crcAll: (value: string) => `整体校验 ${value.toUpperCase()}`
  },
  none: '无',
  rom: (id: string, from: string) => `${id}：只读（${from === 'file.prg' ? '来自 PRG' : '来自 CHR'}）`,
  ram: (id: string, size: number, battery: boolean) => `${id}：可写 ${size} 字节${battery ? '，带电池' : ''}`,
  register: (bus: string, range: string, read: boolean, write: boolean) => `${bus === 'cpu' ? 'CPU' : 'PPU'} ${range}（${read && write ? '读写' : read ? '读' : '写'}）`,
  state: (name: string, bits: number, description: string) => `${name}（${bits} 位）${description === '' ? '' : `：${description}`}`,
  labels: { id: '标识', version: '版本', publisher: '发布者', description: '说明', match: '匹配', host: '主机型号', devices: '外设', storage: '存储区', registers: '寄存器范围', state: '状态声明' }
} as const;

/** 来源的显示名。 */
export function sourceLabel(source: MapperSummary['source']): string {
  return TEXT.source[source];
}

/** 主机型号的显示名；不认识的型号显示标识。 */
export function hostLabel(host: string): string {
  return TEXT.machines[host] ?? host;
}

/** 一项匹配条件的文字：mapper 号加写出的其他条件。 */
export function ruleText(rule: MapperMatchRule): string {
  const parts: string[] = [TEXT.rule.mapper(rule.mapper)];
  if (rule.variant !== undefined) parts.push(TEXT.rule.variant(rule.variant));
  if (rule.prgSize !== undefined) parts.push(TEXT.rule.prg(rule.prgSize));
  if (rule.chrSize !== undefined) parts.push(TEXT.rule.chr(rule.chrSize));
  if (rule.crc32Prg !== undefined) parts.push(TEXT.rule.crcPrg(rule.crc32Prg));
  if (rule.crc32All !== undefined) parts.push(TEXT.rule.crcAll(rule.crc32All));
  return parts.join('，');
}

/** 全部匹配条件在表格里的一行文字。 */
export function matchesText(rules: readonly MapperMatchRule[]): string {
  return rules.map(ruleText).join('；');
}

/** 主机型号在表格里的一行文字。 */
export function hostsText(hosts: readonly string[]): string {
  return hosts.map(hostLabel).join('、');
}

/**
 * 按来源与搜索词筛选插件；搜索词不分大小写，匹配名称、标识、发布者与说明。
 * @param plugins 全部插件。
 * @param source 来源筛选。
 * @param search 搜索词，空白不筛选。
 */
export function filterPlugins(plugins: readonly MapperSummary[], source: SourceFilter, search: string): readonly MapperSummary[] {
  const needle = search.trim().toLowerCase();
  return plugins.filter((plugin) => {
    if (source !== 'all' && plugin.source !== source) return false;
    if (needle === '') return true;
    return [plugin.name, plugin.id, plugin.publisher, plugin.description ?? ''].some((text) => text.toLowerCase().includes(needle));
  });
}

/** 清单摘要的各项，没有内容的项不列出。 */
export function detailFacts(detail: MapperDetail): readonly DetailFact[] {
  const { summary } = detail;
  const facts: DetailFact[] = [
    { label: TEXT.labels.id, lines: [summary.id] },
    { label: TEXT.labels.version, lines: [summary.version] },
    { label: TEXT.labels.publisher, lines: [summary.publisher] }
  ];
  if (summary.description !== undefined && summary.description !== '') facts.push({ label: TEXT.labels.description, lines: [summary.description] });
  facts.push({ label: TEXT.labels.match, lines: summary.match.map(ruleText) });
  facts.push({ label: TEXT.labels.host, lines: [hostsText(summary.host)] });
  if (summary.devices.length > 0) facts.push({ label: TEXT.labels.devices, lines: [summary.devices.map((device) => TEXT.devices[device] ?? device).join('、')] });
  facts.push({
    label: TEXT.labels.storage,
    lines: detail.storage.length === 0 ? [TEXT.none] : detail.storage.map((item) => (item.kind === 'rom' ? TEXT.rom(item.id, item.from ?? '') : TEXT.ram(item.id, item.size ?? 0, item.battery)))
  });
  facts.push({
    label: TEXT.labels.registers,
    lines: detail.registers.length === 0 ? [TEXT.none] : detail.registers.map((item) => TEXT.register(item.bus, item.range, item.read, item.write))
  });
  facts.push({ label: TEXT.labels.state, lines: detail.state.length === 0 ? [TEXT.none] : detail.state.map((item) => TEXT.state(item.name, item.bits, item.description)) });
  return facts;
}

/** 错误里给用户看的说明。 */
export function errorText(error: unknown, fallback: string): string {
  if (typeof error === 'object' && error !== null && 'message' in error && typeof error.message === 'string' && error.message !== '') return error.message;
  return fallback;
}
