// ------------------------------------------------------------------------
// 名称：targets-packagers.test.mjs
// 说明：测试目标注册表、程序名规则与打包器：字段校验、选项合并、8.3 校验、文件头与大小校验、iNES 校验。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：目标用临时目录里的假目标测试，另检查内置目标都能被读取。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { getPackager, parseHeaderPattern } from '../src/packagers/index.mjs';
import { applyNameRule, listTargetNames, loadTarget, resolveTargetOptions, resolveTargetPath } from '../src/targets/index.mjs';
import { createTempDir, writeTestFile } from './helpers.mjs';

/** 最小合法的目标定义 */
function validTarget(overrides = {}) {
  return {
    name: 'fake',
    title: '假目标',
    cpu: '6502',
    cc65Target: 'none',
    linkConfig: 'link.cfg',
    packager: 'flat-exe',
    extension: '.EXE',
    ...overrides,
  };
}

describe('目标注册表', () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('内置目标 nes 与 sb2000 能读取', () => {
    assert.ok(listTargetNames().includes('nes'));
    assert.equal(loadTarget('nes').cc65Target, 'nes');
    assert.deepEqual(listTargetNames().filter((name) => ['nes', 'sb2000'].includes(name)), ['nes', 'sb2000']);
    assert.equal(loadTarget('sb2000').nameRule, 'dos-8.3');
    assert.equal(loadTarget('sb2000').run.mode, 'firmware-disk');
  });

  it('不存在的目标报错并列出可用目标', () => {
    assert.throws(() => loadTarget('nope'), (error) => error.code === 'TARGET_UNKNOWN' && error.hint.includes('nes'));
  });

  it('目标名不合法时不会逃出目标目录', () => {
    assert.throws(() => loadTarget('../x'), (error) => error.code === 'TARGET_UNKNOWN');
  });

  it('缺少必需字段报错', () => {
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify({ name: 'fake' }));
    assert.throws(() => loadTarget('fake', temp.dir), /缺少字符串字段/);
  });

  it('目录名与 name 不一致报错', () => {
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify(validTarget({ name: 'other' })));
    assert.throws(() => loadTarget('fake', temp.dir), /不一致/);
  });

  it('路径值按目录或工具链解析', () => {
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify(validTarget()));
    const target = loadTarget('fake', temp.dir);
    assert.equal(resolveTargetPath(target, 'lib/a.lib', '/cc65'), path.join(temp.dir, 'fake', 'lib', 'a.lib'));
    assert.equal(resolveTargetPath(target, 'toolchain:cfg/none.cfg', '/cc65'), path.join('/cc65', 'cfg', 'none.cfg'));
  });

  it('目标选项合并默认值，未知选项与类型错误报错', () => {
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify(validTarget({ options: { font: { type: 'boolean', default: true, links: ['lib/f.o'] } } })));
    const target = loadTarget('fake', temp.dir);
    assert.deepEqual(resolveTargetOptions(target, {}), { font: true });
    assert.deepEqual(resolveTargetOptions(target, { font: false }), { font: false });
    assert.throws(() => resolveTargetOptions(target, { color: 1 }), (error) => error.code === 'TARGET_OPTION_UNKNOWN');
    assert.throws(() => resolveTargetOptions(target, { font: 'no' }), (error) => error.code === 'TARGET_OPTION_INVALID');
  });

  it('links 只能用在布尔选项上', () => {
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify(validTarget({ options: { size: { type: 'number', default: 1, links: ['a.o'] } } })));
    assert.throws(() => loadTarget('fake', temp.dir), /links/);
  });

  it('banked 定义多页程序：选项必须是数字选项，库页个数在 0 到上限之间', () => {
    const banked = { option: 'banks', linkConfig: 'banked.cfg', generatedFrom: 2, maxBanks: 8, memory: 'BANK{n}: x;', segment: 'BANK{n}: y;' };
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify(validTarget({ options: { banks: { type: 'number', default: 0 } }, banked })));
    const target = loadTarget('fake', temp.dir);
    assert.equal(target.banked.maxBanks, 8);
    assert.deepEqual(resolveTargetOptions(target, {}), { banks: 0 });
    assert.deepEqual(resolveTargetOptions(target, { banks: 8 }), { banks: 8 });
    for (const bad of [9, -1, 1.5]) {
      assert.throws(() => resolveTargetOptions(target, { banks: bad }), (error) => error.code === 'TARGET_OPTION_INVALID' && error.message.includes('0 到 8'));
    }
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify(validTarget({ options: { banks: { type: 'boolean', default: false } }, banked })));
    assert.throws(() => loadTarget('fake', temp.dir), /数字选项/);
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify(validTarget({ options: { banks: { type: 'number', default: 0 } }, banked: { ...banked, maxBanks: 0 } })));
    assert.throws(() => loadTarget('fake', temp.dir), /maxBanks/);
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify(validTarget({ options: { banks: { type: 'number', default: 0 } }, banked: { ...banked, memory: undefined } })));
    assert.throws(() => loadTarget('fake', temp.dir), /memory/);
  });

  it('内置目标 sb2000 支持多页程序，nes 不支持', () => {
    assert.equal(loadTarget('nes').banked, null);
    assert.equal(loadTarget('sb2000').banked.option, 'banks');
    assert.equal(loadTarget('sb2000').banked.maxBanks, 113);
  });
});

