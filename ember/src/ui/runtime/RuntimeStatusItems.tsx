/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeStatusItems.tsx
 * 说明：运行模块登记到状态栏的项：左侧是电源状态（状态点加文字）、主机型号、卡带名、mapper 插件，右侧是音量。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：状态点运行中为绿色、已暂停为黄色、其余为灰色，旁边始终有文字；帧率项没有做：核心的状态里只有帧数与周期数的累计值，没有帧率，界面不自己推算。
 * ------------------------------------------------------------------------
 */

import { StatusBarItem, type StatusBarDot } from '@rujianjs/ui-kit';
import type { RuntimeController } from './runtime-controller';
import { cartridgeText, hostText, machineState, pluginText, stateText, volumeText, type MachineState } from './runtime-status';
import { useRuntimeState } from './use-runtime-state';

/** 用户可见的文字。 */
const TEXT = {
  power: '电源状态',
  host: '主机型号',
  cartridge: '卡带',
  plugin: 'mapper 插件',
  volume: '音量'
} as const;

/** 各机器状态的状态点：运行中为绿色，已暂停为黄色，其余为灰色。 */
const DOTS: Readonly<Record<MachineState, StatusBarDot>> = { unknown: 'neutral', none: 'neutral', off: 'neutral', paused: 'warning', running: 'success' };

/** 状态栏项的属性。 */
export interface RuntimeStatusItemProps {
  readonly controller: RuntimeController;
}

/** 电源状态：状态点加文字。 */
export function PowerStatusItem({ controller }: RuntimeStatusItemProps) {
  const { status } = useRuntimeState(controller);
  return <StatusBarItem text={stateText(status)} dot={DOTS[machineState(status)]} title={TEXT.power} />;
}

/** 主机型号。 */
export function HostStatusItem({ controller }: RuntimeStatusItemProps) {
  const { status } = useRuntimeState(controller);
  return <StatusBarItem text={hostText(status)} title={TEXT.host} />;
}

/** 卡带名。 */
export function CartridgeStatusItem({ controller }: RuntimeStatusItemProps) {
  const { status } = useRuntimeState(controller);
  return <StatusBarItem text={cartridgeText(status)} title={TEXT.cartridge} />;
}

/** mapper 插件。 */
export function PluginStatusItem({ controller }: RuntimeStatusItemProps) {
  const { status } = useRuntimeState(controller);
  return <StatusBarItem text={pluginText(status)} title={TEXT.plugin} />;
}

/** 音量：静音时显示“已静音”。 */
export function VolumeStatusItem({ controller }: RuntimeStatusItemProps) {
  const { volume, muted } = useRuntimeState(controller);
  return <StatusBarItem text={volumeText(volume, muted)} title={TEXT.volume} />;
}
