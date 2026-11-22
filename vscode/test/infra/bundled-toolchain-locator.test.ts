// ------------------------------------------------------------------------
// 名称：bundled-toolchain-locator.test.ts
// 说明：测试工具链定位与运行：缺少工具时报错并给出办法，真实工具链能运行 cc65 读到版本。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用只含空文件的假目录测试定位逻辑，不运行假目录里的程序；真实工具链的用例在没有组装时跳过。
// ------------------------------------------------------------------------

import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { NodeFileSystem } from '../../src/infra/files/node-file-system';
import { createExtensionResources } from '../../src/infra/resources/extension-resources-factory';
import { BundledToolchainLocator } from '../../src/infra/toolchain/bundled-toolchain-locator';
import { ProcessToolRunner } from '../../src/infra/toolchain/process-tool-runner';
import { EXTENSION_DIRECTORY, HAS_TOOLCHAIN, createTempDir, fakeResources, writeTestFile, type TempDir } from '../support/build-fixtures';

describe('工具链定位', () => {
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  /** 以临时目录为工具链目录的定位器。 */
  function locatorFor(toolchainDirectory: string): BundledToolchainLocator {
    return new BundledToolchainLocator(new NodeFileSystem(), fakeResources(temp.dir, { toolchainDirectory }));
  }

  it('三个必需的程序都在时返回工具链根目录', () => {
    for (const tool of ['cc65', 'ca65', 'ld65']) writeTestFile(temp.dir, `bin/${tool}.exe`, '');
    expect(locatorFor(temp.dir).locate()).toEqual({ home: temp.dir });
  });

  it('缺少工具时报错并列出缺少的程序', () => {
    writeTestFile(temp.dir, 'bin/cc65.exe', '');
    expect(() => locatorFor(temp.dir).locate()).toThrow(
      expect.objectContaining({ code: 'TOOLCHAIN_INCOMPLETE', message: expect.stringContaining('ca65、ld65') })
    );
  });
});

describe.skipIf(!HAS_TOOLCHAIN)('真实工具链', () => {
  it('能运行 cc65 并读到版本', async () => {
    const resources = createExtensionResources(EXTENSION_DIRECTORY);
    const toolchain = new BundledToolchainLocator(new NodeFileSystem(), resources).locate();
    const result = await new ProcessToolRunner().run(toolchain, 'cc65', ['--version']);
    expect(`${result.stderr}${result.stdout}`.trim()).toMatch(/^cc65 V\d+\.\d+/);
  });
});
