/*
 * ------------------------------------------------------------------------
 * 名称：recent-cartridges.test.ts
 * 说明：最近打开的卡带清单规则的测试：读出时逐项校验与去重、损坏的内容得到空清单、加入时移到最前并限制个数、移出、显示名。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { addRecentCartridge, MAX_RECENT_CARTRIDGES, parseRecentCartridges, recentCartridgeName, removeRecentCartridge, toRecentCartridgesFile } from './recent-cartridges';

describe('parseRecentCartridges', () => {
  it('读出合格的项，保持顺序', () => {
    expect(parseRecentCartridges({ version: 1, items: [{ path: 'D:/a.nes' }, { path: 'D:/b.NES' }] })).toEqual(['D:/a.nes', 'D:/b.NES']);
  });

  it('不合格的项被跳过，其余保留：没有路径、不是字符串、不是 .nes、空字符串、过长', () => {
    const raw = { items: [{ path: 'D:/a.nes' }, {}, { path: 5 }, { path: 'D:/b.txt' }, { path: '' }, 'x', null, { path: `${'x'.repeat(5000)}.nes` }, { path: 'D:/c.nes' }] };
    expect(parseRecentCartridges(raw)).toEqual(['D:/a.nes', 'D:/c.nes']);
  });

  it('重复的路径只留第一个，超出个数的丢弃', () => {
    const items = Array.from({ length: 15 }, (_, index) => ({ path: `D:/${index}.nes` }));
    expect(parseRecentCartridges({ items: [{ path: 'D:/0.nes' }, ...items] })).toHaveLength(MAX_RECENT_CARTRIDGES);
    expect(parseRecentCartridges({ items: [{ path: 'D:/a.nes' }, { path: 'D:/a.nes' }] })).toEqual(['D:/a.nes']);
  });

  it('内容缺失或损坏时得到空清单', () => {
    for (const raw of [undefined, null, 5, '文字', [], {}, { items: '不是数组' }, { items: {} }]) expect(parseRecentCartridges(raw)).toEqual([]);
  });
});

describe('addRecentCartridge 与 removeRecentCartridge', () => {
  it('加入的卡带排在最前；已在清单里的移到最前，不重复', () => {
    expect(addRecentCartridge(['D:/a.nes', 'D:/b.nes'], 'D:/c.nes')).toEqual(['D:/c.nes', 'D:/a.nes', 'D:/b.nes']);
    expect(addRecentCartridge(['D:/a.nes', 'D:/b.nes'], 'D:/b.nes')).toEqual(['D:/b.nes', 'D:/a.nes']);
  });

  it('最多保留 10 个，丢弃最早的', () => {
    const full = Array.from({ length: MAX_RECENT_CARTRIDGES }, (_, index) => `D:/${index}.nes`);
    const next = addRecentCartridge(full, 'D:/new.nes');
    expect(next).toHaveLength(MAX_RECENT_CARTRIDGES);
    expect(next[0]).toBe('D:/new.nes');
    expect(next).not.toContain(`D:/${MAX_RECENT_CARTRIDGES - 1}.nes`);
  });

  it('不是 .nes 的路径不加入', () => {
    const list = ['D:/a.nes'];
    expect(addRecentCartridge(list, 'D:/a.txt')).toBe(list);
  });

  it('移出清单里的卡带；不在清单里时原样返回', () => {
    const list = ['D:/a.nes', 'D:/b.nes'];
    expect(removeRecentCartridge(list, 'D:/a.nes')).toEqual(['D:/b.nes']);
    expect(removeRecentCartridge(list, 'D:/z.nes')).toBe(list);
  });
});

describe('显示名与保存格式', () => {
  it('显示名是文件名，Windows 与 Unix 路径都能取', () => {
    expect(recentCartridgeName('D:\\roms\\超级玛丽.nes')).toBe('超级玛丽.nes');
    expect(recentCartridgeName('/home/user/idle.nes')).toBe('idle.nes');
  });

  it('保存的内容带版本号，读回来与原清单一致', () => {
    const list = ['D:/a.nes', 'D:/b.nes'];
    const file = toRecentCartridgesFile(list);
    expect(file.version).toBe(1);
    expect(parseRecentCartridges(JSON.parse(JSON.stringify(file)))).toEqual(list);
  });
});
