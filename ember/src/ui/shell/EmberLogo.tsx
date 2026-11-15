/*
 * ------------------------------------------------------------------------
 * 名称：EmberLogo.tsx
 * 说明：Ember 的标志：一张蓝色的卡带图，与应用图标是同一张图（ember-logo.png）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：图是正方形、自带颜色，不随主题变色；只作装饰，对读屏隐藏；图由 tools/build/make-icons.cjs 从文档仓库里的原图生成。
 * ------------------------------------------------------------------------
 */

import logoUrl from './ember-logo.png';

/** Ember 的标志。 */
export function EmberLogo() {
  return <img src={logoUrl} alt="" aria-hidden="true" draggable={false} />;
}
