/*
 * ------------------------------------------------------------------------
 * 名称：EmberLogo.tsx
 * 说明：Ember 的标志：一张卡带；标题栏里用强调色，空工作区里用淡色，颜色取所在位置的文字颜色。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：窗口、屏幕与卡带槽都是镂空的（奇偶填充），所以淡色半透明时底下的底色也不会透出别的形状；只作装饰，对读屏隐藏。
 * ------------------------------------------------------------------------
 */

/** 卡带外形、屏幕窗口与三个卡带槽（奇偶填充下是镂空）。 */
const LOGO_PATH =
  'M4 2h16a1 1 0 0 1 1 1v14h-3v4a1 1 0 0 1-1 1H7a1 1 0 0 1-1-1v-4H3V3a1 1 0 0 1 1-1Z' +
  'M7 5h10a1 1 0 0 1 1 1v5a1 1 0 0 1-1 1H7a1 1 0 0 1-1-1V6a1 1 0 0 1 1-1Z' +
  'M8.25 17h1.5v3h-1.5ZM11.25 17h1.5v3h-1.5ZM14.25 17h1.5v3h-1.5Z';

/** 标志的属性。 */
export interface EmberLogoProps {
  /** 用强调色（标题栏）；否则取所在位置的文字颜色。 */
  readonly brand?: boolean;
}

/** Ember 的标志。 */
export function EmberLogo({ brand = false }: EmberLogoProps) {
  return (
    <svg viewBox="0 0 24 24" aria-hidden="true" focusable="false" className={brand ? 'ember-logo--brand' : undefined}>
      <path d={LOGO_PATH} fill="currentColor" fillRule="evenodd" />
    </svg>
  );
}
