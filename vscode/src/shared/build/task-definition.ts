// ------------------------------------------------------------------------
// 名称：task-definition.ts
// 说明：fcdev 任务的定义：tasks.json 里 type 为 fcdev 的任务字段的解析，以及构建结果的文字说明。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：任务字段与 package.json 的 taskDefinitions 一致；path 相对工作区文件夹，
//       缺省为文件夹本身。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import { FcdevError } from '../errors/fcdev-error';
import { isRecord } from '../json/json-text';
import type { BuildResult, DiskImageResult } from './build-plan';

/** 任务能做的事。 */
export const TASK_COMMANDS = ['build', 'run'] as const;

/** 任务命令。 */
export type TaskCommand = (typeof TASK_COMMANDS)[number];

/** 一个 fcdev 任务的定义。 */
export interface FcdevTaskDefinition {
  /** 要做的事：构建（目标支持磁盘镜像时同时生成镜像）、运行。 */
  readonly command: TaskCommand;
  /** 构建路径（文件或目录），相对工作区文件夹；不给时为文件夹本身。 */
  readonly path?: string | undefined;
  /** 覆盖 fcdev.json 里的目标。 */
  readonly target?: string | undefined;
  /** 运行时以调试模式启动模拟器。 */
  readonly debug?: boolean | undefined;
}

/**
 * 解析并检查任务定义。
 * @param raw 任务里的字段。
 * @returns 任务定义。
 * @throws FcdevError 命令不认识或字段类型不对。
 */
export function parseTaskDefinition(raw: unknown): FcdevTaskDefinition {
  if (!isRecord(raw)) throw new FcdevError('fcdev 任务的定义必须是对象', { code: 'TASK_INVALID' });
  const { command } = raw;
  if (typeof command !== 'string' || !(TASK_COMMANDS as readonly string[]).includes(command)) {
    throw new FcdevError(`fcdev 任务的 command 必须是 ${TASK_COMMANDS.join('、')} 之一`, { code: 'TASK_INVALID' });
  }
  for (const field of ['path', 'target'] as const) {
    if (raw[field] !== undefined && typeof raw[field] !== 'string') throw new FcdevError(`fcdev 任务的 ${field} 必须是字符串`, { code: 'TASK_INVALID' });
  }
  if (raw.debug !== undefined && typeof raw.debug !== 'boolean') throw new FcdevError('fcdev 任务的 debug 必须是布尔值', { code: 'TASK_INVALID' });
  return {
    command: command as TaskCommand,
    path: raw.path as string | undefined,
    target: raw.target as string | undefined,
    debug: raw.debug as boolean | undefined
  };
}

/**
 * 构建结果的一行说明。
 * @param result 构建结果。
 * @param targetName 目标名。
 * @param base 显示相对路径时的基准目录。
 * @returns 说明文字。
 */
export function describeBuildResult(result: BuildResult, targetName: string, base: string): string {
  const shown = path.relative(base, result.outputFile) || result.outputFile;
  return `已生成 ${shown}（${result.size} 字节，目标 ${targetName}，重新编译 ${result.compiled} 个文件，沿用 ${result.reused} 个）`;
}

/**
 * 磁盘镜像的一行说明。
 * @param image 镜像结果。
 * @param base 显示相对路径时的基准目录。
 * @returns 说明文字。
 */
export function describeDiskImage(image: DiskImageResult, base: string): string {
  return `已生成 ${path.relative(base, image.imageFile) || image.imageFile}（${image.size} 字节，含 ${image.files.join('、')}）`;
}
