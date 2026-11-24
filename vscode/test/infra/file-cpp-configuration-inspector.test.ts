// ------------------------------------------------------------------------
// 名称：file-cpp-configuration-inspector.test.ts
// 说明：测试 C/C++ 配置检查器：在临时目录里放不同内容的 c_cpp_properties.json，检查判断结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：用真实文件系统，每个用例用独立的临时目录，结束后删除。
// ------------------------------------------------------------------------

import { mkdir, mkdtemp, rm, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { FileCppConfigurationInspector } from '../../src/infra/c-intellisense/file-cpp-configuration-inspector';

let root = '';

beforeEach(async () => {
  root = await mkdtemp(path.join(tmpdir(), 'fcdev-inspector-'));
});

afterEach(async () => {
  await rm(root, { recursive: true, force: true });
});

/** 写入项目的配置文件。 */
async function writeProperties(content: string): Promise<void> {
  await mkdir(path.join(root, '.vscode'), { recursive: true });
  await writeFile(path.join(root, '.vscode', 'c_cpp_properties.json'), content);
}

describe('hasOtherConfigurations', () => {
  it('文件不存在：没有其他配置', async () => {
    expect(await new FileCppConfigurationInspector().hasOtherConfigurations(root)).toBe(false);
  });

  it('只有 FC Dev 配置：没有其他配置', async () => {
    await writeProperties(JSON.stringify({ configurations: [{ name: 'fcdev' }] }));
    expect(await new FileCppConfigurationInspector().hasOtherConfigurations(root)).toBe(false);
  });

  it('还有其他名字的配置：有其他配置', async () => {
    await writeProperties(JSON.stringify({ configurations: [{ name: 'Win32' }, { name: 'fcdev' }] }));
    expect(await new FileCppConfigurationInspector().hasOtherConfigurations(root)).toBe(true);
  });

  it('文件内容无法解析：按没有其他配置处理', async () => {
    await writeProperties('{ 坏了');
    expect(await new FileCppConfigurationInspector().hasOtherConfigurations(root)).toBe(false);
  });

  it('路径不是文件时读取失败，错误原样抛出', async () => {
    await mkdir(path.join(root, '.vscode', 'c_cpp_properties.json'), { recursive: true });
    await expect(new FileCppConfigurationInspector().hasOtherConfigurations(root)).rejects.toThrow();
  });
});
