// ------------------------------------------------------------------------
// 名称：baseline-coverage.test.ts
// 说明：基线文件的覆盖检查：仓库里的每个样例项目与每个模板都有一条基线记录，且产物摘要齐全。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：基线由命令行时代的 fcdev 0.9.2 生成，生成脚本已随 packages/ 删除；对比用例以这份基线为准。
// ------------------------------------------------------------------------

import { readdirSync, readFileSync, existsSync } from 'node:fs';
import * as path from 'node:path';
import { describe, expect, it } from 'vitest';

const REPO_ROOT = path.resolve(__dirname, '..', '..', '..');

interface BaselineRecord {
  target: string;
  output: string;
  size: number;
  sha256: string;
  image?: { size: number; sha256: string };
}

interface Baseline {
  fcdevVersion: string;
  toolchain: string;
  projects: Record<string, BaselineRecord>;
  templates: Record<string, BaselineRecord>;
  disassembly: Record<string, { verified: boolean; files: Record<string, string> }>;
}

const baseline = JSON.parse(readFileSync(path.join(__dirname, 'fcdev-0.9.2-baseline.json'), 'utf8')) as Baseline;

/** 列出目录下含 fcdev.json 与 src 的样例项目，路径相对仓库根目录。 */
function listSampleProjects(group: string): string[] {
  const found: string[] = [];
  const walk = (directory: string): void => {
    for (const entry of readdirSync(directory, { withFileTypes: true })) {
      if (!entry.isDirectory() || ['node_modules', 'build', '.vscode'].includes(entry.name)) continue;
      const full = path.join(directory, entry.name);
      if (existsSync(path.join(full, 'fcdev.json')) && existsSync(path.join(full, 'src'))) {
        found.push(path.relative(REPO_ROOT, full).split(path.sep).join('/'));
      }
      walk(full);
    }
  };
  const root = path.join(REPO_ROOT, group);
  if (existsSync(path.join(root, 'fcdev.json'))) found.push(group);
  walk(root);
  return found;
}

describe('fcdev 0.9.2 基线', () => {
  it('记录了工具链版本', () => {
    expect(baseline.fcdevVersion).toBe('0.9.2');
    expect(baseline.toolchain).toMatch(/^cc65 V/);
  });

  it('覆盖仓库里的每个样例项目', () => {
    const expected = [...listSampleProjects('apps/sb2000'), ...listSampleProjects('apps/disasm-example')].toSorted();
    expect(Object.keys(baseline.projects).toSorted()).toEqual(expected);
  });

  it('每个项目与模板都有产物摘要，学习机目标另有磁盘镜像摘要', () => {
    for (const record of [...Object.values(baseline.projects), ...Object.values(baseline.templates)]) {
      expect(record.sha256).toMatch(/^[0-9a-f]{64}$/);
      expect(record.size).toBeGreaterThan(0);
      if (record.target === 'sb2000') expect(record.image?.sha256).toMatch(/^[0-9a-f]{64}$/);
    }
  });

  it('覆盖四个模板', () => {
    expect(Object.keys(baseline.templates).toSorted()).toEqual(['nes-asm', 'nes-c', 'sb2000-asm', 'sb2000-c']);
  });

  it('两个反汇编样例的回读都通过', () => {
    expect(Object.keys(baseline.disassembly).toSorted()).toEqual(['apps/disasm-example', 'apps/sb2000/examples/hello']);
    for (const record of Object.values(baseline.disassembly)) {
      expect(record.verified).toBe(true);
      expect(Object.keys(record.files)).toContain('src/main.s');
    }
  });
});
