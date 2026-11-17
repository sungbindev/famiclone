/*
 * ------------------------------------------------------------------------
 * 名称：allow-multiple-instances.ts
 * 说明：启动时读取首选项“允许同时运行多个实例”：直接读用户数据目录里的首选项文件，决定主进程要不要取单实例锁。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-18
 * 备注：单实例锁在应用就绪、核心启动之前就要决定，此时读不了核心里的设置，所以只读文件里的这一项；
 *       文件不存在、读不了、格式不对或取值不是布尔值时都按默认（不允许）处理，不报错。
 * ------------------------------------------------------------------------
 */

import { readFileSync } from 'node:fs';

/** 设置项的标识，与核心里登记的一致。 */
export const ALLOW_MULTIPLE_INSTANCES_SETTING_ID = 'app.allowMultipleInstances';

/** 首选项文件的格式版本，与核心一致。 */
const PREFERENCES_FILE_VERSION = 1;

/**
 * 读取是否允许同时运行多个实例。
 * @param file 首选项文件的路径（用户数据目录下的 config/preferences.json）。
 * @returns 设置为开启时为 true；其他情况（含读取失败）为 false。
 */
export function readAllowMultipleInstances(file: string): boolean {
  let document: unknown;
  try {
    document = JSON.parse(readFileSync(file, 'utf8'));
  } catch {
    return false;
  }
  if (typeof document !== 'object' || document === null) return false;
  const { version, values } = document as { version?: unknown; values?: unknown };
  if (version !== PREFERENCES_FILE_VERSION || typeof values !== 'object' || values === null) return false;
  return (values as Record<string, unknown>)[ALLOW_MULTIPLE_INSTANCES_SETTING_ID] === true;
}
