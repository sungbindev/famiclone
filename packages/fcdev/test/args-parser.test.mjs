// ------------------------------------------------------------------------
// 名称：args-parser.test.mjs
// 说明：测试命令行参数解析：命令、位置参数、选项、未知选项、debug 原样传参。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { describe, it } from 'node:test';
import { parseArguments } from '../src/cli/args-parser.mjs';

describe('命令行参数解析', () => {
  it('命令后面的路径是位置参数', () => {
    const parsed = parseArguments(['build', 'src/hello.c']);
    assert.equal(parsed.command, 'build');
    assert.deepEqual(parsed.positionals, ['src/hello.c']);
  });

  it('没有参数时命令为 null', () => {
    assert.equal(parseArguments([]).command, null);
  });

  it('--target 支持空格和等号两种写法', () => {
    assert.equal(parseArguments(['build', '--target', 'nes']).options.target, 'nes');
    assert.equal(parseArguments(['build', '--target=sb2000']).options.target, 'sb2000');
  });

  it('--target 缺少值时报错', () => {
    assert.throws(() => parseArguments(['build', '--target']), /缺少目标名/);
  });

  it('未知选项报错', () => {
    assert.throws(() => parseArguments(['build', '--nope']), /未知的选项：--nope/);
  });

  it('--debug 与 --port 被解析，端口不合法时报错', () => {
    const parsed = parseArguments(['run', '--debug', '--port', '5688', 'src']);
    assert.equal(parsed.options.debug, true);
    assert.equal(parsed.options.port, 5688);
    assert.deepEqual(parsed.positionals, ['src']);
    assert.equal(parseArguments(['run', '--port=6000']).options.port, 6000);
    assert.throws(() => parseArguments(['run', '--port', 'abc']), /--port/);
    assert.throws(() => parseArguments(['run', '--port', '70000']), /--port/);
    assert.throws(() => parseArguments(['run', '--port']), /--port/);
  });

  it('debug 命令之后的参数原样保留，包括以 - 开头的', () => {
    const parsed = parseArguments(['debug', 'read', 'cpu', '8000', '-1', '--json']);
    assert.equal(parsed.command, 'debug');
    assert.deepEqual(parsed.positionals, ['read', 'cpu', '8000', '-1', '--json']);
    assert.equal(parsed.options.json, false);
  });
});
