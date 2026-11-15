/*
 * ------------------------------------------------------------------------
 * 名称：PadSlotEditor.tsx
 * 说明：一个玩家的手柄设置：一张手柄图，点图上的键就设置它；“输入来源”在键盘与实体手柄之间切换，决定图上显示和设置的是哪一份绑定。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：键盘与实体手柄是同一个手柄键上互相独立的两份绑定，切换来源只改变显示与学习的对象，不改数据、不清除另一份；
 *       绑定用“学习”的方式：点图上的键后按下键盘上的键或手柄上的按钮，学习中再点一次同一个键取消；选中的键保持选中，下方说明行显示它的当前绑定并可清除。
 *       实体手柄来源要先选定手柄且手柄已连接才能设置。本组件只显示与转发操作，不保存数据；规则见 docs/05-界面.md 第 4.6 节。
 * ------------------------------------------------------------------------
 */

import { Button, Field, RadioGroup, Row, Select, Stack } from '@rujianjs/ui-kit';
import { useState } from 'react';
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
import { PadDiagram, type PadHotspot } from './PadDiagram';
import type { Waiting } from './use-binding-capture';

/** 输入来源：键盘，或指定的实体手柄。 */
type Source = 'key' | 'pad';

/** 用户可见的文字。 */
const TEXT = {
  source: '输入来源',
  keyboard: '键盘',
  gamepad: '实体手柄',
  device: '使用的手柄',
  deviceHint: '手柄接上后要按一下它的任意按钮，系统才会发现它。',
  none: '不使用',
  listening: '请按键',
  waitKey: '请按下键盘上的键（Esc 取消）',
  waitPad: '请按下手柄上的按钮或推动摇杆（再点一次取消）',
  idleKey: '点手柄上的键，再按下键盘上的键来设置。',
  idlePad: '点手柄上的键，再按下手柄上的按钮或推动摇杆来设置。',
  noDevice: '先在上面选择要使用的手柄。',
  offline: '这个手柄现在没有连接，接上后要按一下它的任意按钮。',
  clear: '清除',
  clearNone: '清除绑定',
  bindKey: (button: string, current: string) => `绑定键盘按键：${button}（当前：${current}）`,
  bindPad: (button: string, current: string) => `绑定手柄按钮：${button}（当前：${current}）`,
  clearKey: (button: string) => `清除键盘按键：${button}`,
  clearPad: (button: string) => `清除手柄按钮：${button}`,
  learning: (button: string, wait: string) => `设置“${button}”：${wait}`,
  current: (button: string, current: string) => `${button}：${current}`
} as const;

/** 来源切换的选项。 */
const SOURCE_OPTIONS = [
  { value: 'key', label: TEXT.keyboard },
  { value: 'pad', label: TEXT.gamepad }
];

/** 手柄键的名称，如“上”“A”“连发 A”。 */
function nameOf(button: PadButtonName): string {
  return PAD_BUTTONS.find((item) => item.id === button)?.label ?? button;
}

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
  const [source, setSource] = useState<Source>('key');
  const [selected, setSelected] = useState<PadButtonName | null>(null);
  const assigned = slot.gamepad === undefined ? undefined : { id: slot.gamepad.id, occurrence: slot.gamepad.occurrence };
  const isConnected = (device: PadDevice): boolean => devices.some((item) => item.id === device.id && item.occurrence === device.occurrence);
  const connected = assigned !== undefined && isConnected(assigned);
  // 键盘总能设置；实体手柄要先指定并且已连接。
  const usable = source === 'key' || connected;
  // 选项：系统发现的手柄，加上已经指定但现在没接上的那个。
  const choices = assigned !== undefined && !connected ? [...devices, assigned] : [...devices];
  const options = choices.map((device) => ({ value: deviceKey(device), label: deviceLabel(device, isConnected(device)) }));
  const learning: PadButtonName | null = waiting !== null && waiting.slot === slotIndex && waiting.kind === source ? waiting.button : null;
  const waitText = source === 'key' ? TEXT.waitKey : TEXT.waitPad;

  /** 一个键当前来源下的绑定文字；没有绑定时为 undefined。 */
  const boundTextOf = (button: PadButtonName): string | undefined => {
    if (source === 'key') {
      const code = keyOf(slot, button);
      return code === undefined ? undefined : keyLabel(code);
    }
    const binding = bindingOf(slot.gamepad, button);
    return binding === undefined ? undefined : sourceLabel(binding);
  };

  const hotspots: PadHotspot[] = PAD_BUTTONS.map(({ id, label }) => {
    const bound = boundTextOf(id);
    const current = bound ?? unboundText();
    const isLearning = learning === id;
    let ariaLabel = isLearning ? waitText : TEXT.bindPad(label, current);
    if (source === 'key' && !isLearning) ariaLabel = TEXT.bindKey(label, current);
    return { id, role: label, current: isLearning ? TEXT.listening : current, unbound: bound === undefined && !isLearning, ariaLabel, selected: selected === id, learning: isLearning };
  });

  /** 点图上的键：正在学习的再点一次取消，否则选中它并开始学习。 */
  const pick = (button: PadButtonName): void => {
    if (learning === button) {
      onCancel();
      return;
    }
    setSelected(button);
    if (source === 'key') onStartKey(slotIndex, button);
    else if (assigned !== undefined && connected) onStartPad(slotIndex, button, assigned);
  };

  const changeSource = (value: string): void => {
    onCancel();
    setSelected(null);
    setSource(value === 'pad' ? 'pad' : 'key');
  };

  let message: string = source === 'key' ? TEXT.idleKey : TEXT.idlePad;
  if (source === 'pad' && assigned === undefined) message = TEXT.noDevice;
  else if (source === 'pad' && !connected) message = TEXT.offline;
  else if (learning !== null) message = TEXT.learning(nameOf(learning), waitText);
  else if (selected !== null) message = TEXT.current(nameOf(selected), boundTextOf(selected) ?? unboundText());

  const canClear = selected !== null && boundTextOf(selected) !== undefined;
  let clearLabel: string = TEXT.clearNone;
  if (selected !== null) clearLabel = source === 'key' ? TEXT.clearKey(nameOf(selected)) : TEXT.clearPad(nameOf(selected));
  const clear = (): void => {
    if (selected === null) return;
    onCancel();
    if (source === 'key') onClearKey(slotIndex, selected);
    else onClearPad(slotIndex, selected);
  };

  return (
    <Stack>
      <Row>
        <RadioGroup aria-label={TEXT.source} variant="segmented" options={SOURCE_OPTIONS} value={source} onChange={changeSource} />
      </Row>
      {source === 'pad' ? (
        <Field label={TEXT.device} description={TEXT.deviceHint}>
          <Select
            aria-label={TEXT.device}
            placeholder={TEXT.none}
            options={options}
            value={assigned === undefined ? '' : deviceKey(assigned)}
            onChange={(value) => onAssign(slotIndex, parseDeviceKey(value))}
          />
        </Field>
      ) : null}
      <PadDiagram hotspots={hotspots} disabled={!usable} onPick={pick} />
      <div className="ember-pad-status">
        <span>{message}</span>
        <Button compact disabled={!canClear} aria-label={clearLabel} onClick={clear}>
          {TEXT.clear}
        </Button>
      </div>
    </Stack>
  );
}
