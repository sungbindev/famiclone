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
import { cartridgePathFromArgv } from './launch-args';

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
