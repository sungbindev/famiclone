/*
 * ------------------------------------------------------------------------
 * 名称：pack-plugins.test.ts
 * 说明：插件打包脚本的测试：生成的 zip 能被解析（条目名、压缩方式、校验值、内容），输出可重现，目录名与清单标识、多余文件、非法标识等错误，以及仓库里的官方插件能打包。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：打包在临时目录里进行，不改动仓库里的 build/plugins；zip 的解析在测试里用 node:zlib 手写，与核心里的 miniz 读取互相独立。
 * ------------------------------------------------------------------------
 */

// @vitest-environment node

import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { inflateRawSync } from 'node:zlib';
import { afterEach, describe, expect, it } from 'vitest';
import { createZip, packPlugin, packPlugins } from '../tools/build/pack-plugins.mjs';

const REPO_ROOT = fileURLToPath(new URL('..', import.meta.url));
const roots: string[] = [];

/** 创建临时项目目录。 */
function createRoot(): string {
  const root = mkdtempSync(join(tmpdir(), 'ember-pack-'));
  roots.push(root);
  return root;
}

/** 在临时项目里写一个插件目录。 */
function writePlugin(root: string, name: string, files: Record<string, string>): string {
  const directory = join(root, 'plugins', name);
  mkdirSync(directory, { recursive: true });
  for (const [file, content] of Object.entries(files)) writeFileSync(join(directory, file), content);
  return directory;
}

/** 一个解析出来的 zip 条目。 */
interface ZipEntry {
  readonly name: string;
  readonly method: number;
  readonly crc: number;
  readonly data: Buffer;
}

/** 解析 zip：从结尾的目录记录读出全部条目，并还原内容。 */
function readZip(zip: Buffer): ZipEntry[] {
  const endOffset = zip.length - 22;
  expect(zip.readUInt32LE(endOffset)).toBe(0x06054b50);
  const count = zip.readUInt16LE(endOffset + 10);
  let offset = zip.readUInt32LE(endOffset + 16);
  const entries: ZipEntry[] = [];
  for (let index = 0; index < count; index += 1) {
    expect(zip.readUInt32LE(offset)).toBe(0x02014b50);
    const method = zip.readUInt16LE(offset + 10);
    const crc = zip.readUInt32LE(offset + 16);
    const compressedSize = zip.readUInt32LE(offset + 20);
    const size = zip.readUInt32LE(offset + 24);
    const nameLength = zip.readUInt16LE(offset + 28);
    const localOffset = zip.readUInt32LE(offset + 42);
    const name = zip.toString('utf8', offset + 46, offset + 46 + nameLength);
    expect(zip.readUInt32LE(localOffset)).toBe(0x04034b50);
    const localNameLength = zip.readUInt16LE(localOffset + 26);
    const dataStart = localOffset + 30 + localNameLength;
    const raw = zip.subarray(dataStart, dataStart + compressedSize);
    const data = method === 8 ? inflateRawSync(raw) : Buffer.from(raw);
    expect(data.length).toBe(size);
    entries.push({ name, method, crc, data });
    offset += 46 + nameLength;
  }
  return entries;
}

/** CRC-32，用来核对条目里记录的校验值。 */
function crc32(data: Buffer): number {
  let crc = 0xffffffff;
  for (const byte of data) {
    crc ^= byte;
    for (let bit = 0; bit < 8; bit += 1) crc = crc & 1 ? (crc >>> 1) ^ 0xedb88320 : crc >>> 1;
  }
  return (crc ^ 0xffffffff) >>> 0;
}

afterEach(() => {
  for (const root of roots.splice(0)) rmSync(root, { recursive: true, force: true });
});

describe('createZip', () => {
  it('生成的 zip 能还原每个条目，压得小就用 deflate，否则存储，校验值正确', () => {
    const big = Buffer.from('abc'.repeat(1000));
    const tiny = Buffer.from('x');
    const entries = readZip(createZip([{ name: 'big.txt', data: big }, { name: 'tiny.txt', data: tiny }, { name: 'empty.txt', data: Buffer.alloc(0) }]));
    expect(entries.map((entry) => entry.name)).toEqual(['big.txt', 'tiny.txt', 'empty.txt']);
    expect(entries.map((entry) => entry.method)).toEqual([8, 0, 0]);
    expect(entries[0]?.data.equals(big)).toBe(true);
    expect(entries[1]?.data.equals(tiny)).toBe(true);
    expect(entries[2]?.data.length).toBe(0);
    for (const entry of entries) expect(entry.crc).toBe(crc32(entry.data));
  });

  it('中文文件名按 UTF-8 写入', () => {
    expect(readZip(createZip([{ name: '说明.txt', data: Buffer.from('内容') }]))[0]?.name).toBe('说明.txt');
  });
});

