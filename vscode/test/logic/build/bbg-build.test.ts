// ------------------------------------------------------------------------
// 名称：bbg-build.test.ts
// 说明：测试步步高目标：用模板里的示例程序构建，文件前 2 个字节是装入地址 $8000，第一条指令是
//       JMP，大小不超过 16386 字节；有 diskImage 时磁盘镜像建在它上面，没有时只构建程序，
//       超过上限时报错。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-23
// 备注：用真实 cc65，本机没有组装工具链时整体跳过。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { buildFat12Image, listFat12, readFat12File } from '../../../src/shared/packaging/fat12-image';
import { EXTENSION_DIRECTORY, HAS_TOOLCHAIN, createRealBuildEnvironment, createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

/** 真实构建较慢，单个用例的超时时间。 */
const TIMEOUT_MS = 120_000;

/** DOS 把文件装在这个地址并从这里开始执行。 */
const LOAD_ADDRESS = 0x8000;

/** 模板里的示例程序。 */
function templateSource(): string {
  return readFileSync(path.join(EXTENSION_DIRECTORY, 'templates', 'bbg-asm', 'src', 'hello.s'), 'utf8');
}

describe.skipIf(!HAS_TOOLCHAIN)('步步高目标（需要 cc65）', { timeout: TIMEOUT_MS }, () => {
  const { service } = createRealBuildEnvironment();
  let temp: TempDir;

  /** 写项目配置。 */
  function writeConfig(extra: Record<string, unknown> = {}): void {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'bbg', name: 'hello', ...extra }));
  }

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'src/hello.s', templateSource());
  });

  afterEach(() => temp.cleanup());

  it('示例程序：前 2 个字节是装入地址 $8000，随后是 JMP，大小不超过 16386 字节', async () => {
    writeConfig();
    const { result } = await service.build({ pathArg: 'src', cwd: temp.dir });
    expect(path.basename(result.outputFile)).toBe('HELLO.CMD');
    const image = readFileSync(result.outputFile);
    expect([image[0], image[1]]).toEqual([0x00, 0x80]);
    expect(image[2]).toBe(0x4c);
    const entry = image[3]! | (image[4]! << 8);
    expect(entry).toBeGreaterThanOrEqual(LOAD_ADDRESS + 3);
    expect(entry).toBeLessThan(LOAD_ADDRESS + image.length - 2);
    expect(image.length).toBeLessThanOrEqual(16386);
  });

  it('没有 diskImage 时只构建程序，不生成镜像', async () => {
    writeConfig();
    const outcome = await service.build({ pathArg: 'src', cwd: temp.dir });
    expect(outcome.disk).toBeUndefined();
    expect(() => service.buildDisk(outcome)).toThrow(expect.objectContaining({ code: 'DISK_BASE_MISSING', hint: expect.stringContaining('diskImage') }));
  });

  it('有 diskImage 时镜像建在它上面：保留系统文件，加入程序与 disk 目录下的文件', async () => {
    const system = Buffer.alloc(3000, 0x53);
    const base = buildFat12Image([{ name: 'BBGDOS.SYS', data: system }], { label: 'BBGSYS' });
    writeTestFile(temp.dir, 'system.img', base);
    writeTestFile(temp.dir, 'disk/DATA.BIN', Uint8Array.from([1, 2, 3]));
    writeConfig({ diskImage: 'system.img' });
    const { disk, result } = await service.build({ pathArg: 'src', cwd: temp.dir });
    expect(disk).toMatchObject({ imageFile: path.join(temp.dir, 'build', 'bbg', 'HELLO.img'), files: ['HELLO.CMD', 'DATA.BIN'] });
    const image = readFileSync(disk!.imageFile);
    const entries = listFat12(image);
    expect(entries.map((entry) => entry.name)).toEqual(['BBGDOS.SYS', 'HELLO.CMD', 'DATA.BIN']);
    expect(readFat12File(image, entries[0]!)).toEqual(system);
    expect(readFat12File(image, entries[1]!)).toEqual(readFileSync(result.outputFile));
    expect(readFileSync(path.join(temp.dir, 'system.img'))).toEqual(base);
  });

  it('diskImage 指向的文件不存在时报错', async () => {
    writeConfig({ diskImage: 'missing.img' });
    await expect(service.build({ pathArg: 'src', cwd: temp.dir })).rejects.toMatchObject({ code: 'DISK_BASE_MISSING' });
  });

  it('程序超过 16386 字节时报错', async () => {
    writeConfig();
    writeTestFile(temp.dir, 'src/hello.s', `${templateSource()}\n        .segment "RODATA"\n        .res 17000, $EA\n`);
    await expect(service.build({ pathArg: 'src', cwd: temp.dir })).rejects.toThrow();
  });
});
