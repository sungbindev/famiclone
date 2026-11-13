/*
 * ------------------------------------------------------------------------
 * 名称：schema-gen.d.mts
 * 说明：Schema 代码生成脚本的类型声明，供测试与其他脚本引用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

/** 一个模块的 Schema 及其来源文件。 */
export interface LoadedSchema {
  readonly name: string;
  readonly file: string;
  readonly schema: Record<string, unknown>;
  readonly refs: readonly { readonly module: string; readonly name: string }[];
}

/** 读取并检查全部模块的 Schema。 */
export function loadSchemas(root: string): LoadedSchema[];

/** 生成全部代码。 */
export function generate(root: string): { written: string[]; removed: string[]; modules: string[] };
