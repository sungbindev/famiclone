/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeStatusItems.tsx
 * 说明：运行模块登记到状态栏左侧的项：卡带名、mapper 插件；没有信息的项不显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：电源状态与主机型号不在状态栏显示（运行状态由左侧运行按钮的图标表达）；帧率项没有做：核心的状态里只有帧数与周期数的累计值，没有帧率，界面不自己推算。
 * ------------------------------------------------------------------------
 */

import { StatusBarItem } from '@rujianjs/ui-kit';
import type { RuntimeController } from './runtime-controller';
import { cartridgeText, pluginText } from './runtime-status';
import { useRuntimeState } from './use-runtime-state';

/** 用户可见的文字。 */
const TEXT = {
  cartridge: '卡带',
  plugin: 'mapper 插件'
} as const;

/** 状态栏项的属性。 */
export interface RuntimeStatusItemProps {
  readonly controller: RuntimeController;
}

/** 卡带名；没有卡带时不显示。 */
export function CartridgeStatusItem({ controller }: RuntimeStatusItemProps) {
  const { status } = useRuntimeState(controller);
  const text = cartridgeText(status);
  return text === undefined ? null : <StatusBarItem text={text} title={TEXT.cartridge} />;
}

/** mapper 插件；没有卡带时不显示。 */
export function PluginStatusItem({ controller }: RuntimeStatusItemProps) {
  const { status } = useRuntimeState(controller);
  const text = pluginText(status);
  return text === undefined ? null : <StatusBarItem text={text} title={TEXT.plugin} />;
}
