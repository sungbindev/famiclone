// ------------------------------------------------------------------------
// 名称：target-parser.test.ts
// 说明：测试目标定义的解析与取值规则：字段校验、专用选项合并、路径值解析、程序名规则、多页定义。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用内存里的假目标定义测试，内置目标的读取见 test/logic/targets。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { describe, expect, it } from 'vitest';
import { parseTarget } from '../../../src/shared/targets/target-parser';
import { applyNameRule, resolveTargetOptions, resolveTargetPath } from '../../../src/shared/targets/target-settings';

const DIR = path.resolve('targets', 'fake');

/** 最小合法的目标定义。 */
function validTarget(overrides: Record<string, unknown> = {}): Record<string, unknown> {
  return {
    name: 'fake',
    title: '假目标',
    cpu: '6502',
    cc65Target: 'none',
    linkConfig: 'link.cfg',
    packager: 'flat-exe',
    extension: '.EXE',
    ...overrides
  };
}

describe('目标定义的解析', () => {
  it('缺少必需字段报错', () => {
    expect(() => parseTarget({ name: 'fake' }, DIR)).toThrow(/缺少字符串字段/);
  });

  it('目录名与 name 不一致报错', () => {
    expect(() => parseTarget(validTarget({ name: 'other' }), DIR)).toThrow(/不一致/);
  });

  it('sourceCharset 只认 gb2312', () => {
    expect(() => parseTarget(validTarget({ sourceCharset: 'big5' }), DIR)).toThrow(
      expect.objectContaining({ code: 'TARGET_INVALID', message: expect.stringContaining('sourceCharset') })
    );
    expect(parseTarget(validTarget({ sourceCharset: 'gb2312' }), DIR).sourceCharset).toBe('gb2312');
    expect(parseTarget(validTarget(), DIR).sourceCharset).toBeNull();
  });

  it('运行方案的 mode 只能是 rom、firmware-disk 或 firmware', () => {
    expect(() => parseTarget(validTarget({ run: { emulator: 'ember', mode: 'x' } }), DIR)).toThrow(/mode/);
    expect(parseTarget(validTarget({ run: { emulator: 'ember', mode: 'rom' } }), DIR).run?.mode).toBe('rom');
    expect(parseTarget(validTarget({ run: { emulator: 'ember', mode: 'firmware' } }), DIR).run).toEqual({ emulator: 'ember', mode: 'firmware', machine: null });
  });

  it('firmware-disk 的运行方案必须给整机机型，rom 的机型为 null', () => {
    expect(() => parseTarget(validTarget({ run: { emulator: 'ember', mode: 'firmware-disk' } }), DIR)).toThrow(/machine/);
    expect(parseTarget(validTarget({ run: { emulator: 'ember', mode: 'firmware-disk', machine: 'sb97' } }), DIR).run?.machine).toBe('sb97');
    expect(parseTarget(validTarget({ run: { emulator: 'ember', mode: 'rom' } }), DIR).run?.machine).toBeNull();
  });

  it('disk 的 baseImage 缺省为 false，只能是布尔值', () => {
    expect(parseTarget(validTarget({ disk: { packager: 'fat-floppy' } }), DIR).disk).toEqual({ packager: 'fat-floppy', baseImage: false });
    expect(parseTarget(validTarget({ disk: { packager: 'fat-floppy-overlay', baseImage: true } }), DIR).disk).toEqual({ packager: 'fat-floppy-overlay', baseImage: true });
    expect(() => parseTarget(validTarget({ disk: { packager: 'fat-floppy', baseImage: 'yes' } }), DIR)).toThrow(/baseImage/);
    expect(parseTarget(validTarget(), DIR).disk).toBeNull();
  });

  it('links 只能用在布尔选项上', () => {
    const options = { size: { type: 'number', default: 1, links: ['a.o'] } };
    expect(() => parseTarget(validTarget({ options }), DIR)).toThrow(/links/);
  });

  it('extension 可以是空字符串（没有扩展名），不是字符串时报错', () => {
    expect(parseTarget(validTarget({ extension: '' }), DIR).extension).toBe('');
    expect(() => parseTarget(validTarget({ extension: undefined }), DIR)).toThrow(/extension/);
  });

  it('地址选项：缺省值是范围内的数字，linkSymbol 与范围只用在数字与地址选项上', () => {
    const option = { type: 'address', default: 0x6000, min: 0x800, max: 0x8fff, linkSymbol: 'LOAD_ADDRESS' };
    expect(parseTarget(validTarget({ options: { loadAddress: option } }), DIR).options.loadAddress).toEqual({ type: 'address', default: 0x6000, links: [], linkSymbol: 'LOAD_ADDRESS', min: 0x800, max: 0x8fff });
    expect(() => parseTarget(validTarget({ options: { loadAddress: { ...option, default: '$6000' } } }), DIR)).toThrow(/定义不合法/);
    expect(() => parseTarget(validTarget({ options: { loadAddress: { ...option, default: 0x9000 } } }), DIR)).toThrow(/缺省值/);
    expect(() => parseTarget(validTarget({ options: { size: { type: 'boolean', default: true, linkSymbol: 'X' } } }), DIR)).toThrow(/linkSymbol/);
    expect(() => parseTarget(validTarget({ options: { loadAddress: { ...option, linkSymbol: '1bad' } } }), DIR)).toThrow(/linkSymbol/);
    expect(() => parseTarget(validTarget({ options: { size: { type: 'number', default: 1, max: 5 } } }), DIR)).toThrow(/max/);
  });
});

