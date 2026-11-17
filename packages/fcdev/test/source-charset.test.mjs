// ------------------------------------------------------------------------
// 名称：source-charset.test.mjs
// 说明：测试源文件转码：UTF-8 转 GB2312 的规则与出错提示、目标定义里 sourceCharset 的校验，以及用真实 cc65 构建学习机程序时汉字被转成 GB2312 字节。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-18
// 备注：集成部分在本机没有 cc65 时跳过；每个用例使用独立的临时项目。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath } from '../src/build/index.mjs';
import { convertToGb2312 } from '../src/build/source-charset.mjs';
import { loadTarget, parseTarget } from '../src/targets/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** “你好”的 GB2312 内码 */
const NI_HAO = Buffer.from([0xc4, 0xe3, 0xba, 0xc3]);

/** “你好”的 UTF-8 编码 */
const NI_HAO_UTF8 = Buffer.from('你好', 'utf8');

describe('convertToGb2312', () => {
  it('汉字与全角标点转成两个字节，ASCII 不变，注释里的汉字一并转换', () => {
    assert.deepEqual([...convertToGb2312(Buffer.from('你好，小霸王！', 'utf8'), 'a.c')], [0xc4, 0xe3, 0xba, 0xc3, 0xa3, 0xac, 0xd0, 0xa1, 0xb0, 0xd4, 0xcd, 0xf5, 0xa3, 0xa1]);
    assert.equal(convertToGb2312(Buffer.from('/* 说明 */ "a\\n"\r\n', 'utf8'), 'a.c').toString('latin1').length, '/* 说明 */ "a\\n"\r\n'.length + 2);
  });

  it('开头的 BOM 被去掉', () => {
    assert.deepEqual([...convertToGb2312(Buffer.from('\uFEFFint x;', 'utf8'), 'a.c')], [...Buffer.from('int x;')]);
  });

  it('不是合法 UTF-8 的内容视为已经是 GB2312，原样返回', () => {
    const gb = Buffer.concat([Buffer.from('s = "'), NI_HAO, Buffer.from('";')]);
    assert.equal(convertToGb2312(gb, 'a.c'), gb);
  });

  it('GB2312 里没有的字符报错，写出文件、行、列与字符', () => {
    assert.throws(
      () => convertToGb2312(Buffer.from('int a;\n  s = "好😀";\n', 'utf8'), 'x/a.c'),
      (error) => error.code === 'SOURCE_CHAR_UNSUPPORTED' && error.message.startsWith('x/a.c:2:9：') && error.message.includes('U+1F600'),
    );
  });
});

describe('目标定义的 sourceCharset', () => {
  it('sb2000 声明了 gb2312，nes 没有声明', () => {
    assert.equal(loadTarget('sb2000').sourceCharset, 'gb2312');
    assert.equal(loadTarget('nes').sourceCharset, null);
  });

  it('不认识的编码报错', () => {
    const base = { name: 't', title: 't', cpu: '6502', cc65Target: 'none', linkConfig: 'l.cfg', packager: 'ines', extension: '.x' };
    assert.throws(() => parseTarget({ ...base, sourceCharset: 'big5' }, '/x/t'), (error) => error.code === 'TARGET_INVALID' && error.message.includes('sourceCharset'));
    assert.equal(parseTarget({ ...base, sourceCharset: 'gb2312' }, '/x/t').sourceCharset, 'gb2312');
  });
});

describe('用真实 cc65 构建含汉字的源文件', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  /** 构建并返回结果与工具输出 */
  async function build(pathArg) {
    const output = [];
    const reporter = { log() {}, write: (text) => output.push(text) };
    const { result } = await buildPath({ pathArg, cwd: temp.dir }, reporter);
    return { result, output: output.join('') };
  }

  it('学习机的 C 程序里直接写汉字，程序里是 GB2312 字节', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'hello' }));
    writeTestFile(temp.dir, 'src/hello.c', '#include <sb2000.h>\n/* 你好 */\nint main(void) { sb_puts("你好"); return 0; }\n');
    const { result } = await build('src');
    const image = readFileSync(result.outputFile);
    assert.ok(image.includes(NI_HAO), '应包含 GB2312 内码');
    assert.equal(image.includes(NI_HAO_UTF8), false, '不应包含 UTF-8 字节');
  });

  it('学习机的汇编程序里直接写汉字，程序里是 GB2312 字节', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'hello' }));
    writeTestFile(temp.dir, 'src/hello.s', '.export _main\n.segment "CODE"\n_main:\n        rts\n.segment "RODATA"\n        .byte "你好", 0\n');
    const { result } = await build('src/hello.s');
    const image = readFileSync(result.outputFile);
    assert.ok(image.includes(NI_HAO));
    assert.equal(image.includes(NI_HAO_UTF8), false);
  });

  it('没有声明编码的目标不转码', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes', name: 'game' }));
    writeTestFile(temp.dir, 'src/game.c', 'const char text[] = "你好";\nint main(void) { return text[0]; }\n');
    const { result } = await build('src/game.c');
    assert.ok(readFileSync(result.outputFile).includes(NI_HAO_UTF8));
  });

  it('C 编译错误指向原文件和原行号，不出现转码副本的名字', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'bad' }));
    writeTestFile(temp.dir, 'src/bad.c', '/* 你好 */\nint main(void) { return x; }\n');
    const output = [];
    const reporter = { log() {}, write: (text) => output.push(text) };
    await assert.rejects(buildPath({ pathArg: 'src/bad.c', cwd: temp.dir }, reporter), (error) => error.code === 'TOOL_FAILED');
    assert.match(output.join(''), /bad\.c:2: Error/);
    assert.doesNotMatch(output.join(''), /gb2312/);
  });

  it('汇编错误指向原文件和原行号，不出现转码副本的名字', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'bad' }));
    writeTestFile(temp.dir, 'src/bad.s', '; 你好\n.export _main\n_main:\n        bogus\n');
    const output = [];
    const reporter = { log() {}, write: (text) => output.push(text) };
    await assert.rejects(buildPath({ pathArg: 'src/bad.s', cwd: temp.dir }, reporter), (error) => error.code === 'TOOL_FAILED');
    assert.match(output.join(''), /bad\.s:4: Error/);
    assert.doesNotMatch(output.join(''), /gb2312/);
  });

  it('字符不在 GB2312 里时构建失败并指出位置', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'bad' }));
    writeTestFile(temp.dir, 'src/bad.c', 'int main(void) { return 0; }\n/* 😀 */\n');
    await assert.rejects(buildPath({ pathArg: 'src/bad.c', cwd: temp.dir }), (error) => error.code === 'SOURCE_CHAR_UNSUPPORTED' && error.message.includes(':2:4：'));
  });
});
