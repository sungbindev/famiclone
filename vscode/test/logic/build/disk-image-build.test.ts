// ------------------------------------------------------------------------
// 名称：disk-image-build.test.ts
// 说明：测试构建时默认生成磁盘镜像：镜像里有程序与项目 disk 目录下的全部文件，配置 disk 为
//       false 时不生成，disk 目录里有子目录或与程序重名时报错，不支持磁盘镜像的目标没有镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-23
// 备注：用真实 cc65，本机没有组装工具链时整体跳过。
// ------------------------------------------------------------------------

import { existsSync, readFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { listFat12, readFat12File } from '../../../src/shared/packaging/fat12-image';
import { HAS_TOOLCHAIN, createRealBuildEnvironment, createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

/** 真实构建较慢，单个用例的超时时间。 */
const TIMEOUT_MS = 120_000;

/** 汇编程序：导出 _main，直接返回 DOS。 */
const HELLO_ASM = '.export _main\n.segment "CODE"\n_main:\n        rts\n';

describe.skipIf(!HAS_TOOLCHAIN)('构建时的磁盘镜像（需要 cc65）', { timeout: TIMEOUT_MS }, () => {
  const { service } = createRealBuildEnvironment();
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'src/hello.s', HELLO_ASM);
  });

  afterEach(() => temp.cleanup());

  /** 写项目配置。 */
  function writeConfig(extra: Record<string, unknown> = {}): void {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'hello', ...extra }));
  }

  /** 构建 src。 */
  function build(): Promise<Awaited<ReturnType<typeof service.build>>> {
    return service.build({ pathArg: 'src', cwd: temp.dir });
  }

  it('默认同时生成镜像：程序在最前，后面是 disk 目录下的全部文件，内容原样', async () => {
    writeConfig();
    writeTestFile(temp.dir, 'disk/README.TXT', '使用说明');
    writeTestFile(temp.dir, 'disk/data.bin', Uint8Array.from([1, 2, 3]));
    const outcome = await build();
    expect(outcome.disk).toMatchObject({ imageFile: path.join(temp.dir, 'build', 'sb2000', 'HELLO.img'), files: ['HELLO.EXE', 'data.bin', 'README.TXT'] });
    const image = readFileSync(outcome.disk!.imageFile);
    const entries = listFat12(image);
    expect(entries.map((entry) => entry.name)).toEqual(['HELLO.EXE', 'DATA.BIN', 'README.TXT']);
    expect(readFat12File(image, entries[0]!)).toEqual(readFileSync(outcome.result.outputFile));
    expect(readFat12File(image, entries[1]!)).toEqual(Buffer.from([1, 2, 3]));
    expect(readFat12File(image, entries[2]!).toString('utf8')).toBe('使用说明');
  });

  it('没有 disk 目录时镜像里只有程序', async () => {
    writeConfig();
    const outcome = await build();
    expect(outcome.disk?.files).toEqual(['HELLO.EXE']);
  });

  it('配置里 disk 为 false 时构建不生成镜像，之后单独生成仍可以', async () => {
    writeConfig({ disk: false });
    const outcome = await build();
    expect(outcome.disk).toBeUndefined();
    expect(existsSync(path.join(temp.dir, 'build', 'sb2000', 'HELLO.img'))).toBe(false);
    expect(existsSync(service.buildDisk(outcome).imageFile)).toBe(true);
  });

  it('disk 目录里有子目录时报错并说明只支持根目录', async () => {
    writeConfig();
    writeTestFile(temp.dir, 'disk/sub/a.txt', 'x');
    await expect(build()).rejects.toMatchObject({ code: 'DISK_DIRECTORY_INVALID', hint: expect.stringContaining('根目录') });
  });

  it('disk 目录里的文件与程序重名时报错', async () => {
    writeConfig();
    writeTestFile(temp.dir, 'disk/hello.exe', 'x');
    await expect(build()).rejects.toMatchObject({ code: 'DISK_NAME_CLASH' });
  });

  it('disk 目录里的文件名不符合 8.3 时报错', async () => {
    writeConfig();
    writeTestFile(temp.dir, 'disk/long-file-name.txt', 'x');
    await expect(build()).rejects.toMatchObject({ code: 'DISK_NAME_INVALID' });
  });

  it('不支持磁盘镜像的目标没有镜像，即使有 disk 目录', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'a.c', 'int main(void) { for (;;) { } }\n');
    writeTestFile(temp.dir, 'disk/README.TXT', 'x');
    const outcome = await service.build({ pathArg: 'a.c', cwd: temp.dir });
    expect(outcome.disk).toBeUndefined();
  });
});
