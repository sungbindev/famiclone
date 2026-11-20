// ------------------------------------------------------------------------
// 名称：build-ember.test.mjs
// 说明：测试模拟器包的组装（tools/build-ember.mjs）：复制程序、许可与第三方声明，清单记录 Ember 版本与 SHA-256，package.json 的字段，缺少来源文件时的报错。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：用临时目录里的假文件，不依赖 Ember 的真实构建产物。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { assembleEmberPackage, EMBER_PACKAGE_FILES } from '../tools/build-ember.mjs';

describe('组装模拟器包', () => {
  let root;
  let input;

  beforeEach(() => {
    root = mkdtempSync(path.join(tmpdir(), 'fcdev-build-ember-'));
    writeFileSync(path.join(root, 'Ember-1.2.3.exe'), 'exe-bytes');
    writeFileSync(path.join(root, 'LICENSE'), 'license-text');
    writeFileSync(path.join(root, 'THIRD_PARTY_NOTICES.txt'), 'notices-text');
    mkdirSync(path.join(root, 'package'));
    input = {
      executable: path.join(root, 'Ember-1.2.3.exe'),
      license: path.join(root, 'LICENSE'),
      notices: path.join(root, 'THIRD_PARTY_NOTICES.txt'),
      emberVersion: '1.2.3',
      fcdevVersion: '0.9.0',
      packageDir: path.join(root, 'package'),
    };
  });

  afterEach(() => rmSync(root, { recursive: true, force: true }));

  it('复制程序、许可与第三方声明，包里的程序叫 bin/Ember.exe', () => {
    assembleEmberPackage(input);
    assert.equal(readFileSync(path.join(input.packageDir, 'bin', 'Ember.exe'), 'utf8'), 'exe-bytes');
    assert.equal(readFileSync(path.join(input.packageDir, 'LICENSE'), 'utf8'), 'license-text');
    assert.equal(readFileSync(path.join(input.packageDir, 'THIRD_PARTY_NOTICES.txt'), 'utf8'), 'notices-text');
  });

  it('清单记录 Ember 的版本与每个文件的 SHA-256', () => {
    assembleEmberPackage(input);
    const manifest = JSON.parse(readFileSync(path.join(input.packageDir, 'manifest.json'), 'utf8'));
    assert.equal(manifest.emberVersion, '1.2.3');
    assert.deepEqual(Object.keys(manifest.files), EMBER_PACKAGE_FILES);
    assert.equal(manifest.files['bin/Ember.exe'], createHash('sha256').update('exe-bytes').digest('hex'));
  });

  it('package.json 的版本取 fcdev 的版本，只限 Windows 64 位，包含生成物', () => {
    assembleEmberPackage(input);
    const document = JSON.parse(readFileSync(path.join(input.packageDir, 'package.json'), 'utf8'));
    assert.equal(document.name, 'fcdev-ember');
    assert.equal(document.version, '0.9.0');
    assert.deepEqual(document.os, ['win32']);
    assert.deepEqual(document.cpu, ['x64']);
    assert.equal(document.license, 'GPL-3.0-only');
    assert.deepEqual(document.files, ['bin', 'LICENSE', 'THIRD_PARTY_NOTICES.txt', 'manifest.json']);
  });

  it('重新组装时换掉旧的程序', () => {
    assembleEmberPackage(input);
    writeFileSync(input.executable, 'new-exe');
    assembleEmberPackage(input);
    assert.equal(readFileSync(path.join(input.packageDir, 'bin', 'Ember.exe'), 'utf8'), 'new-exe');
  });

  it('缺少程序、许可或第三方声明时报错并提示怎么生成，不写出任何文件', () => {
    for (const key of ['executable', 'license', 'notices']) {
      assert.throws(() => assembleEmberPackage({ ...input, [key]: path.join(root, 'missing') }), (error) => error.code === 'EMBER_ARTIFACT_MISSING' && error.hint !== undefined);
    }
    assert.equal(existsSync(path.join(input.packageDir, 'manifest.json')), false);
  });
});
