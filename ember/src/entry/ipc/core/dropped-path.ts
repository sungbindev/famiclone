/*
 * ------------------------------------------------------------------------
 * 名称：dropped-path.ts
 * 说明：主进程请求按编号取拖入的文件：编号没有登记过、已失效或扩展名不是该请求接受的类型时，以请求自己的“找不到拖入的文件”错误拒绝。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：编号对应的路径由预加载脚本从拖入的文件对象取得，渲染进程不能指定路径；这里再按扩展名核对一次，界面传来的编号配错请求时不会把别的文件交给核心。
 * ------------------------------------------------------------------------
 */

import { CoreRequestError, type CoreErrorInfo } from '../../../contracts/core-gateway/core-messages';
import type { DroppedFilesService } from '../../../contracts/shell/dropped-files';

/** 取拖入的文件时的要求。 */
export interface DroppedPathRule {
  /** 接受的扩展名，小写、不带点。 */
  readonly extensions: readonly string[];
  /** 找不到或类型不符时的错误。 */
  readonly notFound: CoreErrorInfo;
}

/**
 * 按编号取拖入的文件路径。
 * @param files 拖入的文件服务。
 * @param id 编号。
 * @param rule 要求。
 * @returns 文件路径。
 * @throws CoreRequestError 编号无效或扩展名不符。
 */
export function resolveDroppedPath(files: DroppedFilesService, id: number, rule: DroppedPathRule): string {
  const path = files.resolve(id);
  const extension = path === undefined ? '' : path.slice(path.lastIndexOf('.') + 1).toLowerCase();
  if (path === undefined || !rule.extensions.includes(extension)) throw new CoreRequestError(rule.notFound);
  return path;
}
