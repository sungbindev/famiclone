/*
 * ------------------------------------------------------------------------
 * 名称：gen-icons.d.mts
 * 说明：gen-icons.mjs 的类型声明，供 TypeScript 测试引用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

/** 公共图标编号头文件的路径。 */
export const ID_FILE: string;
/** 路径数据文件的路径。 */
export const DATA_FILE: string;
/** 按文字匹配图标的规则表文件的路径。 */
export const RULES_FILE: string;
/** 生成三个文件的文本与图标个数。 */
export function generate(): { id: string; data: string; rules: string; count: number };
