/*
 * ------------------------------------------------------------------------
 * 名称：PreferencesPage.test.tsx
 * 说明：首选项页面的测试：读取并按分组显示设置项（隐藏项与没有显示项的分组不显示）、选择与开关立即保存、整数输入的提交与校验、恢复默认、保存失败用通知提示且控件保持原值、
 *       在别处修改（事件）后页面更新、核心重启后重新读取、读取失败的提示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, render, screen, waitFor, within } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { PreferenceGroup, PreferenceSetting, PreferencesListResult } from '../../contracts/preferences/generated/preferences';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { PreferencesPage } from './PreferencesPage';
import { ShellApiContext } from '../shell/shell-context';
import type { ShellApi } from '../shell/shell-registry';

const notify = vi.fn();
const shell = { notify } as unknown as ShellApi;

const GROUPS: PreferenceGroup[] = [
  { id: 'appearance', label: '外观' },
  { id: 'sound', label: '声音' },
  { id: 'printer', label: '打印机' }
];

function initialSettings(): PreferenceSetting[] {
  return [
    {
      id: 'appearance.theme',
      group: 'appearance',
      label: '主题',
      description: '跟随系统时，系统切换深色或浅色后立即生效。',
      kind: 'choice',
      options: [
        { value: 'system', label: '跟随系统' },
        { value: 'dark', label: '深色' },
        { value: 'light', label: '浅色' }
      ],
      hidden: false,
      defaultValue: 'system',
      value: 'system'
    },
    { id: 'sound.startMuted', group: 'sound', label: '启动时静音', description: '音量在运行面板调整。', kind: 'switch', hidden: false, defaultValue: false, value: false },
    { id: 'sound.fade', group: 'sound', label: '淡入毫秒', description: '', kind: 'integer', minimum: 0, maximum: 100, hidden: false, defaultValue: 10, value: 10 },
    { id: 'sound.note', group: 'sound', label: '备注', description: '', kind: 'text', maxLength: 8, hidden: false, defaultValue: '', value: 'abc' },
    { id: 'sound.volume', group: 'sound', label: '音量', description: '', kind: 'integer', minimum: 0, maximum: 100, hidden: true, defaultValue: 100, value: 100 },
    { id: 'printer.outputDirectory', group: 'printer', label: '图片保存位置', description: '', kind: 'text', maxLength: 260, picker: 'directory', hidden: false, defaultValue: '', value: '' }
  ];
}

/** 假预加载接口：记录请求，可以主动触发事件与状态变化。 */
function setup(listResult: () => Promise<PreferencesListResult> = () => Promise.resolve({ groups: GROUPS, settings: initialSettings() })) {
  const eventListeners = new Set<(name: string, data: unknown) => void>();
  const statusListeners = new Set<(status: CoreStatus) => void>();
  const list = vi.fn(listResult);
  const set = vi.fn((params: { id: string; value: unknown }) => Promise.resolve({ id: params.id, value: params.value }));
  const reset = vi.fn((params: { id: string }) => Promise.resolve({ id: params.id, value: initialSettings().find((item) => item.id === params.id)?.defaultValue }));
  const pickDirectory = vi.fn((): Promise<{ canceled: boolean; path?: string }> => Promise.resolve({ canceled: false, path: 'D:/prints' }));
  const bridge = {
    core: { preferences: { list, set, reset } },
    main: { preferences: { pickDirectory } },
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
        <PreferencesPage bridge={bridge} />
      </ShellApiContext.Provider>
    </UiProvider>
  );
  return {
    list,
    set,
    reset,
    pickDirectory,
    emit: (name: string, data: unknown) => act(() => eventListeners.forEach((listener) => listener(name, data))),
    setStatus: (status: CoreStatus) => act(() => statusListeners.forEach((listener) => listener(status)))
  };
}

beforeEach(() => notify.mockClear());
afterEach(() => vi.restoreAllMocks());

