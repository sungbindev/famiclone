// ------------------------------------------------------------------------
// 名称：name-rules.mjs
// 说明：程序名规则：按目标声明的规则校验并规范化程序名。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：目前只有 DOS 8.3 一条规则；超长或含非法字符时报错，不静默截断。
// ------------------------------------------------------------------------
import { FcdevError } from '../shared/index.mjs';

/** DOS 8.3 的主名：1 到 8 个字母、数字、下划线或连字符 */
const DOS_BASE_NAME = /^[A-Za-z0-9_-]{1,8}$/;

/**
 * 按目标的程序名规则校验并规范化程序名。
 * @param {string} name 程序名（不含扩展名）
 * @param {string|null} rule 目标声明的规则；null 表示不限制
 * @returns {string} 规范化后的程序名
 * @throws {FcdevError} 程序名不符合规则
 */
export function applyNameRule(name, rule) {
  if (rule === null) return name;
  if (rule === 'dos-8.3') {
    if (!DOS_BASE_NAME.test(name)) {
      throw new FcdevError(`程序名“${name}”不符合 DOS 8.3 规则：主名为 1 到 8 个字母、数字、下划线或连字符`, {
        code: 'NAME_INVALID',
        hint: '在 fcdev.json 里用 name 指定程序名，或改文件名',
      });
    }
    return name.toUpperCase();
  }
  throw new FcdevError(`未知的程序名规则：${rule}`, { code: 'TARGET_INVALID' });
}
