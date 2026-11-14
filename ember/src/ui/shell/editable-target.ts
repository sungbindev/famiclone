/*
 * ------------------------------------------------------------------------
 * 名称：editable-target.ts
 * 说明：判断键盘事件的目标是不是在编辑文字的控件：文本输入框、多行文字框、下拉框与 contenteditable 的元素。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：外壳用它决定不带 Ctrl 或 Alt 的快捷键（如 F7）在输入控件里是否留给控件自己；按钮、复选框、滑块等不接收文字的 input 不算。
 * ------------------------------------------------------------------------
 */

/** 不接收文字的 input 类型：焦点在这些控件上时不算在编辑文字。 */
const NON_TEXT_INPUT_TYPES: ReadonlySet<string> = new Set(['button', 'checkbox', 'radio', 'range', 'submit', 'reset', 'image', 'file', 'color']);

/** 判断所需的元素信息（HTMLElement 满足）。 */
interface EditableLike {
  readonly tagName?: unknown;
  readonly isContentEditable?: unknown;
  readonly type?: unknown;
}

/**
 * 事件的目标是不是在编辑文字的控件。
 * @param target 事件目标。
 * @returns 是时为 true。
 */
export function isTextEditingTarget(target: unknown): boolean {
  if (typeof target !== 'object' || target === null) return false;
  const element = target as EditableLike;
  if (element.isContentEditable === true) return true;
  const tag = typeof element.tagName === 'string' ? element.tagName.toUpperCase() : '';
  if (tag === 'TEXTAREA' || tag === 'SELECT') return true;
  if (tag === 'INPUT') return !(typeof element.type === 'string' && NON_TEXT_INPUT_TYPES.has(element.type.toLowerCase()));
  return false;
}