describe('PreferencesPage：显示', () => {
  it('读取后显示有显示项的分组，默认选中第一个，右边是它的设置项（名称与说明）', async () => {
    setup();
    expect(await screen.findByText('主题')).toBeTruthy();
    expect(screen.getByText('跟随系统时，系统切换深色或浅色后立即生效。')).toBeTruthy();
    expect(screen.getByRole('combobox', { name: '主题' }).textContent).toContain('跟随系统');
    expect(screen.queryByText('启动时静音')).toBeNull();
  });

  it('点另一个分组切换设置项；隐藏的设置项不显示', async () => {
    setup();
    await screen.findByText('主题');
    await userEvent.click(screen.getByText('声音'));
    expect(await screen.findByText('启动时静音')).toBeTruthy();
    expect(screen.getByText('淡入毫秒')).toBeTruthy();
    expect(screen.queryByText('音量')).toBeNull();
    expect(screen.queryByText('主题')).toBeNull();
  });

  it('读取中显示提示，读取失败显示核心给出的说明', async () => {
    setup(() => Promise.reject({ code: 'core.unavailable', message: '核心未启动' }));
    expect(await screen.findByText('核心未启动')).toBeTruthy();
  });
});

describe('PreferencesPage：修改', () => {
  it('文件夹类设置项的“选择”按钮：弹出选择文件夹窗口，选定的路径立即保存', async () => {
    const page = setup();
    await userEvent.click(await screen.findByText('打印机'));
    await userEvent.click(await screen.findByRole('button', { name: '选择文件夹：图片保存位置' }));
    expect(page.pickDirectory).toHaveBeenCalledTimes(1);
    expect(page.set).toHaveBeenCalledWith({ id: 'printer.outputDirectory', value: 'D:/prints' });
    await waitFor(() => expect((screen.getByRole('textbox', { name: '图片保存位置' }) as HTMLInputElement).value).toBe('D:/prints'));
  });

  it('在选择文件夹的窗口里取消：不保存，也没有通知', async () => {
    const page = setup();
    page.pickDirectory.mockResolvedValueOnce({ canceled: true });
    await userEvent.click(await screen.findByText('打印机'));
    await userEvent.click(await screen.findByRole('button', { name: '选择文件夹：图片保存位置' }));
    await waitFor(() => expect(page.pickDirectory).toHaveBeenCalledTimes(1));
    expect(page.set).not.toHaveBeenCalled();
    expect(notify).not.toHaveBeenCalled();
  });

  it('没有 picker 的文字设置项没有“选择”按钮', async () => {
    setup();
    await userEvent.click(await screen.findByText('声音'));
    await screen.findByText('备注');
    expect(screen.queryByRole('button', { name: /选择文件夹/ })).toBeNull();
  });

  it('选择一个值立即保存，保存后显示新值与“恢复默认”', async () => {
    const page = setup();
    await userEvent.click(await screen.findByRole('combobox', { name: '主题' }));
    await userEvent.click(screen.getByRole('option', { name: '深色' }));
    expect(page.set).toHaveBeenCalledWith({ id: 'appearance.theme', value: 'dark' });
    await waitFor(() => expect(screen.getByRole('combobox', { name: '主题' }).textContent).toContain('深色'));
    expect(screen.getByRole('button', { name: '恢复默认：主题' })).toBeTruthy();
    expect(notify).not.toHaveBeenCalled();
  });

  it('恢复默认把取值改回出厂默认，按钮随之消失', async () => {
    const page = setup();
    await userEvent.click(await screen.findByRole('combobox', { name: '主题' }));
    await userEvent.click(screen.getByRole('option', { name: '浅色' }));
    await userEvent.click(await screen.findByRole('button', { name: '恢复默认：主题' }));
    expect(page.reset).toHaveBeenCalledWith({ id: 'appearance.theme' });
    await waitFor(() => expect(screen.queryByRole('button', { name: '恢复默认：主题' })).toBeNull());
    expect(screen.getByRole('combobox', { name: '主题' }).textContent).toContain('跟随系统');
  });

  it('开关立即保存布尔值', async () => {
    const page = setup();
    await screen.findByText('主题');
    await userEvent.click(screen.getByText('声音'));
    await userEvent.click(await screen.findByRole('checkbox', { name: '启动时静音' }));
    expect(page.set).toHaveBeenCalledWith({ id: 'sound.startMuted', value: true });
  });

  it('整数在按回车时提交；超出范围或不是整数时提示且不提交', async () => {
    const page = setup();
    await screen.findByText('主题');
    await userEvent.click(screen.getByText('声音'));
    const input = await screen.findByRole('textbox', { name: '淡入毫秒' });
    await userEvent.clear(input);
    await userEvent.type(input, '500{Enter}');
    expect(await screen.findByText('请输入 0 至 100 之间的整数。')).toBeTruthy();
    expect(page.set).not.toHaveBeenCalled();
    await userEvent.clear(input);
    await userEvent.type(input, 'abc{Enter}');
    expect(await screen.findByText('请输入整数。')).toBeTruthy();
    await userEvent.clear(input);
    await userEvent.type(input, '30{Enter}');
    await waitFor(() => expect(page.set).toHaveBeenCalledWith({ id: 'sound.fade', value: 30 }));
  });

  it('文字在按回车时原样提交；改回原值时不提交，已经不是默认值时出现“恢复默认”', async () => {
    const page = setup();
    await screen.findByText('主题');
    await userEvent.click(screen.getByText('声音'));
    const input = await screen.findByRole('textbox', { name: '备注' });
    expect((input as HTMLInputElement).value).toBe('abc');
    expect(screen.getByRole('button', { name: '恢复默认：备注' })).toBeTruthy();
    await userEvent.clear(input);
    await userEvent.type(input, '新 备注{Enter}');
    await waitFor(() => expect(page.set).toHaveBeenCalledWith({ id: 'sound.note', value: '新 备注' }));
    page.set.mockClear();
    await userEvent.clear(input);
    await userEvent.type(input, '新 备注{Enter}');
    expect(page.set).not.toHaveBeenCalled();
  });

  it('保存失败时用通知提示核心的说明，控件保持原值', async () => {
    const page = setup();
    page.set.mockRejectedValueOnce({ code: 'preferences.invalidValue', message: '主题：取值必须是下面之一：system、dark、light' });
    await userEvent.click(await screen.findByRole('combobox', { name: '主题' }));
    await userEvent.click(screen.getByRole('option', { name: '深色' }));
    await waitFor(() => expect(notify).toHaveBeenCalledWith('主题：取值必须是下面之一：system、dark、light', 'error'));
    expect(screen.getByRole('combobox', { name: '主题' }).textContent).toContain('跟随系统');
    expect(screen.queryByRole('button', { name: '恢复默认：主题' })).toBeNull();
  });
});

