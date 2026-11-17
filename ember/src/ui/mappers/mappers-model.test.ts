/*
 * ------------------------------------------------------------------------
 * 名称：mappers-model.test.ts
 * 说明：mapper 插件对话框纯规则的测试：匹配条件与主机型号的文字、按来源与搜索词筛选（不分大小写、忽略空白、匹配名称标识发布者说明）、插件与 mapper 数量的统计、清单摘要各项的内容与“无”的写法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import type { MapperDetail, MapperSummary } from '../../contracts/mappers/generated/mappers';
import { detailFacts, errorText, filterPlugins, hostsText, matchesText, pluginStats, ruleText, sourceLabel, statsText } from './mappers-model';

function summary(patch: Partial<MapperSummary> = {}): MapperSummary {
  return { id: 'nrom', name: 'NROM', version: '1.0.0', publisher: 'Ember', description: '最简单的卡带', host: ['fc'], devices: [], match: [{ mapper: 0 }], source: 'bundled', enabled: true, ...patch };
}

describe('匹配条件与主机型号', () => {
  it('只有 mapper 号时只写 mapper 号，写出的其他条件依次列在后面', () => {
    expect(ruleText({ mapper: 0 })).toBe('mapper 0');
    expect(ruleText({ mapper: 4, variant: 1, prgSize: 131072, chrSize: 8192, crc32Prg: 'abcdef12', crc32All: '0000ffff' })).toBe('mapper 4，变体 1，PRG 128 KB，CHR 8 KB，PRG 校验 ABCDEF12，整体校验 0000FFFF');
  });

  it('多条匹配条件用分号隔开，主机型号用顿号隔开，不认识的型号显示标识', () => {
    expect(matchesText([{ mapper: 0 }, { mapper: 3, variant: 2 }])).toBe('mapper 0；mapper 3，变体 2');
    expect(hostsText(['fc', 'sb2000', 'future'])).toBe('经典 FC、小霸王 2000、future');
  });

  it('来源显示为预装或用户安装', () => {
    expect(sourceLabel('bundled')).toBe('预装');
    expect(sourceLabel('user')).toBe('用户安装');
  });
});

describe('filterPlugins', () => {
  const plugins = [summary(), summary({ id: 'mmc3-plus', name: 'MMC3 增强', publisher: 'Someone', description: '', source: 'user' })];

  it('来源与搜索词都为空时返回全部', () => {
    expect(filterPlugins(plugins, 'all', '  ')).toHaveLength(2);
  });

  it('按来源筛选', () => {
    expect(filterPlugins(plugins, 'user', '').map((plugin) => plugin.id)).toEqual(['mmc3-plus']);
  });

  it('搜索词不分大小写，匹配名称、标识、发布者与说明', () => {
    expect(filterPlugins(plugins, 'all', 'nrom').map((plugin) => plugin.id)).toEqual(['nrom']);
    expect(filterPlugins(plugins, 'all', 'MMC3-P').map((plugin) => plugin.id)).toEqual(['mmc3-plus']);
    expect(filterPlugins(plugins, 'all', 'someone').map((plugin) => plugin.id)).toEqual(['mmc3-plus']);
    expect(filterPlugins(plugins, 'all', '最简单').map((plugin) => plugin.id)).toEqual(['nrom']);
    expect(filterPlugins(plugins, 'all', '不存在')).toEqual([]);
  });

  it('搜索词与被搜内容都忽略空白：mmc 3 能搜到 MMC3，mapper4 能搜到 mapper 4', () => {
    const withText = [summary({ id: 'mmc3', name: 'MMC3', description: '支持 mapper 4 的卡带' })];
    expect(filterPlugins(withText, 'all', 'mmc 3')).toHaveLength(1);
    expect(filterPlugins(withText, 'all', 'mapper4')).toHaveLength(1);
    expect(filterPlugins(withText, 'all', ' m m c3 ')).toHaveLength(1);
    expect(filterPlugins(plugins, 'all', 'mmc 3 增强').map((plugin) => plugin.id)).toEqual(['mmc3-plus']);
  });

  it('来源与搜索词同时生效', () => {
    expect(filterPlugins(plugins, 'bundled', 'someone')).toEqual([]);
  });
});

describe('pluginStats 与 statsText', () => {
  const list = [
    summary({ match: [{ mapper: 0 }] }),
    summary({ id: 'a', match: [{ mapper: 3 }, { mapper: 3, variant: 1 }], enabled: false }),
    summary({ id: 'b', match: [{ mapper: 0 }, { mapper: 4 }], source: 'user' })
  ];

  it('插件数是全部个数，已启用数不含停用的，mapper 种数按号去重且不看是否启用', () => {
    expect(pluginStats(list)).toEqual({ total: 3, enabled: 2, mappers: 3 });
    expect(pluginStats([])).toEqual({ total: 0, enabled: 0, mappers: 0 });
  });

  it('显示成一行文字', () => {
    expect(statsText(list)).toBe('共 3 个插件（已启用 2 个），支持 3 种 mapper');
  });
});

describe('detailFacts', () => {
  const detail: MapperDetail = {
    summary: summary({ devices: ['mouse'] }),
    storage: [
      { id: 'prg', kind: 'rom', from: 'file.prg', battery: false },
      { id: 'wram', kind: 'ram', size: 8192, battery: true }
    ],
    windows: [],
    nametable: 'vertical',
    registers: [{ bus: 'cpu', range: '$8000-$FFFF', read: false, write: true }],
    state: [{ name: 'bank', bits: 4, description: '当前 bank' }],
    settings: []
  };

  it('列出标识、版本、发布者、说明、匹配、主机型号、外设、存储区、寄存器范围与状态声明', () => {
    const facts = detailFacts(detail);
    expect(facts.map((fact) => fact.label)).toEqual(['标识', '版本', '发布者', '说明', '匹配', '主机型号', '外设', '存储区', '寄存器范围', '状态声明']);
    expect(facts.find((fact) => fact.label === '存储区')?.lines).toEqual(['prg：只读（来自 PRG）', 'wram：可写 8192 字节，带电池']);
    expect(facts.find((fact) => fact.label === '寄存器范围')?.lines).toEqual(['CPU $8000-$FFFF（写）']);
    expect(facts.find((fact) => fact.label === '状态声明')?.lines).toEqual(['bank（4 位）：当前 bank']);
    expect(facts.find((fact) => fact.label === '外设')?.lines).toEqual(['鼠标']);
  });

  it('没有说明与外设时不列出这两项，没有存储区、寄存器与状态时写“无”', () => {
    const facts = detailFacts({ ...detail, summary: summary({ description: undefined }), storage: [], registers: [], state: [] });
    expect(facts.map((fact) => fact.label)).not.toContain('说明');
    expect(facts.map((fact) => fact.label)).not.toContain('外设');
    for (const label of ['存储区', '寄存器范围', '状态声明']) expect(facts.find((fact) => fact.label === label)?.lines).toEqual(['无']);
  });
});

describe('errorText', () => {
  it('取错误里的说明，没有说明时用备用文字', () => {
    expect(errorText({ code: 'x', message: '包损坏' }, '失败')).toBe('包损坏');
    expect(errorText(new Error(''), '失败')).toBe('失败');
    expect(errorText(undefined, '失败')).toBe('失败');
  });
});
