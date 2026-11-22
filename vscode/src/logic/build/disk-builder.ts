// ------------------------------------------------------------------------
// 名称：disk-builder.ts
// 说明：把构建出的程序放进磁盘镜像：按目标声明的磁盘打包器生成镜像文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：镜像里放这一次构建的程序与项目根 disk 目录下的全部文件（使用说明等，由用户自己写）；
//       镜像放在程序产物旁边，文件名与程序名相同，卷标为 FCDEV。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { BuildPlan, BuildResult, DiskImageResult } from '../../shared/build/build-plan';
import { FcdevError } from '../../shared/errors/fcdev-error';
import type { DiskFile } from '../../shared/packaging/fat12-image';
import { packageDisk } from '../../shared/packaging/program-packager';

/** 镜像卷标。 */
const DISK_LABEL = 'FCDEV';

/** 项目根下放进镜像的文件所在的目录名。 */
const DISK_DIRECTORY = 'disk';

/**
 * 读取项目 disk 目录下要放进镜像的文件；没有这个目录时为空。
 * @param fileSystem 文件系统。
 * @param root 项目根目录。
 * @returns 镜像里的文件，按文件名排序。
 * @throws FcdevError 目录里有子目录（镜像只支持根目录）。
 */
function readDiskDirectory(fileSystem: FileSystem, root: string): DiskFile[] {
  const directory = path.join(root, DISK_DIRECTORY);
  if (fileSystem.kindOf(directory) !== 'directory') return [];
  return fileSystem
    .list(directory)
    .toSorted((left, right) => left.name.localeCompare(right.name))
    .map((entry) => {
      if (entry.isDirectory) {
        throw new FcdevError(`${DISK_DIRECTORY} 目录里不能有子目录：${entry.name}`, {
          code: 'DISK_DIRECTORY_INVALID',
          hint: '磁盘镜像只支持根目录，把文件直接放在 disk 目录下'
        });
      }
      const file = path.join(directory, entry.name);
      const modified = fileSystem.modifiedTime(file);
      return { name: entry.name, data: fileSystem.readBytes(file), ...(modified === undefined ? {} : { mtime: new Date(modified) }) };
    });
}

/**
 * 生成包含这次构建的程序与项目 disk 目录下全部文件的磁盘镜像。
 * @param fileSystem 文件系统。
 * @param plan 构建计划。
 * @param result 构建结果。
 * @returns 镜像路径与大小。
 * @throws FcdevError 目标不支持磁盘镜像，disk 目录有子目录或与程序重名，
 *   或文件名不符合镜像文件名规则。
 */
export function buildDiskImage(fileSystem: FileSystem, plan: BuildPlan, result: BuildResult): DiskImageResult {
  if (plan.target.disk === null) {
    throw new FcdevError(`目标 ${plan.target.name} 不支持磁盘镜像`, { code: 'TARGET_NO_DISK' });
  }
  const outputName = path.basename(result.outputFile);
  const imageFile = path.join(path.dirname(result.outputFile), `${path.basename(outputName, path.extname(outputName))}.img`);
  const modified = fileSystem.modifiedTime(result.outputFile);
  const extraFiles = readDiskDirectory(fileSystem, plan.config.root);
  const clash = extraFiles.find((file) => file.name.toUpperCase() === outputName.toUpperCase());
  if (clash !== undefined) {
    throw new FcdevError(`${DISK_DIRECTORY} 目录里的 ${clash.name} 与程序 ${outputName} 重名`, {
      code: 'DISK_NAME_CLASH',
      hint: '把 disk 目录里的这个文件改名或删除'
    });
  }
  const files: DiskFile[] = [
    { name: outputName, data: fileSystem.readBytes(result.outputFile), ...(modified === undefined ? {} : { mtime: new Date(modified) }) },
    ...extraFiles
  ];
  const image = packageDisk(plan.target.disk.packager, files, { label: DISK_LABEL });
  fileSystem.writeBytes(imageFile, image);
  return { imageFile, size: image.length, files: files.map((file) => file.name) };
}