describe('程序名规则', () => {
  it('没有规则时原样返回', () => {
    assert.equal(applyNameRule('my-long-program-name', null), 'my-long-program-name');
  });

  it('DOS 8.3 规则转大写', () => {
    assert.equal(applyNameRule('hello', 'dos-8.3'), 'HELLO');
  });

  it('超过 8 个字符或含非法字符报错，不截断', () => {
    assert.throws(() => applyNameRule('toolongname', 'dos-8.3'), (error) => error.code === 'NAME_INVALID');
    assert.throws(() => applyNameRule('a b', 'dos-8.3'), (error) => error.code === 'NAME_INVALID');
    assert.throws(() => applyNameRule('a.b', 'dos-8.3'), (error) => error.code === 'NAME_INVALID');
  });
});

describe('打包器', () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('未知打包器报错', () => {
    assert.throws(() => getPackager('nope'), (error) => error.code === 'PACKAGER_UNKNOWN');
  });

  it('文件头模式里 ?? 是通配', () => {
    assert.deepEqual(parseHeaderPattern('4C????AA55'), [0x4c, null, null, 0xaa, 0x55]);
    assert.throws(() => parseHeaderPattern('4G'), (error) => error.code === 'PACKAGER_OPTION_INVALID');
  });

  it('flat-exe 校验文件头后原样写出', () => {
    const binaryFile = path.join(temp.dir, 'a.bin');
    const outputFile = path.join(temp.dir, 'A.EXE');
    writeFileSync(binaryFile, Buffer.from([0x4c, 0x05, 0x80, 0xaa, 0x55, 0x00, 0x41, 0x42, 0x60]));
    const result = getPackager('flat-exe')({ binaryFile, outputFile, options: { header: '4C????AA55', maxSize: 16 } });
    assert.equal(result.size, 9);
    assert.deepEqual(readFileSync(outputFile), readFileSync(binaryFile));
  });

  it('flat-exe 文件头不符时报错并给出实际内容', () => {
    const binaryFile = path.join(temp.dir, 'a.bin');
    writeFileSync(binaryFile, Buffer.from([0x60, 0x00, 0x00, 0x00, 0x00]));
    assert.throws(
      () => getPackager('flat-exe')({ binaryFile, outputFile: path.join(temp.dir, 'o'), options: { header: '4C????AA55' } }),
      (error) => error.code === 'PACKAGE_HEADER_MISMATCH' && error.message.includes('6000000000'),
    );
  });

  it('flat-exe 超过大小上限时报错', () => {
    const binaryFile = path.join(temp.dir, 'a.bin');
    writeFileSync(binaryFile, Buffer.alloc(17));
    assert.throws(() => getPackager('flat-exe')({ binaryFile, outputFile: path.join(temp.dir, 'o'), options: { maxSize: 16 } }), (error) => error.code === 'PACKAGE_TOO_LARGE');
  });

  it('ines 接受文件头与长度一致的文件', () => {
    const binaryFile = path.join(temp.dir, 'a.bin');
    const header = Buffer.from([0x4e, 0x45, 0x53, 0x1a, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
    writeFileSync(binaryFile, Buffer.concat([header, Buffer.alloc(0x4000 + 0x2000)]));
    const result = getPackager('ines')({ binaryFile, outputFile: path.join(temp.dir, 'a.nes'), options: {} });
    assert.equal(result.size, 16 + 0x6000);
  });

  it('ines 拒绝缺少 NES 文件头或长度不符的文件', () => {
    const binaryFile = path.join(temp.dir, 'a.bin');
    writeFileSync(binaryFile, Buffer.alloc(32));
    assert.throws(() => getPackager('ines')({ binaryFile, outputFile: path.join(temp.dir, 'a.nes'), options: {} }), (error) => error.code === 'PACKAGE_HEADER_MISMATCH');
    writeFileSync(binaryFile, Buffer.from([0x4e, 0x45, 0x53, 0x1a, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
    assert.throws(() => getPackager('ines')({ binaryFile, outputFile: path.join(temp.dir, 'a.nes'), options: {} }), (error) => error.code === 'PACKAGE_SIZE_MISMATCH');
  });
});
