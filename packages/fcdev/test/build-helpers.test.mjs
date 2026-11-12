// ------------------------------------------------------------------------
// 名称：build-helpers.test.mjs
// 说明：测试构建辅助模块：依赖文件解析、源文件收集、工具参数拼装。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：依赖文件的样例取自 cc65 与 ca65 的真实输出，包含带空格的 Windows 路径。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import path from 'node:path';
import { readFileSync } from 'node:fs';
import { afterEach, beforeEach, describe, it } from 'node:test';
import {
  buildAssemblerArguments,
  buildCompilerArguments,
  buildLinkerArguments,
  collectSources,
  expandBankedLinkConfig,
  getBankCount,
  getSourceLanguage,
  parseDependencyFile,
  resolveLinkSetup,
} from '../src/build/index.mjs';
import { createTempDir, writeTestFile } from './helpers.mjs';

describe('多页程序的链接配置', () => {
  const banked = { option: 'banks', linkConfig: 'banked.cfg', generatedFrom: 2, maxBanks: 8, memory: 'BANK{n}: file = %O, start = $A000;', segment: 'BANK{n}: load = BANK{n};' };
  const template = 'MEMORY {\n    MAIN: x;\n    BANK1: y;\n    %BANK_MEMORY%\n}\nSEGMENTS {\n    BANK1: z;\n    %BANK_SEGMENTS%\n}\n';

  it('从 generatedFrom 起按库页个数展开内存区与段', () => {
    const text = expandBankedLinkConfig(template, banked, 4);
    assert.equal(text, 'MEMORY {\n    MAIN: x;\n    BANK1: y;\n    BANK2: file = %O, start = $A000;\n    BANK3: file = %O, start = $A000;\n    BANK4: file = %O, start = $A000;\n}\nSEGMENTS {\n    BANK1: z;\n    BANK2: load = BANK2;\n    BANK3: load = BANK3;\n    BANK4: load = BANK4;\n}\n');
  });

  it('库页个数小于 generatedFrom 时不生成任何库页行', () => {
    const text = expandBankedLinkConfig(template, banked, 1);
    assert.equal(text, 'MEMORY {\n    MAIN: x;\n    BANK1: y;\n}\nSEGMENTS {\n    BANK1: z;\n}\n');
  });

  it('模板缺少展开位置时报错', () => {
    assert.throws(() => expandBankedLinkConfig('MEMORY {}\n', banked, 3), (error) => error.code === 'TARGET_INVALID');
  });

  describe('确定链接配置与打包选项', () => {
    let temp;

    beforeEach(() => {
      temp = createTempDir();
      writeTestFile(temp.dir, 'target/banked.cfg', template);
    });

    afterEach(() => temp.cleanup());

    /** 假的构建计划：只带这个函数要用的字段 */
    function fakePlan(banks) {
      return {
        target: { dir: path.join(temp.dir, 'target'), linkConfig: 'link.cfg', banked, packagerOptions: { header: '4C', maxSize: 16384 } },
        targetOptions: { banks },
        objectDir: temp.dir,
        programName: 'prog',
      };
    }

    it('库页个数为 0 时用目标自己的链接配置与打包选项', () => {
      const setup = resolveLinkSetup(fakePlan(0), '/cc65');
      assert.equal(getBankCount(fakePlan(0)), 0);
      assert.equal(setup.linkConfig, path.join(temp.dir, 'target', 'link.cfg'));
      assert.deepEqual(setup.packagerOptions, { header: '4C', maxSize: 16384 });
    });

    it('库页个数大于 0 时写出展开后的配置，并去掉打包器的大小上限', () => {
      const setup = resolveLinkSetup(fakePlan(3), '/cc65');
      assert.equal(setup.linkConfig, path.join(temp.dir, 'prog.cfg'));
      assert.match(readFileSync(setup.linkConfig, 'utf8'), /BANK3: load = BANK3;/);
      assert.deepEqual(setup.packagerOptions, { header: '4C' });
    });

    it('目标不支持多页时库页个数为 0', () => {
      assert.equal(getBankCount({ target: { banked: null }, targetOptions: {} }), 0);
    });
  });
});

