/*
 * ------------------------------------------------------------------------
 * 名称：launch-args.test.ts
 * 说明：启动参数的测试：取 --cartridge=路径，只接受 .nes 文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { cartridgePathFromArgv, debugOptionsFromArgv } from './launch-args';

describe('debugOptionsFromArgv', () => {
  it('取 --debug-port 与 --debug-token 的值，等号和空格两种写法都行', () => {
    expect(debugOptionsFromArgv(['electron', '.', '--debug-port=5688', '--debug-token=abc.DEF-123'])).toEqual({ port: 5688, token: 'abc.DEF-123' });
    expect(debugOptionsFromArgv(['--debug-port', '5688', '--debug-token', 'secret'])).toEqual({ port: 5688, token: 'secret' });
  });

  it('没有参数时两项都没有', () => {
    expect(debugOptionsFromArgv([])).toEqual({});
    expect(debugOptionsFromArgv(['--remote-debugging-port=9301'])).toEqual({});
  });

  it('端口必须是 1 到 65535 的整数，其他写法忽略', () => {
    for (const port of ['0', '65536', '-1', '5688.5', 'abc', '', '1e3', '0x10', '123456']) expect(debugOptionsFromArgv([`--debug-port=${port}`])).toEqual({});
    expect(debugOptionsFromArgv(['--debug-port=1'])).toEqual({ port: 1 });
    expect(debugOptionsFromArgv(['--debug-port=65535'])).toEqual({ port: 65_535 });
    expect(debugOptionsFromArgv(['--debug-port'])).toEqual({});
  });

  it('令牌必须是 1 到 128 个可见的 ASCII 字符', () => {
    expect(debugOptionsFromArgv(['--debug-token='])).toEqual({});
    expect(debugOptionsFromArgv(['--debug-token=带中文'])).toEqual({});
    expect(debugOptionsFromArgv(['--debug-token=has space'])).toEqual({});
    expect(debugOptionsFromArgv([`--debug-token=${'a'.repeat(129)}`])).toEqual({});
    expect(debugOptionsFromArgv([`--debug-token=${'a'.repeat(128)}`])).toEqual({ token: 'a'.repeat(128) });
  });

  it('端口与令牌互不影响', () => {
    expect(debugOptionsFromArgv(['--debug-port=bad', '--debug-token=ok'])).toEqual({ token: 'ok' });
    expect(debugOptionsFromArgv(['--debug-port=5688', '--debug-token=bad token'])).toEqual({ port: 5688 });
  });
});

describe('cartridgePathFromArgv', () => {
  it('取出 --cartridge= 后面的 .nes 路径，扩展名不分大小写', () => {
    expect(cartridgePathFromArgv(['electron', '.', '--cartridge=D:/roms/idle.nes'])).toBe('D:/roms/idle.nes');
    expect(cartridgePathFromArgv(['--cartridge=D:\\超级 卡带\\a.NES'])).toBe('D:\\超级 卡带\\a.NES');
  });

  it('没有参数时为 undefined', () => {
    expect(cartridgePathFromArgv([])).toBeUndefined();
    expect(cartridgePathFromArgv(['--remote-debugging-port=9301'])).toBeUndefined();
  });

  it('不是 .nes 文件时忽略', () => {
    expect(cartridgePathFromArgv(['--cartridge=D:/tools/run.exe'])).toBeUndefined();
    expect(cartridgePathFromArgv(['--cartridge='])).toBeUndefined();
    expect(cartridgePathFromArgv(['--cartridge=D:/roms/idle.nes.exe'])).toBeUndefined();
  });

  it('只看带前缀的参数', () => {
    expect(cartridgePathFromArgv(['D:/roms/idle.nes'])).toBeUndefined();
    expect(cartridgePathFromArgv(['--cartridge', 'D:/roms/idle.nes'])).toBeUndefined();
  });
});
