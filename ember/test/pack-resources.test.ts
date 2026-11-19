/*
 * ------------------------------------------------------------------------
 * 名称：pack-resources.test.ts
 * 说明：资源打包脚本的测试：把预装插件、帮助文档站点与许可全文打成一个 zip，条目名称与顺序固定、内容没有变化时不重写文件、某类资源目录不存在时跳过并报告。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：只用临时目录与 Node 自带的能力读回 zip 的条目名称，不依赖构建产物。
 * ------------------------------------------------------------------------
 */

import { mkdirSync, mkdtempSync, readFileSync, rmSync, statSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { afterEach, describe, expect, it } from 'vitest';
import { RESOURCE_ARCHIVE_FILE, packResources } from '../tools/build/pack-resources.mjs';

const directories: string[] = [];

afterEach(() => {
  for (const directory of directories.splice(0)) rmSync(directory, { recursive: true, force: true });
});

/** 临时目录。 */
function temp(): string {
  const directory = mkdtempSync(join(tmpdir(), 'ember-pack-resources-'));
  directories.push(directory);
  return directory;
}

/** 读出 zip 中央目录里的条目名称（只认本脚本写出的格式）。 */
function entryNames(zip: Buffer): string[] {
  const end = zip.lastIndexOf(Buffer.from([0x50, 0x4b, 0x05, 0x06]));
  const count = zip.readUInt16LE(end + 10);
  let offset = zip.readUInt32LE(end + 16);
  const names: string[] = [];
  for (let index = 0; index < count; index += 1) {
    const nameLength = zip.readUInt16LE(offset + 28);
    const extraLength = zip.readUInt16LE(offset + 30);
    const commentLength = zip.readUInt16LE(offset + 32);
    names.push(zip.subarray(offset + 46, offset + 46 + nameLength).toString('utf8'));
    offset += 46 + nameLength + extraLength + commentLength;
  }
  return names;
}

/** 摆好一组资源目录。 */
function layout(root: string): { plugins: string; help: string; notices: string; out: string } {
  const plugins = join(root, 'plugins');
  const help = join(root, 'help');
  mkdirSync(join(help, 'manual'), { recursive: true });
  mkdirSync(plugins, { recursive: true });
  writeFileSync(join(plugins, 'b.emap'), 'B');
  writeFileSync(join(plugins, 'a.emap'), 'A');
  writeFileSync(join(plugins, 'ignore.txt'), 'x');
  writeFileSync(join(help, 'index.html'), '<h1>帮助</h1>');
  writeFileSync(join(help, 'manual', 'changelog.html'), '更新记录');
  const notices = join(root, 'NOTICES.txt');
  writeFileSync(notices, '许可');
  return { plugins, help, notices, out: join(root, 'out') };
}

describe('打包资源', () => {
  it('条目是插件、帮助文档、许可全文，按路径排序，只认 .emap', () => {
    const root = temp();
    const dirs = layout(root);
    const result = packResources({ outDirectory: dirs.out, pluginsDirectory: dirs.plugins, helpDirectory: dirs.help, noticesFile: dirs.notices });
    expect(result.skipped).toEqual([]);
    expect(result.file).toBe(join(dirs.out, RESOURCE_ARCHIVE_FILE));
    expect(entryNames(readFileSync(result.file))).toEqual(['plugins/a.emap', 'plugins/b.emap', 'help/index.html', 'help/manual/changelog.html', 'THIRD_PARTY_NOTICES.txt']);
    expect(result.entries).toBe(5);
  });

  it('内容没有变化时不重写文件', () => {
    const root = temp();
    const dirs = layout(root);
    const options = { outDirectory: dirs.out, pluginsDirectory: dirs.plugins, helpDirectory: dirs.help, noticesFile: dirs.notices };
    const first = packResources(options);
    const before = statSync(first.file).mtimeMs;
    const content = readFileSync(first.file);
    const second = packResources(options);
    expect(readFileSync(second.file).equals(content)).toBe(true);
    expect(statSync(second.file).mtimeMs).toBe(before);
  });

  it('某类资源的目录不存在时跳过并说明', () => {
    const root = temp();
    const dirs = layout(root);
    const result = packResources({ outDirectory: dirs.out, pluginsDirectory: join(root, 'missing'), helpDirectory: join(root, 'missing-help'), noticesFile: join(root, 'missing.txt') });
    expect(result.skipped).toEqual(['预装插件', '帮助文档', '第三方许可全文']);
    expect(result.entries).toBe(0);
  });
});
