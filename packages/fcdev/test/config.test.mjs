// ------------------------------------------------------------------------
// 名称：config.test.mjs
// 说明：测试项目配置：向上查找、未知项与类型错误报错、默认值、相对路径解析。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { findConfigFile, loadProjectConfig, normalizeConfig } from '../src/config/index.mjs';
import { createTempDir, writeTestFile } from './helpers.mjs';

describe('项目配置', () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('从子目录向上找到最近的 fcdev.json', () => {
    const file = writeTestFile(temp.dir, 'fcdev.json', '{}');
    writeTestFile(temp.dir, 'src/deep/a.c', '');
    assert.equal(findConfigFile(path.join(temp.dir, 'src', 'deep')), file);
  });

  it('没有配置文件时返回 null，并以终端目录为项目根', () => {
    const config = loadProjectConfig(temp.dir, temp.dir);
    assert.equal(config.file, null);
    assert.equal(config.root, path.resolve(temp.dir));
    assert.equal(config.outDir, path.join(path.resolve(temp.dir), 'build'));
  });

  it('补齐默认值并把相对路径换成绝对路径', () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes', include: ['inc'], out: 'dist', firmware: 'rom/bios.nes' }));
    const config = loadProjectConfig(temp.dir, temp.dir);
    assert.equal(config.target, 'nes');
    assert.equal(config.optimize, true);
    assert.deepEqual(config.include, [path.join(temp.dir, 'inc')]);
    assert.equal(config.outDir, path.join(temp.dir, 'dist'));
    assert.equal(config.firmware, path.join(temp.dir, 'rom', 'bios.nes'));
  });

  it('未知的配置项报错并列出可用项', () => {
    assert.throws(() => normalizeConfig({ targt: 'nes' }, null, temp.dir), (error) => error.code === 'CONFIG_UNKNOWN_KEY' && error.hint.includes('target'));
  });

  it('类型不对报错', () => {
    assert.throws(() => normalizeConfig({ optimize: 'yes' }, null, temp.dir), /类型不对/);
    assert.throws(() => normalizeConfig({ include: [1] }, null, temp.dir), /类型不对/);
  });

  it('define 的值只能是字符串或数字', () => {
    assert.throws(() => normalizeConfig({ define: { A: true } }, null, temp.dir), /define\.A/);
  });

  it('内容不是对象报错', () => {
    assert.throws(() => normalizeConfig([], null, temp.dir), /必须是对象/);
  });
});
