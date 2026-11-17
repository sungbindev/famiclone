/*
 * ------------------------------------------------------------------------
 * 名称：publish-update.test.ts
 * 说明：升级文件发布脚本的测试：RELEASES 的解析（含 BOM 与 CRLF）、检查通过时 .nupkg 在前 RELEASES 在最后、文件缺失、大小或校验值不一致、缺当前版本的完整包、文件名带路径等错误。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：只测纯函数，不运行 ossutil，不访问网络。
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { describe, expect, it } from 'vitest';
import { parseReleases, planPublish } from '../tools/build/publish-update.mjs';

const SHA1 = 'a'.repeat(40);
const FULL = 'ember-0.2.0-full.nupkg';

function setup(releases = `${SHA1} ${FULL} 1000\r\n`, files: Record<string, { size: number; sha1: string }> = { [FULL]: { size: 1000, sha1: SHA1 } }) {
  return { version: '0.2.0', releases, files: new Map(Object.entries(files)) };
}

describe('parseReleases', () => {
  it('解析每一行的校验值、文件名与大小，忽略空行、BOM 与 CRLF，校验值转成小写', () => {
    const text = `\uFEFF${'A'.repeat(40)} ember-0.1.0-full.nupkg 10\r\n\r\n${'b'.repeat(40)} ember-0.2.0-full.nupkg 20\r\n`;
    expect(parseReleases(text)).toEqual([
      { sha1: 'a'.repeat(40), file: 'ember-0.1.0-full.nupkg', size: 10 },
      { sha1: 'b'.repeat(40), file: 'ember-0.2.0-full.nupkg', size: 20 }
    ]);
  });

  it('无法识别的行报错', () => {
    expect(() => parseReleases('乱七八糟')).toThrow('RELEASES 里有无法识别的行');
  });
});

describe('planPublish', () => {
  it('检查通过时先传 .nupkg，最后传 RELEASES，路径在升级目录下', () => {
    expect(planPublish(setup()).uploads).toEqual([
      { file: FULL, key: `updates/ember/windows/${FULL}` },
      { file: 'RELEASES', key: 'updates/ember/windows/RELEASES' }
    ]);
  });

  it('RELEASES 是空的时报错', () => {
    expect(() => planPublish(setup('\r\n'))).toThrow('RELEASES 是空的');
  });

  it('RELEASES 列出的文件不在目录里时报错', () => {
    expect(() => planPublish(setup(undefined, {}))).toThrow(`${FULL} 不在目录里`);
  });

  it('大小或校验值与 RELEASES 不一致时报错', () => {
    expect(() => planPublish(setup(undefined, { [FULL]: { size: 999, sha1: SHA1 } }))).toThrow('大小与 RELEASES 不一致');
    expect(() => planPublish(setup(undefined, { [FULL]: { size: 1000, sha1: 'b'.repeat(40) } }))).toThrow('校验值与 RELEASES 不一致');
  });

  it('没有当前版本的完整包时报错', () => {
    const other = 'ember-0.1.0-full.nupkg';
    expect(() => planPublish(setup(`${SHA1} ${other} 1000\r\n`, { [other]: { size: 1000, sha1: SHA1 } }))).toThrow(`没有当前版本的完整包 ${FULL}`);
  });

  it('文件名带路径或叫 RELEASES 时报错', () => {
    expect(() => planPublish(setup(`${SHA1} ../evil.nupkg 1000\r\n`))).toThrow('文件名不合格');
    expect(() => planPublish(setup(`${SHA1} RELEASES 1000\r\n`))).toThrow('文件名不合格');
  });
});