describe('依赖文件解析', () => {
  it('取出第一条规则的依赖，忽略后面的空规则', () => {
    const text = 'hello.s:        hello.c\n\nhello.c:\n';
    assert.deepEqual(parseDependencyFile(text), ['hello.c']);
  });

  it('带空格的路径用反斜杠转义，Windows 路径的反斜杠保持原样', () => {
    const text = 'hello.o:        hello.s D:\\Program\\ Files\\cc65/asminc/longbranch.mac\n\nhello.s:\n';
    assert.deepEqual(parseDependencyFile(text), ['hello.s', 'D:\\Program Files\\cc65/asminc/longbranch.mac']);
  });

  it('行尾反斜杠表示续行', () => {
    assert.deepEqual(parseDependencyFile('a.o: a.s \\\n  b.inc\n'), ['a.s', 'b.inc']);
  });

  it('目标名含盘符的冒号时不会被当成规则分隔符', () => {
    assert.deepEqual(parseDependencyFile('C:\\out\\a.o: a.s\n'), ['a.s']);
  });

  it('空内容没有依赖', () => {
    assert.deepEqual(parseDependencyFile(''), []);
  });
});

describe('源文件收集', () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('识别 .c、.s、.asm，忽略大小写与其他扩展名', () => {
    assert.equal(getSourceLanguage('a.c'), 'c');
    assert.equal(getSourceLanguage('A.S'), 'asm');
    assert.equal(getSourceLanguage('a.asm'), 'asm');
    assert.equal(getSourceLanguage('a.h'), null);
    assert.equal(getSourceLanguage('a.inc'), null);
  });

  it('递归收集，并跳过 build、node_modules、点目录与指定的输出目录', () => {
    writeTestFile(temp.dir, 'main.c', '');
    writeTestFile(temp.dir, 'sub/util.s', '');
    writeTestFile(temp.dir, 'sub/readme.md', '');
    writeTestFile(temp.dir, 'build/old.c', '');
    writeTestFile(temp.dir, 'node_modules/x/y.c', '');
    writeTestFile(temp.dir, '.cache/z.c', '');
    writeTestFile(temp.dir, 'dist/out.c', '');
    const found = collectSources(temp.dir, [path.join(temp.dir, 'dist')]).map((file) => path.relative(temp.dir, file).split(path.sep).join('/'));
    assert.deepEqual(found, ['main.c', 'sub/util.s']);
  });

  it('目录里没有源文件时报错', () => {
    writeTestFile(temp.dir, 'readme.md', '');
    assert.throws(() => collectSources(temp.dir), (error) => error.code === 'NO_SOURCES');
  });
});

describe('工具参数', () => {
  const settings = {
    cc65Target: 'nes',
    cpu: '6502',
    optimize: true,
    includeDirs: ['/inc'],
    asmIncludeDirs: ['/asm'],
    defines: { DEBUG: 1 },
  };

  it('cc65 参数带目标、优化、头文件目录、宏与依赖文件', () => {
    const args = buildCompilerArguments(settings, { source: 'a.c', output: 'a.s', dependencyFile: 'a.d' });
    assert.deepEqual(args, ['-t', 'nes', '--cpu', '6502', '-g', '-Oirs', '-I', '/inc', '-D', 'DEBUG=1', '--create-dep', 'a.d', '-o', 'a.s', 'a.c']);
  });

  it('关闭优化时不带 -Oirs', () => {
    const args = buildCompilerArguments({ ...settings, optimize: false }, { source: 'a.c', output: 'a.s', dependencyFile: 'a.d' });
    assert.equal(args.includes('-Oirs'), false);
  });

  it('ca65 参数的引入目录同时作为二进制引入目录', () => {
    const args = buildAssemblerArguments(settings, { source: 'a.s', output: 'a.o', dependencyFile: 'a.d' });
    assert.deepEqual(args.slice(5, 9), ['-I', '/asm', '--bin-include-dir', '/asm']);
  });

  it('ld65 参数把目标文件放在库之前', () => {
    const args = buildLinkerArguments({ linkConfig: 'c.cfg', binaryFile: 'o.bin', mapFile: 'o.map', debugFile: 'o.dbg', objects: ['a.o'], libraries: ['x.lib'] });
    assert.deepEqual(args.slice(-2), ['a.o', 'x.lib']);
    assert.deepEqual(args.slice(0, 2), ['-C', 'c.cfg']);
  });
});
