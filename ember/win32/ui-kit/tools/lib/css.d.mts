/*
 * ------------------------------------------------------------------------
 * 名称：css.d.mts
 * 说明：css.mjs 的类型声明，供 TypeScript 测试引用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：求值结果与语法节点的结构是生成器内部约定，这里用宽松的类型描述。
 * ------------------------------------------------------------------------
 */

/** 去掉 CSS 注释。 */
export function stripComments(text: string): string;
/** 在括号与引号之外按分隔符拆分。 */
export function splitTopLevel(text: string, separator: string): string[];
/** 解析样式文本里的规则。 */
export function parseRules(source: string): { selector: string; media: string; declarations: Map<string, string> }[];
/** 把取值文本切成节点。 */
export function parseValue(text: string): { t: string }[];
/** 求值失败时的异常。 */
export class CssValueError extends Error {}
/** 求值环境：一个主题下全部自定义属性的取值。 */
export class Evaluator {
  constructor(variables: Map<string, string>);
  resolve(name: string): unknown;
  evaluateText(text: string): unknown;
}