describe('PreferencesPage：保持最新', () => {
  it('在别处修改（核心发来 preferences.changed）后页面立即更新', async () => {
    const page = setup();
    await screen.findByText('主题');
    page.emit('preferences.changed', { id: 'appearance.theme', value: 'light' });
    await waitFor(() => expect(screen.getByRole('combobox', { name: '主题' }).textContent).toContain('浅色'));
    page.emit('preferences.changed', { id: 'appearance.nothing', value: 1 });
    page.emit('runtime.stateChanged', { id: 'appearance.theme', value: 'dark' });
    page.emit('preferences.changed', null);
    expect(screen.getByRole('combobox', { name: '主题' }).textContent).toContain('浅色');
  });

  it('核心进程重启后重新读取', async () => {
    const page = setup();
    await screen.findByText('主题');
    expect(page.list).toHaveBeenCalledTimes(1);
    page.setStatus({ state: 'starting' });
    page.setStatus({ state: 'ready', coreVersion: '0.1.0' });
    await waitFor(() => expect(page.list).toHaveBeenCalledTimes(2));
    expect(await screen.findByText('主题')).toBeTruthy();
  });

  it('页面里不显示操作结果文字', async () => {
    setup();
    await userEvent.click(await screen.findByRole('combobox', { name: '主题' }));
    await userEvent.click(screen.getByRole('option', { name: '深色' }));
    await waitFor(() => expect(screen.getByRole('combobox', { name: '主题' }).textContent).toContain('深色'));
    expect(within(document.body).queryByText(/已保存|保存成功|成功/)).toBeNull();
  });
});
