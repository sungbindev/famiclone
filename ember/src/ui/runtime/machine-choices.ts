/*
 * ------------------------------------------------------------------------
 * 名称：machine-choices.ts
 * 说明：“选择机型”的选项规则：四个整机机型，顺序固定；要程序里有实现并已添加 BIOS 才可选，不可选的注明原因；经典 FC 不在里面（它按卡带开机，打开卡带即选中）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：“文件”菜单的“选择机型”使用。
 * ------------------------------------------------------------------------
 */

import type { MachineEntry } from './runtime-controller';

/** 用户可见的文字。 */
const TEXT = {
  unsupported: '暂未支持',
  noBios: '未添加 BIOS，请在设置里添加'
} as const;

/** 机型的显示名，顺序固定。 */
const MACHINES: ReadonlyArray<{ readonly id: string; readonly label: string }> = [
  { id: 'sb97', label: '小霸王 97' },
  { id: 'sb2000', label: '小霸王 2000' },
  { id: 'bbk10', label: '步步高软驱一号' },
  { id: 'bbk20', label: '步步高 98' }
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
    const hint = info === undefined || !info.supported ? TEXT.unsupported : info.bios === undefined ? TEXT.noBios : undefined;
    return hint === undefined ? { id: machine.id, label: machine.label } : { id: machine.id, label: machine.label, hint };
  });
}
