// ------------------------------------------------------------------------
// 名称：file-project-config-reader.test.ts
// 说明：测试项目配置的读取：从子目录向上查找最近的 fcdev.json，没有配置文件时的默认值。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：配置内容的校验见 test/shared/project/project-config.test.ts。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { afterEach, beforeEach, describe, expect, it } from 'vitest';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';
import { FileProjectConfigReader } from '../../../src/logic/project/file-project-config-reader';
import { createTempDir, writeTestFile, type TempDir } from '../../support/build-fixtures';

describe('项目配置的读取', () => {
  const reader = new FileProjectConfigReader(new NodeFileSystem());
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('从子目录向上找到最近的 fcdev.json', () => {
    const file = writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'src/deep/a.c', '');
    const config = reader.read(path.join(temp.dir, 'src', 'deep'), temp.dir);
    expect(config.file).toBe(file);
    expect(config.root).toBe(temp.dir);
    expect(config.target).toBe('nes');
  });

  it('没有配置文件时返回默认值，并以给定目录为项目根', () => {
    const config = reader.read(temp.dir, temp.dir);
    expect(config.file).toBeNull();
    expect(config.root).toBe(path.resolve(temp.dir));
    expect(config.outDir).toBe(path.join(path.resolve(temp.dir), 'build'));
  });

  it('配置文件不是合法的 JSON 时报错', () => {
    writeTestFile(temp.dir, 'fcdev.json', '{');
    expect(() => reader.read(temp.dir, temp.dir)).toThrow(expect.objectContaining({ code: 'JSON_INVALID' }));
  });
});
