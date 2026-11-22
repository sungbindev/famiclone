// ------------------------------------------------------------------------
// 名称：disk-builder.ts
// 说明：把构建出的程序放进磁盘镜像：按目标声明的磁盘打包器生成镜像文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：镜像里只放这一次构建的程序；镜像放在程序产物旁边，文件名与程序名相同，卷标为 FCDEV。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { BuildPlan, BuildResult, DiskImageResult } from '../../shared/build/build-plan';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { packageDisk } from '../../shared/packaging/program-packager';

/** 镜像卷标。 */
const DISK_LABEL = 'FCDEV';

/**
 * 生成包含这次构建的程序的磁盘镜像。
 * @param fileSystem 文件系统。
 * @param plan 构建计划。
 * @param result 构建结果。
 * @returns 镜像路径与大小。
 * @throws FcdevError 目标不支持磁盘镜像，或程序名不符合镜像文件名规则。
 */
export function buildDiskImage(fileSystem: FileSystem, plan: BuildPlan, result: BuildResult): DiskImageResult {
  if (plan.target.disk === null) {
    throw new FcdevError(`目标 ${plan.target.name} 不支持磁盘镜像`, { code: 'TARGET_NO_DISK' });
  }
  const outputName = path.basename(result.outputFile);
  const imageFile = path.join(path.dirname(result.outputFile), `${path.basename(outputName, path.extname(outputName))}.img`);
  const modified = fileSystem.modifiedTime(result.outputFile);
  const image = packageDisk(
    plan.target.disk.packager,
    [{ name: outputName, data: fileSystem.readBytes(result.outputFile), ...(modified === undefined ? {} : { mtime: new Date(modified) }) }],
    { label: DISK_LABEL }
  );
  fileSystem.writeBytes(imageFile, image);
  return { imageFile, size: image.length };
}
