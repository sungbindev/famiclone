// ------------------------------------------------------------------------
// 名称：yuxing-build.test.ts
// 说明：测试裕兴目标：用模板里的示例程序构建，文件最前面是 JMP，没有签名，大小不超过 8192 字节，
//       磁盘镜像里有程序，超过上限时报错。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-23
// 备注：用真实 cc65，本机没有组装工具链时整体跳过。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { listFat12 } from '../../../src/shared/packaging/fat12-image';
import { EXTENSION_DIRECTORY, HAS_TOOLCHAIN, createRealBuildEnvironment, createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

/** 真实构建较慢，单个用例的超时时间。 */
const TIMEOUT_MS = 120_000;

/** DOS 把文件装在这个地址并从这里开始执行。 */
const LOAD_ADDRESS = 0x6000;

/** 模板里的示例程序。 */
function templateSource(): string {
  return readFileSync(path.join(EXTENSION_DIRECTORY, 'templates', 'yuxing-asm', 'src', 'hello.s'), 'utf8');
}

describe.skipIf(!HAS_TOOLCHAIN)('裕兴目标（需要 cc65）', { timeout: TIMEOUT_MS }, () => {
  const { service } = createRealBuildEnvironment();
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'yuxing', name: 'hello' }));
  });

  afterEach(() => temp.cleanup());

  it('示例程序：文件最前面是 JMP 到 $6000 起的地址，大小不超过 8192 字节', async () => {
    writeTestFile(temp.dir, 'src/hello.s', templateSource());
    const { result } = await service.build({ pathArg: 'src', cwd: temp.dir });
    expect(path.basename(result.outputFile)).toBe('HELLO.EXE');
    const image = readFileSync(result.outputFile);
    expect(image[0]).toBe(0x4c);
    const entry = image[1]! | (image[2]! << 8);
    expect(entry).toBeGreaterThanOrEqual(LOAD_ADDRESS + 3);
    expect(entry).toBeLessThan(LOAD_ADDRESS + image.length);
    expect(image.length).toBeLessThanOrEqual(8192);
  });

  it('默认同时生成磁盘镜像，里面有程序与项目 disk 目录下的文件', async () => {
    writeTestFile(temp.dir, 'src/hello.s', templateSource());
    writeTestFile(temp.dir, 'disk/README.TXT', '说明');
    const { disk } = await service.build({ pathArg: 'src', cwd: temp.dir });
    expect(listFat12(readFileSync(disk!.imageFile)).map((entry) => entry.name)).toEqual(['HELLO.EXE', 'README.TXT']);
  });

  it('程序超过 8192 字节时报错', async () => {
    writeTestFile(temp.dir, 'src/hello.s', `${templateSource()}\n        .segment "RODATA"\n        .res 9000, $EA\n`);
    await expect(service.build({ pathArg: 'src', cwd: temp.dir })).rejects.toThrow();
  });
});
