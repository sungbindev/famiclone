/*
 * ------------------------------------------------------------------------
 * 名称：PadSlotEditor.tsx
 * 说明：一个玩家的手柄设置：键盘部分是手柄的 10 个键各绑定一个物理键；实体手柄部分先从系统发现的手柄里选一个，再让手柄的 10 个键各绑定它的一个按钮或轴。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：绑定用“学习”的方式：点绑定按钮后按下键盘上的键或手柄上的按钮；学习中再点一次取消。没有绑定的键显示“未绑定”。
 *       本组件只显示与转发操作，不保存数据；规则见 docs/05-界面.md 第 4.6 节。
 * ------------------------------------------------------------------------
 */

import { Button, List, Panel, Row, Select, SettingRow, Stack } from '@rujianjs/ui-kit';
import type { PadSlot } from '../../contracts/input/generated/input';
import {
  bindingOf,
  deviceKey,
  deviceLabel,
  keyLabel,
  keyOf,
  PAD_BUTTONS,
  parseDeviceKey,
  sourceLabel,
  unboundText,
  type PadButtonName,
  type PadDevice
} from './input-model';
import type { Waiting } from './use-binding-capture';

/** 用户可见的文字。 */
const TEXT = {
  keyboard: '键盘',
  gamepad: '实体手柄',
  device: '使用的手柄',
  deviceHint: '手柄接上后要按一下它的任意按钮，系统才会发现它。',
  none: '不使用',
  waitKey: '请按下键盘上的键（Esc 取消）',
  waitPad: '请按下手柄上的按钮或推动摇杆（再点一次取消）',
  clear: '清除',
  bindKey: (button: string, current: string) => `绑定键盘按键：${button}（当前：${current}）`,
  bindPad: (button: string, current: string) => `绑定手柄按钮：${button}（当前：${current}）`,
  clearKey: (button: string) => `清除键盘按键：${button}`,
  clearPad: (button: string) => `清除手柄按钮：${button}`
} as const;

/** 一个玩家的设置的属性。 */
export interface PadSlotEditorProps {
  /** 玩家编号，从 0 开始。 */
  readonly slotIndex: number;
  readonly slot: PadSlot;
  /** 系统发现的手柄。 */
  readonly devices: readonly PadDevice[];
  /** 正在学习的键。 */
  readonly waiting: Waiting | null;
  readonly onStartKey: (slot: number, button: PadButtonName) => void;
  readonly onStartPad: (slot: number, button: PadButtonName, device: PadDevice) => void;
  readonly onCancel: () => void;
  readonly onClearKey: (slot: number, button: PadButtonName) => void;
  readonly onClearPad: (slot: number, button: PadButtonName) => void;
  /** 指定使用的手柄；undefined 是不使用。 */
  readonly onAssign: (slot: number, device: PadDevice | undefined) => void;
}

/**
 * 一个玩家的手柄设置。
 * @param props 属性。
 */
export function PadSlotEditor({ slotIndex, slot, devices, waiting, onStartKey, onStartPad, onCancel, onClearKey, onClearPad, onAssign }: PadSlotEditorProps) {
  const assigned = slot.gamepad === undefined ? undefined : { id: slot.gamepad.id, occurrence: slot.gamepad.occurrence };
  const isConnected = (device: PadDevice): boolean => devices.some((item) => item.id === device.id && item.occurrence === device.occurrence);
  // 选项：系统发现的手柄，加上已经指定但现在没接上的那个。
  const choices = assigned !== undefined && !isConnected(assigned) ? [...devices, assigned] : [...devices];
  const options = choices.map((device) => ({ value: deviceKey(device), label: deviceLabel(device, isConnected(device)) }));

  return (
    <Stack>
      <Panel fixed title={TEXT.keyboard}>
        <List variant="divided">
          {PAD_BUTTONS.map(({ id, label }) => {
            const code = keyOf(slot, id);
            const current = code === undefined ? unboundText() : keyLabel(code);
            const learning = waiting?.kind === 'key' && waiting.slot === slotIndex && waiting.button === id;
            return (
              <SettingRow
                key={id}
                label={label}
                control={
                  <Row>
                    <Button compact pressed={learning} aria-label={learning ? TEXT.waitKey : TEXT.bindKey(label, current)} onClick={() => (learning ? onCancel() : onStartKey(slotIndex, id))}>
                      {learning ? TEXT.waitKey : current}
                    </Button>
                    <Button compact disabled={code === undefined} aria-label={TEXT.clearKey(label)} onClick={() => onClearKey(slotIndex, id)}>
                      {TEXT.clear}
                    </Button>
                  </Row>
                }
              />
            );
          })}
        </List>
      </Panel>
      <Panel fixed title={TEXT.gamepad}>
        <List variant="divided">
          <SettingRow
            label={TEXT.device}
            description={TEXT.deviceHint}
            control={
              <Select
                aria-label={TEXT.device}
                placeholder={TEXT.none}
                options={options}
                value={assigned === undefined ? '' : deviceKey(assigned)}
                onChange={(value) => onAssign(slotIndex, parseDeviceKey(value))}
              />
            }
          />
          {assigned === undefined
            ? null
            : PAD_BUTTONS.map(({ id, label }) => {
                const binding = bindingOf(slot.gamepad, id);
                const current = binding === undefined ? unboundText() : sourceLabel(binding);
                const learning = waiting?.kind === 'pad' && waiting.slot === slotIndex && waiting.button === id;
                return (
                  <SettingRow
                    key={id}
                    label={label}
                    control={
                      <Row>
                        <Button
                          compact
                          pressed={learning}
                          disabled={!isConnected(assigned)}
                          aria-label={learning ? TEXT.waitPad : TEXT.bindPad(label, current)}
                          onClick={() => (learning ? onCancel() : onStartPad(slotIndex, id, assigned))}
                        >
                          {learning ? TEXT.waitPad : current}
                        </Button>
                        <Button compact disabled={binding === undefined} aria-label={TEXT.clearPad(label)} onClick={() => onClearPad(slotIndex, id)}>
                          {TEXT.clear}
                        </Button>
                      </Row>
                    }
                  />
                );
              })}
        </List>
      </Panel>
    </Stack>
  );
}
