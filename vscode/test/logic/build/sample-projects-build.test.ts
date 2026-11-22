// ------------------------------------------------------------------------
// 名称：sample-projects-build.test.ts
// 说明：用扩展自带的构建核心构建仓库里的全部样例项目：都能构建成功，产物非空，
//       学习机项目默认同时生成磁盘镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：样例项目就地构建，输出在各项目被忽略的 build 目录；本机没有组装工具链时跳过。
// ------------------------------------------------------------------------

import { existsSync, readdirSync, statSync } from 'node:fs';
import * as path from 'node:path';
import { describe, expect, it } from 'vitest';
import { HAS_TOOLCHAIN, createRealBuildEnvironment } from '../../support/build-fixtures';

/** 仓库根目录。 */
const REPO_ROOT = path.resolve(__dirname, '..', '..', '..', '..');

/** 构建一个项目较慢，用例的超时时间。 */
const TIMEOUT_MS = 180_000;

/** 不进入的目录名。 */
const SKIPPED_DIRECTORIES = ['node_modules', 'build', '.vscode'];

/**
 * 列出目录下含 fcdev.json 与 src 的样例项目。
 * @param group 相对仓库根目录的起始目录。
 * @returns 相对仓库根目录、用 / 分隔的项目路径。
 */
function listSampleProjects(group: string): string[] {
  const found: string[] = [];
  const walk = (directory: string): void => {
    for (const entry of readdirSync(directory, { withFileTypes: true })) {
      if (!entry.isDirectory() || SKIPPED_DIRECTORIES.includes(entry.name)) continue;
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

/** 样例项目所在的目录。 */
const SAMPLE_PROJECTS = [...listSampleProjects('apps/sb2000'), ...listSampleProjects('apps/sb97'), ...listSampleProjects('apps/disasm-example')].toSorted();

describe.skipIf(!HAS_TOOLCHAIN)('样例项目都能构建', { timeout: TIMEOUT_MS }, () => {
  const { service, fileSystem } = createRealBuildEnvironment();

  it('找到了样例项目', () => {
    expect(SAMPLE_PROJECTS.length).toBeGreaterThan(0);
  });

  it.each(SAMPLE_PROJECTS)('%s', async (name) => {
    const outcome = await service.build({ pathArg: 'src', cwd: path.join(REPO_ROOT, name) });
    expect(statSync(outcome.result.outputFile).size).toBeGreaterThan(0);
    if (outcome.plan.target.disk !== null) {
      expect(fileSystem.kindOf(outcome.disk?.imageFile ?? '')).toBe('file');
    }
  });
});