describe('packPlugins', () => {
  it('把 plugins/<插件>/ 打成 build/plugins/<id>.emap，内容按文件名排序，输出可重现', () => {
    const root = createRoot();
    writePlugin(root, 'demo-plugin', { 'manifest.json': JSON.stringify({ id: 'demo-plugin', name: '示例' }), 'main.js': 'export default {};' });
    writePlugin(root, 'only-manifest', { 'manifest.json': JSON.stringify({ id: 'only-manifest' }) });
    const packed = packPlugins(root);
    expect(packed.map((item) => item.id)).toEqual(['demo-plugin', 'only-manifest']);
    expect(packed[0]?.file).toBe(join(root, 'build', 'plugins', 'demo-plugin.emap'));

    const first = readFileSync(join(root, 'build', 'plugins', 'demo-plugin.emap'));
    const entries = readZip(first);
    expect(entries.map((entry) => entry.name)).toEqual(['main.js', 'manifest.json']);
    expect(entries[0]?.data.toString('utf8')).toBe('export default {};');
    expect(JSON.parse(entries[1]?.data.toString('utf8') ?? '{}')).toEqual({ id: 'demo-plugin', name: '示例' });
    expect(readZip(readFileSync(join(root, 'build', 'plugins', 'only-manifest.emap'))).map((entry) => entry.name)).toEqual(['manifest.json']);

    packPlugins(root);
    expect(readFileSync(join(root, 'build', 'plugins', 'demo-plugin.emap')).equals(first)).toBe(true);
  });

  it('没有 plugins 目录时什么也不打', () => {
    expect(packPlugins(createRoot())).toEqual([]);
  });

  it('目录名必须等于清单里的 id', () => {
    const root = createRoot();
    const directory = writePlugin(root, 'folder-name', { 'manifest.json': JSON.stringify({ id: 'other-id' }) });
    expect(() => packPlugin(directory, join(root, 'out'))).toThrow('目录名必须等于清单里的 id');
  });

  it('拒绝多余的文件、缺少的清单、不合法的清单与标识', () => {
    const root = createRoot();
    expect(() => packPlugin(writePlugin(root, 'extra-file', { 'manifest.json': JSON.stringify({ id: 'extra-file' }), 'notes.txt': 'x' }), join(root, 'out'))).toThrow('不允许出现 notes.txt');
    expect(() => packPlugin(writePlugin(root, 'no-manifest', { 'main.js': 'x' }), join(root, 'out'))).toThrow('缺少 manifest.json');
    expect(() => packPlugin(writePlugin(root, 'bad-json', { 'manifest.json': '{' }), join(root, 'out'))).toThrow('不是合法的 JSON');
    expect(() => packPlugin(writePlugin(root, 'Bad_Id', { 'manifest.json': JSON.stringify({ id: 'Bad_Id' }) }), join(root, 'out'))).toThrow('id 必须是');
    expect(() => packPlugin(writePlugin(root, 'no-id', { 'manifest.json': JSON.stringify({ name: 'x' }) }), join(root, 'out'))).toThrow('id 必须是');
  });

  it('仓库里的官方插件 nrom 能打包，包里有清单与脚本', () => {
    const outDirectory = join(createRoot(), 'out');
    const result = packPlugin(join(REPO_ROOT, 'plugins', 'nrom'), outDirectory);
    expect(result.id).toBe('nrom');
    const entries = readZip(readFileSync(result.file));
    expect(entries.map((entry) => entry.name)).toEqual(['main.js', 'manifest.json']);
    expect(entries[0]?.data.toString('utf8')).toContain('export default');
    expect(JSON.parse(entries[1]?.data.toString('utf8') ?? '{}')).toMatchObject({ id: 'nrom', apiVersion: 1, match: [{ mapper: 0 }] });
  });
});
