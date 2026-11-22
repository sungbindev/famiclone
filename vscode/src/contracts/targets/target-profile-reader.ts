// ------------------------------------------------------------------------
// 名称：target-profile-reader.ts
// 说明：目标描述（profile.json）的读取接口：按内置名字或文件路径读出并检查目标描述。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：内置描述在扩展自带的 targets/<名>/profile.json；反汇编与模拟器的操作共用。
// ------------------------------------------------------------------------

import type { TargetProfile } from '../../shared/targets/target-profile';

export type { BootStep, ProfileInput, SelectedInput, TargetProfile } from '../../shared/targets/target-profile';

/** 读取目标描述的能力。 */
export interface TargetProfileReader {
  /**
   * 读取目标描述。
   * @param reference 内置描述的名字（如 nes），或描述文件的路径。
   * @param cwd 相对路径的起点目录。
   * @returns 目标描述与它的文件路径。
   * @throws FcdevError 找不到或不合法（错误码 PROFILE_NOT_FOUND、PROFILE_INVALID）。
   */
  read(reference: string, cwd: string): { profile: Readonly<TargetProfile>; file: string };

  /**
   * 列出内置目标描述的名字。
   * @returns 名字，按字母顺序。
   */
  listBuiltin(): string[];
}
