/*
 * ------------------------------------------------------------------------
 * 名称：MachinePicker.tsx
 * 说明：“运行”面板第二行左边的机型选择：机型下拉列表，选经典 FC 时右边有只有图标的“打开卡带”按钮。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：规则见 docs/05-界面.md 第 4.1 节；选机型只改已选机型并保存，不开机；没有 BIOS 或暂未支持的机型在选项下注明原因，选了不生效；
 *       打开卡带成功后已选机型自动变成经典 FC，由控制器负责。
 * ------------------------------------------------------------------------
 */

import { Button, iconFolderOpen, Select } from '@rujianjs/ui-kit';
import { machineChoices } from './machine-choices';
import { CLASSIC_MACHINE, type RuntimeController } from './runtime-controller';
import { useRuntimeState } from './use-runtime-state';
import './ember-run.css';

/** 用户可见的文字。 */
const TEXT = {
  machine: '机型',
  openCartridge: '打开卡带'
} as const;

/** 机型选择的属性。 */
export interface MachinePickerProps {
  readonly controller: RuntimeController;
}

/**
 * 机型下拉列表与打开卡带按钮。
 * @param props 属性。
 */
export function MachinePicker({ controller }: MachinePickerProps) {
  const { machines, selectedMachine } = useRuntimeState(controller);
  const choices = machineChoices(machines);
  const options = choices.map((choice) => ({ value: choice.id, label: choice.label, description: choice.hint ?? '' }));
  /** 不可选的机型选了不生效，下拉列表仍显示原来的选项。 */
  const selectMachine = (id: string) => {
    if (choices.some((choice) => choice.id === id && choice.hint === undefined)) controller.selectMachine(id);
  };

  return (
    <div className="ember-machine-picker">
      <Select className="ember-machine-picker__select" aria-label={TEXT.machine} options={options} value={selectedMachine} allowEmpty={false} onChange={selectMachine} />
      {selectedMachine === CLASSIC_MACHINE ? (
        <Button variant="ghost" iconOnly compact icon={iconFolderOpen} title={TEXT.openCartridge} onClick={() => controller.openCartridge()}>
          {TEXT.openCartridge}
        </Button>
      ) : null}
    </div>
  );
}
