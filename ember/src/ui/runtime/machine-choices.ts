/*
 * ------------------------------------------------------------------------
 * 名称：machine-choices.ts
 * 说明：“选择机型”的选项规则：经典 FC 与四个整机机型，顺序固定；经典 FC 总是可选，整机机型要程序里有实现并已添加 BIOS 才可选，不可选的注明原因。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：“文件”菜单的“选择机型”使用。
 * ------------------------------------------------------------------------
 */

import { CLASSIC_MACHINE, type MachineEntry } from './runtime-controller';

/** 用户可见的文字。 */
const TEXT = {
  unsupported: '暂未支持',
  noBios: '未添加 BIOS，请在设置里添加'
} as const;

/** 机型的显示名，顺序固定。 */
const MACHINES: ReadonlyArray<{ readonly id: string; readonly label: string }> = [
  { id: CLASSIC_MACHINE, label: '经典 FC' },
  { id: 'sb97', label: '小霸王 97' },
  { id: 'sb2000', label: '小霸王 2000' },
  { id: 'bbk10', label: '步步高 1.0' },
  { id: 'bbk20', label: '步步高 2.0' }
];

/** 一个机型选项。 */
export interface MachineChoice {
  readonly id: string;
  readonly label: string;
  /** 不可选的原因；可选时没有这一项。 */
  readonly hint?: string;
}

/**
 * 机型选项。
 * @param machines 核心给出的机型清单；还没读到时为空，整机机型都按“暂未支持”处理。
 */
export function machineChoices(machines: readonly MachineEntry[]): readonly MachineChoice[] {
  return MACHINES.map((machine) => {
    const info = machines.find((item) => item.id === machine.id);
    const hint = machine.id === CLASSIC_MACHINE ? undefined : info === undefined || !info.supported ? TEXT.unsupported : info.bios === undefined ? TEXT.noBios : undefined;
    return hint === undefined ? { id: machine.id, label: machine.label } : { id: machine.id, label: machine.label, hint };
  });
}
