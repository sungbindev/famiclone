// ------------------------------------------------------------------------
// 名称：cec1-build.test.ts
// 说明：测试中华学习机 CEC-I 目标：用模板里的示例程序构建，
//       产物是 DOS 3.3 二进制文件（4 字节文件头：装入地址与长度，第一条指令在装入地址上）；
//       装入地址默认 $6000，可以用 targetOptions.loadAddress 改，超出范围报错；
//       有 diskImage 时磁盘镜像建在它上面，没有时只构建程序；程序名不合规时报错。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-26
// 备注：用真实 cc65，本机没有组装工具链时整体跳过。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { listDos33, readDos33File } from '../../../src/shared/packaging/dos33-image';
import { EXTENSION_DIRECTORY, HAS_TOOLCHAIN, createRealBuildEnvironment, createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';
import { createBlankDos33Image } from '../../support/dos33-fixtures';

/** 真实构建较慢，单个用例的超时时间。 */
const TIMEOUT_MS = 120_000;

/** 模板里的示例程序。 */
function templateSource(): string {
  return readFileSync(path.join(EXTENSION_DIRECTORY, 'templates', 'cec1-asm', 'src', 'hello.s'), 'utf8');
}

describe.skipIf(!HAS_TOOLCHAIN)('中华学习机 CEC-I 目标（需要 cc65）', { timeout: TIMEOUT_MS }, () => {
  const { service } = createRealBuildEnvironment();
  let temp: TempDir;

  /** 写项目配置。 */
  function writeConfig(extra: Record<string, unknown> = {}): void {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'cec1', name: 'hello', ...extra }));
  }

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'src/hello.s', templateSource());
  });

  afterEach(() => temp.cleanup());

  it('示例程序：产物没有扩展名，文件头是 $6000 与长度，第一条指令是 CLD，在 HIMEM 之前', async () => {
    writeConfig();
    const { result } = await service.build({ pathArg: 'src', cwd: temp.dir });
    expect(path.basename(result.outputFile)).toBe('HELLO');
    const image = readFileSync(result.outputFile);
    expect([image[0], image[1]]).toEqual([0x00, 0x60]);
    expect(image.readUInt16LE(2)).toBe(image.length - 4);
    expect(image[4]).toBe(0xd8);
    expect(0x6000 + image.length - 4).toBeLessThanOrEqual(0x9200);
  });

  it('装入地址用 targetOptions.loadAddress 修改，数字与 "$0800" 字符串都行，调用的子程序地址也跟着变', async () => {
    for (const loadAddress of [0x0800, '$0800', '0x0800']) {
      writeConfig({ targetOptions: { loadAddress } });
      const { result } = await service.build({ pathArg: 'src', cwd: temp.dir });
      const image = readFileSync(result.outputFile);
      expect([image[0], image[1]]).toEqual([0x00, 0x08]);
      // 第一条指令 CLD，第二条 JSR _main：目标地址在新的装入地址之后
      expect(image[5]).toBe(0x20);
      expect(image.readUInt16LE(6)).toBeGreaterThanOrEqual(0x0800);
      expect(image.readUInt16LE(6)).toBeLessThan(0x0800 + image.length);
    }
  });

  it('装入地址不在范围内或格式不对时报错', async () => {
    for (const loadAddress of [0x0100, 0x9000, 'abc', true]) {
      writeConfig({ targetOptions: { loadAddress } });
      await expect(service.build({ pathArg: 'src', cwd: temp.dir })).rejects.toMatchObject({ code: 'TARGET_OPTION_INVALID' });
    }
  });

  it('程序名不合规时报错', async () => {
    writeConfig({ name: '1bad' });
    await expect(service.build({ pathArg: 'src', cwd: temp.dir })).rejects.toMatchObject({ code: 'NAME_INVALID' });
  });

  it('有 diskName 但没有 diskImage 时只构建程序，不生成镜像', async () => {
    writeConfig({ diskName: 'cec1-hello.dsk' });
    const outcome = await service.build({ pathArg: 'src', cwd: temp.dir });
    expect(outcome.disk).toBeUndefined();
    expect(() => service.buildDisk(outcome)).toThrow(expect.objectContaining({ code: 'DISK_BASE_MISSING', hint: expect.stringContaining('diskImage') }));
  });

  it('有 diskImage 时镜像建在它上面：程序是二进制文件，disk 目录下的文件是文本文件，底盘不改', async () => {
    const base = createBlankDos33Image();
    writeTestFile(temp.dir, 'system.dsk', base);
    writeTestFile(temp.dir, 'disk/DATA', Uint8Array.from([1, 2, 3]));
    writeConfig({ diskImage: 'system.dsk', diskName: 'cec1-hello.dsk' });
    const { disk, result } = await service.build({ pathArg: 'src', cwd: temp.dir });
    expect(disk).toMatchObject({ imageFile: path.join(temp.dir, 'build', 'cec1', 'cec1-hello.dsk'), files: ['HELLO', 'DATA'] });
    const image = readFileSync(disk!.imageFile);
    expect(listDos33(image).map((entry) => [entry.name, entry.type])).toEqual([['HELLO', 4], ['DATA', 0]]);
    expect(readDos33File(image, 'HELLO')).toEqual(readFileSync(result.outputFile));
    expect(readDos33File(image, 'DATA')?.subarray(0, 3)).toEqual(Buffer.from([1, 2, 3]));
    expect(readFileSync(path.join(temp.dir, 'system.dsk'))).toEqual(base);
  });

  it('diskImage 不是 DOS 3.3 镜像时报错', async () => {
    writeTestFile(temp.dir, 'system.dsk', Buffer.alloc(1440 * 1024));
    writeConfig({ diskImage: 'system.dsk', diskName: 'cec1-hello.dsk' });
    await expect(service.build({ pathArg: 'src', cwd: temp.dir })).rejects.toMatchObject({ code: 'DISK_BASE_INVALID' });
  });
});
