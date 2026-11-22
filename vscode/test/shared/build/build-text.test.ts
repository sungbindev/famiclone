// ------------------------------------------------------------------------
// 名称：build-text.test.ts
// 说明：测试构建文本处理：源文件语言、依赖文件解析、工具参数拼装、多页链接配置展开。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：依赖文件的样例取自 cc65 与 ca65 的真实输出，包含带空格的 Windows 路径。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import {
  buildAssemblerArguments,
  buildCompilerArguments,
  buildLinkerArguments,
  expandBankedLinkConfig,
  getSourceLanguage,
  parseDependencyFile
} from '../../../src/shared/build/build-text';
import type { BankedSpec } from '../../../src/shared/targets/target';

describe('多页程序的链接配置', () => {
  const banked: BankedSpec = {
    option: 'banks',
    linkConfig: 'banked.cfg',
    generatedFrom: 2,
    maxBanks: 8,
    memory: 'BANK{n}: file = %O, start = $A000;',
    segment: 'BANK{n}: load = BANK{n};'
  };
  const template = 'MEMORY {\n    MAIN: x;\n    BANK1: y;\n    %BANK_MEMORY%\n}\nSEGMENTS {\n    BANK1: z;\n    %BANK_SEGMENTS%\n}\n';

  it('从 generatedFrom 起按库页个数展开内存区与段', () => {
    expect(expandBankedLinkConfig(template, banked, 4)).toBe(
      'MEMORY {\n    MAIN: x;\n    BANK1: y;\n    BANK2: file = %O, start = $A000;\n    BANK3: file = %O, start = $A000;\n' +
        '    BANK4: file = %O, start = $A000;\n}\nSEGMENTS {\n    BANK1: z;\n    BANK2: load = BANK2;\n    BANK3: load = BANK3;\n' +
        '    BANK4: load = BANK4;\n}\n'
    );
  });

  it('库页个数小于 generatedFrom 时不生成任何库页行', () => {
    expect(expandBankedLinkConfig(template, banked, 1)).toBe('MEMORY {\n    MAIN: x;\n    BANK1: y;\n}\nSEGMENTS {\n    BANK1: z;\n}\n');
  });

  it('模板缺少展开位置时报错', () => {
    expect(() => expandBankedLinkConfig('MEMORY {}\n', banked, 3)).toThrow(expect.objectContaining({ code: 'TARGET_INVALID' }));
  });
});

describe('依赖文件解析', () => {
  it('取出第一条规则的依赖，忽略后面的空规则', () => {
    expect(parseDependencyFile('hello.s:        hello.c\n\nhello.c:\n')).toEqual(['hello.c']);
  });

  it('带空格的路径用反斜杠转义，Windows 路径的反斜杠保持原样', () => {
    const text = 'hello.o:        hello.s D:\\Program\\ Files\\cc65/asminc/longbranch.mac\n\nhello.s:\n';
    expect(parseDependencyFile(text)).toEqual(['hello.s', 'D:\\Program Files\\cc65/asminc/longbranch.mac']);
  });

  it('行尾反斜杠表示续行', () => {
    expect(parseDependencyFile('a.o: a.s \\\n  b.inc\n')).toEqual(['a.s', 'b.inc']);
  });

  it('目标名含盘符的冒号时不会被当成规则分隔符', () => {
    expect(parseDependencyFile('C:\\out\\a.o: a.s\n')).toEqual(['a.s']);
  });

  it('空内容没有依赖', () => {
    expect(parseDependencyFile('')).toEqual([]);
  });
});

describe('源文件语言', () => {
  it('识别 .c、.s、.asm，忽略大小写与其他扩展名', () => {
    expect(getSourceLanguage('a.c')).toBe('c');
    expect(getSourceLanguage('A.S')).toBe('asm');
    expect(getSourceLanguage('a.asm')).toBe('asm');
    expect(getSourceLanguage('a.h')).toBeNull();
    expect(getSourceLanguage('a.inc')).toBeNull();
  });
});

describe('工具参数', () => {
  const settings = {
    cc65Target: 'nes',
    cpu: '6502',
    optimize: true,
    includeDirs: ['/inc'],
    asmIncludeDirs: ['/asm'],
    defines: { DEBUG: 1 }
  };

  it('cc65 参数带目标、优化、头文件目录、宏与依赖文件', () => {
    expect(buildCompilerArguments(settings, { source: 'a.c', output: 'a.s', dependencyFile: 'a.d' })).toEqual([
      '-t', 'nes', '--cpu', '6502', '-g', '-Oirs', '-I', '/inc', '-D', 'DEBUG=1', '--create-dep', 'a.d', '-o', 'a.s', 'a.c'
    ]);
  });

  it('关闭优化时不带 -Oirs', () => {
    const args = buildCompilerArguments({ ...settings, optimize: false }, { source: 'a.c', output: 'a.s', dependencyFile: 'a.d' });
    expect(args).not.toContain('-Oirs');
  });

  it('ca65 参数的引入目录同时作为二进制引入目录', () => {
    const args = buildAssemblerArguments(settings, { source: 'a.s', output: 'a.o', dependencyFile: 'a.d' });
    expect(args.slice(5, 9)).toEqual(['-I', '/asm', '--bin-include-dir', '/asm']);
  });

  it('ld65 参数把目标文件放在库之前', () => {
    const args = buildLinkerArguments({
      linkConfig: 'c.cfg',
      binaryFile: 'o.bin',
      mapFile: 'o.map',
      debugFile: 'o.dbg',
      objects: ['a.o'],
      libraries: ['x.lib']
    });
    expect(args.slice(-2)).toEqual(['a.o', 'x.lib']);
    expect(args.slice(0, 2)).toEqual(['-C', 'c.cfg']);
  });
});
