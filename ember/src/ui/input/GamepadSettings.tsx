/*
 * ------------------------------------------------------------------------
 * 名称：GamepadSettings.tsx
 * 说明：“手柄设置”对话框的内容：两个玩家各一个页签，每个玩家设置键盘的 10 个键与可选的一个实体手柄；改动立即保存，下方有“恢复默认”。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：页面不显示操作结果：保存失败用外壳通知给出核心返回的说明，页面保持原样。规则见 docs/05-界面.md 第 4.6 节。
 * ------------------------------------------------------------------------
 */

import { Button, Callout, Row, ScrollArea, Stack, State, Tabs } from '@rujianjs/ui-kit';
import { useRef } from 'react';
import type { PadSlot } from '../../contracts/input/generated/input';
import { useShell } from '../shell/shell-context';
import type { ShellPageProps } from '../shell/shell-registry';
import { assignDevice, bindGamepad, bindKey, clearGamepad, clearKey, errorText, type PadButtonName, type PadDevice } from './input-model';
import { PadSlotEditor } from './PadSlotEditor';
import { useBindingCapture } from './use-binding-capture';
import { useGamepadDevices } from './use-gamepad-devices';
import { useInputBindings } from './use-input-bindings';

/** 用户可见的文字。 */
const TEXT = {
  intro: '键盘只在机器本身不用键盘时才当手柄用：红白机一直如此，学习机在红白机兼容模式下如此。实体手柄在哪种机器上都能用。同一个键盘按键只能绑给一个手柄键。',
  loading: '正在读取…',
  reset: '恢复默认',
  players: '玩家',
  player: (number: number) => `玩家 ${number}`
} as const;

/**
 * 手柄设置页面。
 * @param props 外壳传入的属性。
 */
export function GamepadSettings({ bridge }: ShellPageProps) {
  const { notify } = useShell();
  const { state, save, reset } = useInputBindings(bridge);
  const rootRef = useRef<HTMLDivElement>(null);
  const devices = useGamepadDevices(rootRef);
  const slots: readonly PadSlot[] = state.kind === 'ready' ? state.slots : [];

  /** 保存修改后的槽位；失败时用通知提示，页面保持原样。 */
  const apply = (next: readonly PadSlot[]): void => {
    save(next).catch((error: unknown) => notify(errorText(error), 'error'));
  };

  const capture = useBindingCapture(rootRef, {
    onKey: (slot, button, code) => apply(bindKey(slots, slot, button, code)),
    onPad: (slot, button, source) => apply(bindGamepad(slots, slot, button, source))
  });

  const restore = (): void => {
    capture.cancel();
    reset().catch((error: unknown) => notify(errorText(error), 'error'));
  };

  const assign = (slot: number, device: PadDevice | undefined): void => {
    capture.cancel();
    apply(assignDevice(slots, slot, device));
  };

  const clearKeyOf = (slot: number, button: PadButtonName): void => apply(clearKey(slots, slot, button));
  const clearPadOf = (slot: number, button: PadButtonName): void => apply(clearGamepad(slots, slot, button));

  let status = null;
  if (state.kind === 'failed') status = <State kind="error" text={state.message} />;
  else if (state.kind === 'loading') status = <State kind="busy" text={TEXT.loading} />;

  return (
    <ScrollArea bleed horizontal="hidden">
      <div ref={rootRef}>
        <Stack>
          <Callout text={TEXT.intro} />
          {status}
          {state.kind === 'ready' ? (
            <>
              <Tabs
                aria-label={TEXT.players}
                onSelect={capture.cancel}
                items={slots.map((slot, index) => ({
                  id: `slot${index}`,
                  label: TEXT.player(index + 1),
                  content: (
                    <PadSlotEditor
                      slotIndex={index}
                      slot={slot}
                      devices={devices}
                      waiting={capture.waiting}
                      onStartKey={capture.startKey}
                      onStartPad={capture.startPad}
                      onCancel={capture.cancel}
                      onClearKey={clearKeyOf}
                      onClearPad={clearPadOf}
                      onAssign={assign}
                    />
                  )
                }))}
              />
              <Row>
                <Button onClick={restore}>{TEXT.reset}</Button>
              </Row>
            </>
          ) : null}
        </Stack>
      </div>
    </ScrollArea>
  );
}
