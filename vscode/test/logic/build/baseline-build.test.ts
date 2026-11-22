// ------------------------------------------------------------------------
// 名称：baseline-build.test.ts
// 说明：用扩展自带的构建核心构建全部样例项目，产物与磁盘镜像的 SHA-256
//       与 fcdev 0.9.2 的基线逐字节一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：样例项目就地构建，输出在各项目被忽略的 build 目录；
//       学习机的磁盘镜像用固定修改时间生成后比较。
// ------------------------------------------------------------------------

import { createHash } from 'node:crypto';
import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import { describe, expect, it } from 'vitest';
import { packageDisk } from '../../../src/shared/packaging/program-packager';
import { HAS_TOOLCHAIN, createRealBuildEnvironment } from '../../support/build-fixtures';

/** 仓库根目录。 */
const REPO_ROOT = path.resolve(__dirname, '..', '..', '..', '..');

/** 构建一个项目较慢，用例的超时时间。 */
const TIMEOUT_MS = 180_000;

/** 磁盘镜像里文件的固定修改时间，与基线记录时一致。 */
const FIXED_MTIME = new Date(2026, 0, 1, 12, 0, 0);

interface BaselineRecord {
  target: string;
  output: string;
  size: number;
  sha256: string;
  image?: { size: number; sha256: string };
}

const baseline = JSON.parse(readFileSync(path.join(__dirname, '..', '..', 'baseline', 'fcdev-0.9.2-baseline.json'), 'utf8')) as {
  projects: Record<string, BaselineRecord>;
};

/**
 * 计算数据的 SHA-256。
 * @param data 数据。
 * @returns 十六进制摘要。
 */
function sha256(data: Uint8Array): string {
  return createHash('sha256').update(data).digest('hex');
}

describe.skipIf(!HAS_TOOLCHAIN)('样例项目的产物与 fcdev 0.9.2 基线一致', { timeout: TIMEOUT_MS }, () => {
  const { service, fileSystem } = createRealBuildEnvironment();

  it.each(Object.entries(baseline.projects))('%s', async (name, expected) => {
    const outcome = await service.build({ pathArg: 'src', cwd: path.join(REPO_ROOT, name) });
    const output = readFileSync(outcome.result.outputFile);
    expect(path.basename(outcome.result.outputFile)).toBe(expected.output);
    expect(output.length).toBe(expected.size);
    expect(sha256(output)).toBe(expected.sha256);
    if (expected.image !== undefined && outcome.plan.target.disk !== null) {
      const image = packageDisk(outcome.plan.target.disk.packager, [{ name: expected.output, data: output, mtime: FIXED_MTIME }], { label: 'FCDEV' });
      expect(image.length).toBe(expected.image.size);
      expect(sha256(image)).toBe(expected.image.sha256);
      expect(fileSystem.kindOf(service.buildDisk(outcome).imageFile)).toBe('file');
    }
  });
});
