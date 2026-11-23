// ------------------------------------------------------------------------
// 名称：file-target-repository.test.ts
// 说明：测试目标仓库：扩展自带的目标都能读取，目标名不能逃出目标目录，找不到时列出可用目标。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：无
// ------------------------------------------------------------------------

import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { createExtensionResources } from '../../../src/infra/resources/extension-resources-factory';
import { FileTargetRepository } from '../../../src/logic/targets/file-target-repository';
import { EXTENSION_DIRECTORY, createTempDir, fakeResources, writeTestFile, type TempDir } from '../../support/build-fixtures';

describe('扩展自带的目标', () => {
  const repository = new FileTargetRepository(new NodeFileSystem(), createExtensionResources(EXTENSION_DIRECTORY));

  it('nes 与 sb2000 都能读取', () => {
    expect(repository.listNames()).toEqual(expect.arrayContaining(['nes', 'sb2000']));
    expect(repository.load('nes').cc65Target).toBe('nes');
    expect(repository.load('sb2000').nameRule).toBe('dos-8.3');
    expect(repository.load('sb2000').run?.mode).toBe('firmware-disk');
  });

  it('rom 目标：整机 ROM 工程用 rom-overlay 打包，运行方案是 firmware，机型、偏移与大小是目标专用选项', () => {
    const rom = repository.load('rom');
    expect(rom).toMatchObject({ packager: 'rom-overlay', extension: '.bin', nameRule: null, disk: null });
    expect(rom.run).toEqual({ emulator: 'ember', mode: 'firmware', machine: null });
    expect(Object.keys(rom.options).toSorted()).toEqual(['machine', 'offset', 'size']);
  });

  it('sb2000 支持多页程序与 gb2312 源文件，nes 都不支持', () => {
    expect(repository.load('sb2000').banked).toMatchObject({ option: 'banks', maxBanks: 113 });
    expect(repository.load('sb2000').sourceCharset).toBe('gb2312');
    expect(repository.load('nes').banked).toBeNull();
    expect(repository.load('nes').sourceCharset).toBeNull();
  });

  it('不存在的目标报错并列出可用目标', () => {
    expect(() => repository.load('nope')).toThrow(expect.objectContaining({ code: 'TARGET_UNKNOWN', hint: expect.stringContaining('nes') }));
  });

  it('目标名不合法时不会逃出目标目录', () => {
    expect(() => repository.load('../x')).toThrow(expect.objectContaining({ code: 'TARGET_UNKNOWN' }));
  });
});

describe('临时目录里的目标', () => {
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('target.json 内容有误时报错', () => {
    writeTestFile(temp.dir, 'fake/target.json', JSON.stringify({ name: 'fake' }));
    const repository = new FileTargetRepository(new NodeFileSystem(), fakeResources(temp.dir));
    expect(() => repository.load('fake')).toThrow(/缺少字符串字段/);
  });
});
