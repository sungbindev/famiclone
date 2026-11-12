// ------------------------------------------------------------------------
// 名称：fat-floppy.mjs
// 说明：磁盘镜像打包器 fat-floppy：把程序文件放进 1.44MB 的 FAT12 软盘镜像的根目录。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：磁盘打包器与程序打包器不同：输入是一组文件，输出是一个镜像文件。
// ------------------------------------------------------------------------
import { writeFileSync } from 'node:fs';
import { buildFat12Image } from './fat12-image.mjs';

/**
 * 生成软盘镜像并写到文件。
 * @param {{files: {name: string, data: Buffer, mtime?: Date}[], outputFile: string, options?: {label?: string}}} input 要放进镜像的文件、镜像路径与卷标
 * @returns {{outputFile: string, size: number}} 镜像路径与大小
 * @throws {import('../shared/index.mjs').FcdevError} 文件名不符合 8.3，或空间不足
 */
export function packFatFloppy({ files, outputFile, options = {} }) {
  const image = buildFat12Image(files, { label: options.label });
  writeFileSync(outputFile, image);
  return { outputFile, size: image.length };
}
