/*
 * ------------------------------------------------------------------------
 * 名称：publish-update.test.ts
 * 说明：升级文件发布脚本的测试：RELEASES 的解析（含 BOM 与 CRLF）、检查通过时 .nupkg 在前 RELEASES 在最后、文件缺失、大小或校验值不一致、缺当前版本的完整包、文件名带路径等错误，以及安装程序 zip 的文件名（带版本号）、上传顺序与打包内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-18
 * 备注：只测纯函数与临时目录里的 zip，不运行 ossutil，不访问网络。
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { inflateRawSync } from 'node:zlib';
import { describe, expect, it } from 'vitest';
import { buildInstallerZip, parseReleases, planPublish } from '../tools/build/publish-update.mjs';

const SHA1 = 'a'.repeat(40);
const FULL = 'ember-0.2.0-full.nupkg';
const SETUP = 'Ember-学习机模拟器.exe';

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

  it('给了安装程序时，它的 zip 排在 .nupkg 之后、RELEASES 之前，文件名带版本号、路径在升级目录下', () => {
    const input = setup(undefined, { [FULL]: { size: 1000, sha1: SHA1 }, [SETUP]: { size: 5, sha1: SHA1 } });
    expect(planPublish({ ...input, installer: SETUP }).uploads).toEqual([
      { file: FULL, key: `updates/ember/windows/${FULL}` },
      { file: 'Ember-Setup-0.2.0.zip', key: 'updates/ember/windows/Ember-Setup-0.2.0.zip', zipOf: SETUP },
      { file: 'RELEASES', key: 'updates/ember/windows/RELEASES' }
    ]);
  });

  it('不同版本的 zip 文件名不同，新版本不会覆盖以前上传的', () => {
    const names = ['0.2.0', '0.3.0'].map((version) => {
      const full = `ember-${version}-full.nupkg`;
      const files = new Map([[full, { size: 1000, sha1: SHA1 }], [SETUP, { size: 5, sha1: SHA1 }]]);
      return planPublish({ version, releases: `${SHA1} ${full} 1000\r\n`, files, installer: SETUP }).uploads.find((upload) => upload.zipOf !== undefined)?.file;
    });
    expect(names).toEqual(['Ember-Setup-0.2.0.zip', 'Ember-Setup-0.3.0.zip']);
  });

  it('指定的安装程序不在目录里时报错', () => {
    expect(() => planPublish({ ...setup(), installer: SETUP })).toThrow(`目录里没有安装程序 ${SETUP}`);
  });
});

/** 读出 zip 的第一个条目：文件名、UTF-8 标志与解压后的内容，并给出条目数。 */
function readFirstEntry(zip: Buffer) {
  expect(zip.readUInt32LE(0)).toBe(0x04034b50);
  const method = zip.readUInt16LE(8);
  const compressedSize = zip.readUInt32LE(18);
  const nameLength = zip.readUInt16LE(26);
  const extraLength = zip.readUInt16LE(28);
  const name = zip.subarray(30, 30 + nameLength).toString('utf8');
  const body = zip.subarray(30 + nameLength + extraLength, 30 + nameLength + extraLength + compressedSize);
  const eocd = zip.lastIndexOf(Buffer.from([0x50, 0x4b, 0x05, 0x06]));
  return { name, utf8: (zip.readUInt16LE(6) & 0x0800) !== 0, data: method === 8 ? inflateRawSync(body) : body, entries: zip.readUInt16LE(eocd + 10) };
}

describe('buildInstallerZip', () => {
  it('把安装程序打成只含一个文件的 zip，中文文件名用 UTF-8，解压后内容与原文件一致', () => {
    const directory = mkdtempSync(join(tmpdir(), 'ember-zip-test-'));
    try {
      const content = Buffer.from('MZ\u0090'.repeat(5000), 'latin1');
      writeFileSync(join(directory, SETUP), content);
      buildInstallerZip(join(directory, SETUP), join(directory, 'out.zip'));
      const entry = readFirstEntry(readFileSync(join(directory, 'out.zip')));
      expect(entry.name).toBe(SETUP);
      expect(entry.utf8).toBe(true);
      expect(entry.entries).toBe(1);
      expect(Buffer.compare(entry.data, content)).toBe(0);
    } finally {
      rmSync(directory, { recursive: true, force: true });
    }
  });
});

describe('planPublish 的错误检查', () => {
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
