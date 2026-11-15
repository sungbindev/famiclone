/*
 * ------------------------------------------------------------------------
 * 名称：GamepadSettings.test.tsx
 * 说明：手柄设置页面的测试：手柄图上的 10 个键、按玩家显示键盘绑定、学习键盘键（物理键不重复、Esc 取消）、选中键的说明与清除、切换输入来源、选择实体手柄并学习按钮与轴、保存失败用通知提示、恢复默认、
 *       在别处修改（事件）后页面更新、读取失败的提示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：手柄用假的 navigator.getGamepads，动画帧用手动触发的假实现，页面里的定时刷新用真计时器。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, fireEvent, render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { InputSetBindingsParams, PadSlot } from '../../contracts/input/generated/input';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { ShellApiContext } from '../shell/shell-context';
import type { ShellApi } from '../shell/shell-registry';
import { GamepadSettings } from './GamepadSettings';

const notify = vi.fn();
const shell = { notify } as unknown as ShellApi;

/** 出厂默认：玩家 1 用键盘，玩家 2 空。 */
function defaultSlots(): PadSlot[] {
  return [
    {
      keys: [
        { button: 'up', code: 'ArrowUp' },
        { button: 'down', code: 'ArrowDown' },
        { button: 'left', code: 'ArrowLeft' },
        { button: 'right', code: 'ArrowRight' },
        { button: 'b', code: 'KeyZ' },
        { button: 'a', code: 'KeyX' },
        { button: 'turboB', code: 'KeyC' },
        { button: 'turboA', code: 'KeyV' },
        { button: 'select', code: 'ShiftRight' },
        { button: 'start', code: 'Enter' }
      ]
    },
    { keys: [] }
  ];
}

/** 假手柄。 */
function fakePad(index: number, id: string, pressed: boolean[] = [false, false], axes: number[] = [0, 0]) {
  return { index, id, connected: true, buttons: pressed.map((value) => ({ pressed: value })), axes };
}

/** 假预加载接口：记录请求，可以主动触发事件与状态变化。 */
function setup(initial: () => Promise<{ slots: PadSlot[] }> = () => Promise.resolve({ slots: defaultSlots() })) {
  const eventListeners = new Set<(name: string, data: unknown) => void>();
  const statusListeners = new Set<(status: CoreStatus) => void>();
  const getBindings = vi.fn(initial);
  const setBindings = vi.fn((params: InputSetBindingsParams) => Promise.resolve({ slots: params.slots }));
  const resetBindings = vi.fn(() => Promise.resolve({ slots: defaultSlots() }));
  const bridge = {
    core: { input: { getBindings, setBindings, resetBindings } },
    onCoreEvent: (listener: (name: string, data: unknown) => void) => {
      eventListeners.add(listener);
      return () => eventListeners.delete(listener);
    },
    onCoreStatusChange: (listener: (status: CoreStatus) => void) => {
      statusListeners.add(listener);
      return () => statusListeners.delete(listener);
    }
  } as unknown as EmberBridge;
  render(
    <UiProvider>
      <ShellApiContext.Provider value={shell}>
        <GamepadSettings bridge={bridge} />
      </ShellApiContext.Provider>
    </UiProvider>
  );
  return {
    getBindings,
    setBindings,
    resetBindings,
    emit: (name: string, data: unknown) => act(() => eventListeners.forEach((listener) => listener(name, data))),
    lastSaved: (): PadSlot[] => {
      const call = setBindings.mock.calls.at(-1);
      if (call === undefined) throw new Error('没有保存过');
      return call[0].slots;
    }
  };
}

/** 让页面看到的手柄列表。 */
function plugPads(pads: () => unknown[]): void {
  Object.defineProperty(navigator, 'getGamepads', { configurable: true, value: pads });
}

/** 把输入来源切换到“实体手柄”。 */
async function showPadSource(): Promise<void> {
  await userEvent.click(await screen.findByRole('radio', { name: '实体手柄' }));
}

beforeEach(() => notify.mockClear());
afterEach(() => {
  Reflect.deleteProperty(navigator, 'getGamepads');
  vi.restoreAllMocks();
});

