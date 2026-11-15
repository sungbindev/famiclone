/*
 * ------------------------------------------------------------------------
 * 名称：MediaPanel.test.tsx
 * 说明：设备面板的测试：没有设备时的提示、每个设备一行（软驱的名称、盘片、状态与四个图标按钮，打印机的名称与状态）、按钮的可用性与点击发出的请求、写保护是按下态的锁按钮、显示期间随状态变化。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的核心接口与真实的控制器；按钮的请求经控制器发往假核心，结果取自假核心的记录。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, render, screen, within } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { describe, expect, it } from 'vitest';
import type { FloppyStatus } from '../../contracts/media/generated/media';
import { FakeMediaBackend, floppyStatus } from './media-fakes';
import { MediaController } from './media-controller';
import { MediaPanel } from './MediaPanel';

/** 等已发出的请求与它们的后续处理完成（微任务队列清空）。 */
async function settle(): Promise<void> {
  for (let index = 0; index < 8; index += 1) await Promise.resolve();
}

/** 接上假核心并渲染面板；floppies 是软驱的初始状态。 */
async function setup(floppies?: FloppyStatus[]) {
  const fake = new FakeMediaBackend();
  if (floppies !== undefined) fake.floppies = floppies;
  const controller = new MediaController();
  controller.attach(fake.backend, { notify: () => undefined, refresh: () => undefined });
  render(
    <UiProvider>
      <MediaPanel controller={controller} />
    </UiProvider>
  );
  await act(async () => settle());
  return { fake, controller };
}

/** 一个设备的行（按名称找）。 */
function row(name: string): HTMLElement {
  return screen.getByText(name).closest('li') as HTMLElement;
}

/** 行里的盘片（主文字）与状态（信息列）。 */
function fields(root: HTMLElement): { disk: string; state: string } {
  return { disk: root.querySelector('.ui-row-list__title')?.textContent ?? '', state: root.querySelector('.ui-row-list__cell')?.textContent ?? '' };
}

/** 软驱 A 那一行的写保护按钮。 */
function protectButton(): HTMLElement {
  return within(row('软驱 A')).getByRole('button', { name: '写保护' });
}

describe('设备面板', () => {
  it('没有设备（没有开机或主机没有软驱）时只给一句提示', async () => {
    await setup([]);
    expect(screen.getByText('没有设备：整机机型开机后才有软驱与打印机。')).toBeTruthy();
    expect(screen.queryByText('软驱 A')).toBeNull();
  });

  it('软驱 A、软驱 B 与打印机各一行，没有卡片与标题', async () => {
    await setup();
    const rows = within(screen.getByRole('list', { name: '设备' })).getAllByRole('listitem');
    expect(rows.map((item) => item.querySelector('.ui-row-list__leading')?.textContent)).toEqual(['软驱 A', '软驱 B', '打印机']);
    expect(screen.queryByRole('heading')).toBeNull();
  });

  it('没有盘片：盘片“空”、状态“无盘片”，只有“载入镜像”可用，写保护不可用', async () => {
    await setup();
    const drive = row('软驱 A');
    expect(fields(drive)).toEqual({ disk: '空', state: '无盘片' });
    expect((within(drive).getByRole('button', { name: '载入镜像' }) as HTMLButtonElement).disabled).toBe(false);
    for (const name of ['取出', '保存镜像', '写保护']) expect((within(drive).getByRole('button', { name }) as HTMLButtonElement).disabled).toBe(true);
  });

  it('有盘片：显示镜像名与状态，按钮全部可用；读写中带磁道，有未保存的修改时注明', async () => {
    await setup([floppyStatus(0, 'sys.img', { motor: true, cylinder: 7, modified: true }), floppyStatus(1)]);
    const drive = row('软驱 A');
    expect(fields(drive)).toEqual({ disk: 'sys.img（有未保存的修改）', state: '读写中（磁道 7）' });
    for (const name of ['写保护', '载入镜像', '取出', '保存镜像']) expect((within(drive).getByRole('button', { name }) as HTMLButtonElement).disabled).toBe(false);
  });

  it('按钮只有图标，没有文字', async () => {
    await setup([floppyStatus(0, 'sys.img'), floppyStatus(1)]);
    for (const button of within(row('软驱 A')).getAllByRole('button')) {
      expect(button.textContent).toBe('');
      expect(button.getAttribute('title')).toBe(button.getAttribute('aria-label'));
    }
  });

  it('点“载入镜像”选定文件后盘片显示镜像名', async () => {
    const { fake } = await setup();
    fake.pickedFile = 'sys.img';
    await userEvent.click(within(row('软驱 B')).getByRole('button', { name: '载入镜像' }));
    await act(async () => settle());
    expect(fake.calls).toContain('media.openFloppy {"drive":1}');
    expect(fields(row('软驱 B')).disk).toBe('sys.img');
  });

  it('点“取出”与“保存镜像”发出对应的请求', async () => {
    const { fake } = await setup([floppyStatus(0, 'sys.img', { modified: true }), floppyStatus(1)]);
    await userEvent.click(within(row('软驱 A')).getByRole('button', { name: '保存镜像' }));
    await act(async () => settle());
    expect(fake.calls).toContain('media.floppySave {"drive":0}');
    expect(fields(row('软驱 A')).disk).toBe('sys.img');
    await userEvent.click(within(row('软驱 A')).getByRole('button', { name: '取出' }));
    await act(async () => settle());
    expect(fake.calls).toContain('media.floppyEject {"drive":0}');
    expect(fields(row('软驱 A')).disk).toBe('空');
  });

  it('写保护是按下态的锁按钮：点一下发出请求，按下态随状态变化', async () => {
    const { fake } = await setup([floppyStatus(0, 'sys.img'), floppyStatus(1)]);
    expect(protectButton().getAttribute('aria-pressed')).toBe('false');
    await userEvent.click(protectButton());
    await act(async () => settle());
    expect(fake.calls).toContain('media.floppyWriteProtect {"drive":0,"writeProtected":true}');
    expect(protectButton().getAttribute('aria-pressed')).toBe('true');
  });

  it('打印机的状态随核心的状态变化：空闲、打印中、打印完成', async () => {
    const { fake } = await setup();
    expect(row('打印机').querySelector('.ui-row-list__title')?.textContent).toBe('空闲');
    fake.printer = { state: 'printing', hasContent: true, pages: 0 };
    fake.emit('runtime.stateChanged', { power: 'on', mode: 'running', reason: 'powerOn' });
    await act(async () => settle());
    expect(row('打印机').querySelector('.ui-row-list__title')?.textContent).toBe('打印中');
    fake.printer = { state: 'done', hasContent: false, pages: 1, lastFile: 'p.png' };
    fake.emit('media.printed', { fileName: 'p.png', page: 1 });
    await act(async () => settle());
    expect(row('打印机').querySelector('.ui-row-list__title')?.textContent).toBe('打印完成');
  });
});
