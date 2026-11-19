/*
 * ------------------------------------------------------------------------
 * 名称：png.d.mts
 * 说明：png.mjs 的类型声明，供 TypeScript 测试引用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：无。
 * ------------------------------------------------------------------------
 */

/** 解码 PNG 文件，结果统一为 RGBA。 */
export function decodePng(file: string): { width: number; height: number; data: Uint8Array };