describe('GamepadSettings：显示', () => {
  it('读取后显示玩家 1 的键盘绑定，没有绑定的显示“未绑定”', async () => {
    setup();
    expect(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' })).toBeTruthy();
    expect(screen.getByRole('button', { name: '绑定键盘按键：开始（当前：回车）' })).toBeTruthy();
    expect(screen.getByRole('button', { name: '绑定键盘按键：选择（当前：右 Shift）' })).toBeTruthy();
    expect(screen.getByRole('tab', { name: '玩家 2' })).toBeTruthy();
    await userEvent.click(screen.getByRole('tab', { name: '玩家 2' }));
    expect(screen.getByRole('button', { name: '绑定键盘按键：A（当前：未绑定）' })).toBeTruthy();
  });

  it('读取失败显示核心给出的说明', async () => {
    setup(() => Promise.reject({ code: 'core.unavailable', message: '核心未启动' }));
    expect(await screen.findByText('核心未启动')).toBeTruthy();
  });

  it('在别处修改绑定（核心发 input.bindingsChanged）后页面立即更新', async () => {
    const { emit } = setup();
    await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' });
    const slots = defaultSlots();
    slots[0] = { keys: [{ button: 'a', code: 'KeyK' }] };
    emit('input.bindingsChanged', { slots });
    expect(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：K）' })).toBeTruthy();
    expect(screen.getByRole('button', { name: '绑定键盘按键：B（当前：未绑定）' })).toBeTruthy();
  });
});

describe('GamepadSettings：键盘', () => {
  it('点绑定后按下键盘上的键，保存成新的绑定', async () => {
    const { setBindings, lastSaved } = setup();
    await userEvent.click(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' }));
    expect(screen.getByRole('button', { name: /请按下键盘上的键/ })).toBeTruthy();
    fireEvent.keyDown(document.body, { code: 'KeyJ' });
    await waitFor(() => expect(setBindings).toHaveBeenCalledTimes(1));
    expect(lastSaved()[0]?.keys.find((item) => item.button === 'a')?.code).toBe('KeyJ');
    expect(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：J）' })).toBeTruthy();
  });

  it('选的物理键已经绑给别的键时，原来的绑定被解除', async () => {
    const { setBindings, lastSaved } = setup();
    await userEvent.click(await screen.findByRole('button', { name: '绑定键盘按键：B（当前：Z）' }));
    fireEvent.keyDown(document.body, { code: 'KeyX' });
    await waitFor(() => expect(setBindings).toHaveBeenCalledTimes(1));
    const keys = lastSaved()[0]?.keys ?? [];
    expect(keys.find((item) => item.button === 'b')?.code).toBe('KeyX');
    expect(keys.find((item) => item.button === 'a')).toBeUndefined();
  });

  it('按 Esc 取消学习，什么也不保存', async () => {
    const { setBindings } = setup();
    await userEvent.click(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' }));
    fireEvent.keyDown(document.body, { code: 'Escape' });
    expect(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' })).toBeTruthy();
    expect(setBindings).not.toHaveBeenCalled();
  });

  it('学习中再点一次同一个按钮取消', async () => {
    const { setBindings } = setup();
    await userEvent.click(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' }));
    await userEvent.click(screen.getByRole('button', { name: /请按下键盘上的键/ }));
    expect(screen.queryByRole('button', { name: /请按下键盘上的键/ })).toBeNull();
    expect(setBindings).not.toHaveBeenCalled();
  });

  it('清除一个键的键盘绑定；没有绑定的键“清除”不可点', async () => {
    const { setBindings, lastSaved } = setup();
    await userEvent.click(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' }));
    await userEvent.click(screen.getByRole('button', { name: '清除键盘按键：A' }));
    await waitFor(() => expect(setBindings).toHaveBeenCalledTimes(1));
    expect(lastSaved()[0]?.keys.find((item) => item.button === 'a')).toBeUndefined();
    await waitFor(() => expect((screen.getByRole('button', { name: '清除键盘按键：A' }) as HTMLButtonElement).disabled).toBe(true));
  });

  it('保存失败用通知给出核心的说明，页面保持原样', async () => {
    const { setBindings } = setup();
    setBindings.mockImplementationOnce(() => Promise.reject({ code: 'input.invalidBindings', message: '玩家 1：不合格' }));
    await userEvent.click(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' }));
    fireEvent.keyDown(document.body, { code: 'KeyJ' });
    await waitFor(() => expect(notify).toHaveBeenCalledWith('玩家 1：不合格', 'error'));
    expect(screen.getByRole('button', { name: '绑定键盘按键：A（当前：X）' })).toBeTruthy();
  });

  it('恢复默认', async () => {
    const { resetBindings } = setup();
    await userEvent.click(await screen.findByRole('button', { name: '恢复默认' }));
    expect(resetBindings).toHaveBeenCalledTimes(1);
  });
});

describe('GamepadSettings：实体手柄', () => {
  it('系统发现的手柄出现在“使用的手柄”里，选中后保存指定（按钮绑定为空）', async () => {
    plugPads(() => [fakePad(0, 'Demo Pad')]);
    const { setBindings, lastSaved } = setup();
    await showPadSource();
    await userEvent.click(await screen.findByRole('combobox', { name: '使用的手柄' }));
    await userEvent.click(screen.getByRole('option', { name: 'Demo Pad' }));
    await waitFor(() => expect(setBindings).toHaveBeenCalledTimes(1));
    expect(lastSaved()[0]?.gamepad).toEqual({ id: 'Demo Pad', occurrence: 0, bindings: [] });
    expect(await screen.findByRole('button', { name: '绑定手柄按钮：A（当前：未绑定）' })).toBeTruthy();
  });

  it('已指定但现在没接上的手柄显示“未连接”，学习按钮不可点', async () => {
    plugPads(() => []);
    const slots = defaultSlots();
    slots[0] = { ...slots[0]!, gamepad: { id: 'Gone Pad', occurrence: 0, bindings: [{ button: 'a', kind: 'button', index: 1 }] } };
    setup(() => Promise.resolve({ slots }));
    await showPadSource();
    expect((await screen.findByRole('combobox', { name: '使用的手柄' })).textContent).toContain('Gone Pad（未连接）');
    expect((screen.getByRole('button', { name: '绑定手柄按钮：A（当前：按钮 1）' }) as HTMLButtonElement).disabled).toBe(true);
  });

  it('学习手柄上的按钮：以开始时的状态为基准，之后被按下的按钮保存为绑定', async () => {
    const callbacks: FrameRequestCallback[] = [];
    vi.spyOn(window, 'requestAnimationFrame').mockImplementation((callback) => callbacks.push(callback));
    vi.spyOn(window, 'cancelAnimationFrame').mockImplementation(() => undefined);
    let pressed = [false, false, false];
    plugPads(() => [fakePad(2, 'Demo Pad', pressed)]);
    const slots = defaultSlots();
    slots[0] = { ...slots[0]!, gamepad: { id: 'Demo Pad', occurrence: 0, bindings: [] } };
    const { setBindings, lastSaved } = setup(() => Promise.resolve({ slots }));
    const tick = () => act(() => callbacks.splice(0).forEach((callback) => callback(0)));

    await showPadSource();
    await userEvent.click(await screen.findByRole('button', { name: '绑定手柄按钮：A（当前：未绑定）' }));
    expect(screen.getByRole('button', { name: /请按下手柄上的按钮/ })).toBeTruthy();
    tick();
    tick();
    expect(setBindings).not.toHaveBeenCalled();
    pressed = [false, false, true];
    tick();
    await waitFor(() => expect(setBindings).toHaveBeenCalledTimes(1));
    expect(lastSaved()[0]?.gamepad?.bindings).toEqual([{ button: 'a', kind: 'button', index: 2 }]);
  });

  it('学习手柄上的轴：推到底的方向保存为绑定', async () => {
    const callbacks: FrameRequestCallback[] = [];
    vi.spyOn(window, 'requestAnimationFrame').mockImplementation((callback) => callbacks.push(callback));
    vi.spyOn(window, 'cancelAnimationFrame').mockImplementation(() => undefined);
    let axes = [0, 0];
    plugPads(() => [fakePad(0, 'Demo Pad', [false], axes)]);
    const slots = defaultSlots();
    slots[0] = { ...slots[0]!, gamepad: { id: 'Demo Pad', occurrence: 0, bindings: [] } };
    const { setBindings, lastSaved } = setup(() => Promise.resolve({ slots }));
    const tick = () => act(() => callbacks.splice(0).forEach((callback) => callback(0)));

    await showPadSource();
    await userEvent.click(await screen.findByRole('button', { name: '绑定手柄按钮：左（当前：未绑定）' }));
    tick();
    axes = [-1, 0];
    tick();
    await waitFor(() => expect(setBindings).toHaveBeenCalledTimes(1));
    expect(lastSaved()[0]?.gamepad?.bindings).toEqual([{ button: 'left', kind: 'axis', index: 0, direction: 'negative' }]);
    expect(await screen.findByRole('button', { name: '绑定手柄按钮：左（当前：轴 0 负向）' })).toBeTruthy();
  });

  it('清除手柄上的绑定', async () => {
    plugPads(() => [fakePad(0, 'Demo Pad')]);
    const slots = defaultSlots();
    slots[0] = { ...slots[0]!, gamepad: { id: 'Demo Pad', occurrence: 0, bindings: [{ button: 'a', kind: 'button', index: 1 }] } };
    const { setBindings, lastSaved } = setup(() => Promise.resolve({ slots }));
    await showPadSource();
    await userEvent.click(await screen.findByRole('button', { name: '绑定手柄按钮：A（当前：按钮 1）' }));
    await userEvent.click(screen.getByRole('button', { name: '清除手柄按钮：A' }));
    await waitFor(() => expect(setBindings).toHaveBeenCalledTimes(1));
    expect(lastSaved()[0]?.gamepad?.bindings).toEqual([]);
  });

  it('选“不使用”取消指定的手柄', async () => {
    plugPads(() => [fakePad(0, 'Demo Pad')]);
    const slots = defaultSlots();
    slots[0] = { ...slots[0]!, gamepad: { id: 'Demo Pad', occurrence: 0, bindings: [] } };
    const { setBindings, lastSaved } = setup(() => Promise.resolve({ slots }));
    await showPadSource();
    await userEvent.click(await screen.findByRole('combobox', { name: '使用的手柄' }));
    await userEvent.click(screen.getByRole('option', { name: '不使用' }));
    await waitFor(() => expect(setBindings).toHaveBeenCalledTimes(1));
    expect(lastSaved()[0]?.gamepad).toBeUndefined();
  });
});

describe('GamepadSettings：手柄图', () => {
  it('图上有手柄的 10 个键，各自写着键名与当前的键盘按键', async () => {
    setup();
    await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' });
    for (const name of ['上', '下', '左', '右', 'A', 'B', '连发 A', '连发 B', '选择', '开始']) {
      expect(screen.getByRole('button', { name: new RegExp(`^绑定键盘按键：${name}（`) }), name).toBeTruthy();
    }
    expect(screen.getAllByRole('button', { name: /^绑定键盘按键：/ })).toHaveLength(10);
  });

  it('点图上的键后选中它，下方说明行显示它的当前绑定；点另一个键就换成另一个', async () => {
    setup();
    await userEvent.click(await screen.findByRole('button', { name: '绑定键盘按键：B（当前：Z）' }));
    fireEvent.keyDown(document.body, { code: 'Escape' });
    expect(await screen.findByText('B：Z')).toBeTruthy();
    expect(screen.getByRole('button', { name: '绑定键盘按键：B（当前：Z）' }).getAttribute('aria-pressed')).toBe('true');
    await userEvent.click(screen.getByRole('button', { name: '绑定键盘按键：开始（当前：回车）' }));
    expect(screen.getByText(/设置“开始”/)).toBeTruthy();
    expect(screen.getByRole('button', { name: '绑定键盘按键：B（当前：Z）' }).getAttribute('aria-pressed')).toBe('false');
  });

  it('学习中点另一个键，改为学习那个键，不保存前一个', async () => {
    const { setBindings, lastSaved } = setup();
    await userEvent.click(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' }));
    await userEvent.click(screen.getByRole('button', { name: '绑定键盘按键：B（当前：Z）' }));
    expect(setBindings).not.toHaveBeenCalled();
    fireEvent.keyDown(document.body, { code: 'KeyJ' });
    await waitFor(() => expect(setBindings).toHaveBeenCalledTimes(1));
    const keys = lastSaved()[0]?.keys ?? [];
    expect(keys.find((item) => item.button === 'b')?.code).toBe('KeyJ');
    expect(keys.find((item) => item.button === 'a')?.code).toBe('KeyX');
  });

  it('切换到“实体手柄”只改显示：图上显示手柄的绑定，切回键盘时键盘的绑定不变', async () => {
    plugPads(() => [fakePad(0, 'Demo Pad')]);
    const slots = defaultSlots();
    slots[0] = { ...slots[0]!, gamepad: { id: 'Demo Pad', occurrence: 0, bindings: [{ button: 'a', kind: 'button', index: 1 }] } };
    setup(() => Promise.resolve({ slots }));
    await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' });
    await showPadSource();
    expect(await screen.findByRole('button', { name: '绑定手柄按钮：A（当前：按钮 1）' })).toBeTruthy();
    expect(screen.queryByRole('button', { name: /^绑定键盘按键/ })).toBeNull();
    await userEvent.click(screen.getByRole('radio', { name: '键盘' }));
    expect(await screen.findByRole('button', { name: '绑定键盘按键：A（当前：X）' })).toBeTruthy();
  });

  it('实体手柄来源还没有指定手柄时，图上的键不可点，说明里提示先选手柄', async () => {
    plugPads(() => [fakePad(0, 'Demo Pad')]);
    setup();
    await showPadSource();
    const key = (await screen.findByRole('button', { name: '绑定手柄按钮：A（当前：未绑定）' })) as HTMLButtonElement;
    expect(key.disabled).toBe(true);
    expect(screen.getByText('先在上面选择要使用的手柄。')).toBeTruthy();
  });
});