describe('目标定义的取值规则', () => {
  it('路径值按目录或工具链解析', () => {
    const target = parseTarget(validTarget(), DIR);
    expect(resolveTargetPath(target, 'lib/a.lib', '/cc65')).toBe(path.join(DIR, 'lib', 'a.lib'));
    expect(resolveTargetPath(target, 'toolchain:cfg/none.cfg', '/cc65')).toBe(path.join('/cc65', 'cfg', 'none.cfg'));
  });

  it('目标选项合并默认值，未知选项与类型错误报错', () => {
    const target = parseTarget(validTarget({ options: { font: { type: 'boolean', default: true, links: ['lib/f.o'] } } }), DIR);
    expect(resolveTargetOptions(target, {})).toEqual({ font: true });
    expect(resolveTargetOptions(target, { font: false })).toEqual({ font: false });
    expect(() => resolveTargetOptions(target, { color: 1 })).toThrow(expect.objectContaining({ code: 'TARGET_OPTION_UNKNOWN' }));
    expect(() => resolveTargetOptions(target, { font: 'no' })).toThrow(expect.objectContaining({ code: 'TARGET_OPTION_INVALID' }));
  });

  it('地址选项接受数字与 "$6000"、"0x6000"、十进制字符串，超出范围或格式不对报错', () => {
    const option = { type: 'address', default: 0x6000, min: 0x800, max: 0x8fff, linkSymbol: 'LOAD_ADDRESS' };
    const target = parseTarget(validTarget({ options: { loadAddress: option } }), DIR);
    expect(resolveTargetOptions(target, {})).toEqual({ loadAddress: 0x6000 });
    for (const value of [0x800, '$0800', '0x0800', '2048', ' $0800 ']) {
      expect(resolveTargetOptions(target, { loadAddress: value })).toEqual({ loadAddress: 0x800 });
    }
    for (const value of [0x7ff, 0x9000, '$7FF', 'abc', '$', true, 1.5, -1]) {
      expect(() => resolveTargetOptions(target, { loadAddress: value })).toThrow(expect.objectContaining({ code: 'TARGET_OPTION_INVALID' }));
    }
  });

  describe('banked 多页定义', () => {
    const banked = { option: 'banks', linkConfig: 'banked.cfg', generatedFrom: 2, maxBanks: 8, memory: 'BANK{n}: x;', segment: 'BANK{n}: y;' };
    const numberOption = { banks: { type: 'number', default: 0 } };

    it('库页个数在 0 到上限之间', () => {
      const target = parseTarget(validTarget({ options: numberOption, banked }), DIR);
      expect(target.banked?.maxBanks).toBe(8);
      expect(resolveTargetOptions(target, {})).toEqual({ banks: 0 });
      expect(resolveTargetOptions(target, { banks: 8 })).toEqual({ banks: 8 });
      for (const bad of [9, -1, 1.5]) {
        expect(() => resolveTargetOptions(target, { banks: bad })).toThrow(expect.objectContaining({ code: 'TARGET_OPTION_INVALID', message: expect.stringContaining('0 到 8') }));
      }
    });

    it('选项必须是数字选项，字段必须齐全', () => {
      expect(() => parseTarget(validTarget({ options: { banks: { type: 'boolean', default: false } }, banked }), DIR)).toThrow(/数字选项/);
      expect(() => parseTarget(validTarget({ options: numberOption, banked: { ...banked, maxBanks: 0 } }), DIR)).toThrow(/maxBanks/);
      expect(() => parseTarget(validTarget({ options: numberOption, banked: { ...banked, memory: undefined } }), DIR)).toThrow(/memory/);
    });
  });
});

describe('程序名规则', () => {
  it('没有规则时原样返回', () => {
    expect(applyNameRule('my-long-program-name', null)).toBe('my-long-program-name');
  });

  it('DOS 3.3 规则：字母开头，只用字母与数字，最多 30 个字符，转大写，不符合时报错', () => {
    expect(applyNameRule('Demo1', 'dos33')).toBe('DEMO1');
    expect(applyNameRule('a'.repeat(30), 'dos33')).toBe('A'.repeat(30));
    for (const name of ['1demo', 'my-demo', 'a.b', 'a'.repeat(31), '']) {
      expect(() => applyNameRule(name, 'dos33')).toThrow(expect.objectContaining({ code: 'NAME_INVALID' }));
    }
  });

  it('DOS 8.3 规则转大写', () => {
    expect(applyNameRule('hello', 'dos-8.3')).toBe('HELLO');
  });

  it('超过 8 个字符或含非法字符报错，不截断', () => {
    for (const name of ['toolongname', 'a b', 'a.b']) {
      expect(() => applyNameRule(name, 'dos-8.3')).toThrow(expect.objectContaining({ code: 'NAME_INVALID' }));
    }
  });
});
