/*
 * ------------------------------------------------------------------------
 * 名称：preferences-model.test.ts
 * 说明：首选项页面纯逻辑的测试：显示哪些分组与设置项、是否与默认不同、修改后的列表、整数解析与范围、错误说明。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import type { PreferenceGroup, PreferenceSetting } from '../../contracts/preferences/generated/preferences';
import { biosStatusText, errorText, isChanged, MACHINES_GROUP_ID, parseInteger, visibleGroups, visibleSettings, withMachinesGroup, withValue } from './preferences-model';

function setting(id: string, group: string, overrides: Partial<PreferenceSetting> = {}): PreferenceSetting {
  return { id, group, label: id, description: '', kind: 'switch', hidden: false, defaultValue: false, value: false, ...overrides };
}

const GROUPS: PreferenceGroup[] = [
  { id: 'appearance', label: '外观' },
  { id: 'sound', label: '声音' },
  { id: 'hiddenOnly', label: '只有隐藏项' }
];

describe('withMachinesGroup', () => {
  it('把“机型与 BIOS”放在声音分组之后，没有声音分组时放在最后', () => {
    const groups: PreferenceGroup[] = [
      { id: 'sound', label: '声音' },
      { id: 'printer', label: '打印机' },
      { id: 'debug', label: '调试服务' }
    ];
    expect(withMachinesGroup(groups).map((group) => group.label)).toEqual(['声音', '机型与 BIOS', '打印机', '调试服务']);
    expect(withMachinesGroup(groups.slice(1)).map((group) => group.id)).toEqual(['printer', 'debug', MACHINES_GROUP_ID]);
  });
});

describe('biosStatusText', () => {
  const base = { id: 'sb2000', name: '小霸王 2000', needsBios: true, supported: true };

  it('暂未支持、未添加、已添加（文件名与大小，KB 四舍五入）', () => {
    expect(biosStatusText({ ...base, supported: false })).toBe('暂未支持');
    expect(biosStatusText(base)).toBe('未添加');
    expect(biosStatusText({ ...base, bios: { fileName: 'SB2K_BIOS.NES', size: 524304 } })).toBe('SB2K_BIOS.NES（512 KB）');
  });
});

describe('visibleSettings 与 visibleGroups', () => {
  const settings = [setting('appearance.theme', 'appearance'), setting('sound.buffer', 'sound'), setting('sound.volume', 'sound', { hidden: true }), setting('hiddenOnly.x', 'hiddenOnly', { hidden: true })];

  it('隐藏的设置项不显示', () => {
    expect(visibleSettings(settings).map((item) => item.id)).toEqual(['appearance.theme', 'sound.buffer']);
  });

  it('没有显示项的分组不显示，顺序保持核心给出的顺序', () => {
    expect(visibleGroups(GROUPS, settings).map((group) => group.id)).toEqual(['appearance', 'sound']);
    expect(visibleGroups(GROUPS.toReversed(), settings).map((group) => group.id)).toEqual(['sound', 'appearance']);
  });
});

describe('isChanged', () => {
  it('取值与出厂默认值不同才算改过', () => {
    expect(isChanged(setting('a.b', 'a', { defaultValue: 'system', value: 'system' }))).toBe(false);
    expect(isChanged(setting('a.b', 'a', { defaultValue: 'system', value: 'dark' }))).toBe(true);
    expect(isChanged(setting('a.b', 'a', { defaultValue: true, value: false }))).toBe(true);
    expect(isChanged(setting('a.b', 'a', { kind: 'integer', defaultValue: 100, value: 100 }))).toBe(false);
  });
});

describe('withValue', () => {
  it('只改指定的设置项，其他原样，不认识的标识不改动', () => {
    const settings = [setting('a.one', 'a', { value: 1 }), setting('a.two', 'a', { value: 2 })];
    expect(withValue(settings, 'a.two', 9).map((item) => item.value)).toEqual([1, 9]);
    expect(withValue(settings, 'a.none', 9).map((item) => item.value)).toEqual([1, 2]);
    expect(settings[1]?.value).toBe(2);
  });
});

describe('parseInteger', () => {
  const range = { minimum: 0, maximum: 100 };

  it('合格的整数（可带空白与负号）', () => {
    expect(parseInteger(' 42 ', range)).toEqual({ ok: true, value: 42 });
    expect(parseInteger('0', range)).toEqual({ ok: true, value: 0 });
    expect(parseInteger('-5', { minimum: -10, maximum: 10 })).toEqual({ ok: true, value: -5 });
  });

  it('不是整数时提示', () => {
    for (const text of ['', ' ', 'abc', '1.5', '1e3', '--1', '12a']) expect(parseInteger(text, range)).toEqual({ ok: false, message: '请输入整数。' });
  });

  it('超出范围时提示范围', () => {
    expect(parseInteger('101', range)).toEqual({ ok: false, message: '请输入 0 至 100 之间的整数。' });
    expect(parseInteger('-1', range)).toEqual({ ok: false, message: '请输入 0 至 100 之间的整数。' });
  });

  it('没有登记范围时不限制', () => {
    expect(parseInteger('99999', {})).toEqual({ ok: true, value: 99999 });
  });
});

describe('errorText', () => {
  it('取核心错误里的说明，取不到时用通用说明', () => {
    expect(errorText({ code: 'preferences.invalidValue', message: '缓冲大小：取值必须是下面之一' })).toBe('缓冲大小：取值必须是下面之一');
    expect(errorText(new Error('没有响应'))).toBe('没有响应');
    expect(errorText({ message: '' })).toBe('操作失败。');
    expect(errorText('字符串')).toBe('操作失败。');
    expect(errorText(null)).toBe('操作失败。');
  });
});
