// ------------------------------------------------------------------------
// 名称：disk-builder.mjs
// 说明：把构建出的程序放进磁盘镜像：按目标声明的磁盘打包器生成镜像文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：镜像里只放这一次构建的程序；镜像放在程序产物旁边，文件名与程序名相同。
// ------------------------------------------------------------------------
import { readFileSync, statSync } from 'node:fs';
import path from 'node:path';
import { getDiskPackager } from '../packagers/index.mjs';
import { FcdevError } from '../shared/index.mjs';

/** 镜像卷标 */
const DISK_LABEL = 'FCDEV';

/**
 * 生成包含这次构建的程序的磁盘镜像。
 * @param {import('./build-plan.mjs').BuildPlan} plan 构建计划
 * @param {import('./program-builder.mjs').BuildResult} result 构建结果
 * @returns {{imageFile: string, size: number}} 镜像路径与大小
 * @throws {FcdevError} 目标不支持磁盘镜像，或程序名不符合镜像文件名规则
 */
export function buildDiskImage(plan, result) {
  if (!plan.target.disk) {
    throw new FcdevError(`目标 ${plan.target.name} 不支持磁盘镜像`, { code: 'TARGET_NO_DISK' });
  }
  const outputName = path.basename(result.outputFile);
  const imageFile = path.join(path.dirname(result.outputFile), `${path.basename(outputName, path.extname(outputName))}.img`);
  const packaged = getDiskPackager(plan.target.disk.packager)({
    files: [{ name: outputName, data: readFileSync(result.outputFile), mtime: statSync(result.outputFile).mtime }],
    outputFile: imageFile,
    options: { label: DISK_LABEL },
  });
  return { imageFile: packaged.outputFile, size: packaged.size };
}
