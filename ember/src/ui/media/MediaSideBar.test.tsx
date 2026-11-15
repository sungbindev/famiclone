/*
 * ------------------------------------------------------------------------
 * 名称：MediaSideBar.test.tsx
 * 说明：设备边栏的测试：没有设备时的提示、软驱卡片（盘片、状态、写保护开关、按钮的可用性与点击发出的请求）、打印机卡片的状态、显示期间定时读取并随状态变化。
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
import { MediaSideBar } from './MediaSideBar';

/** 等已发出的请求与它们的后续处理完成（微任务队列清空）。 */
async function settle(): Promise<void> {
  for (let index = 0; index < 8; index += 1) await Promise.resolve();
}

/** 接上假核心并渲染边栏；floppies 是软驱的初始状态。 */
async function setup(floppies?: FloppyStatus[]) {
  const fake = new FakeMediaBackend();
  if (floppies !== undefined) fake.floppies = floppies;
  const controller = new MediaController();
  controller.attach(fake.backend, { addMessage: () => undefined, notify: () => undefined, refresh: () => undefined });
  render(
    <UiProvider>
      <MediaSideBar controller={controller} />
    </UiProvider>
  );
  await act(async () => settle());
  return { fake, controller };
}

/** 一张卡片（按标题找）。 */
function card(title: string): HTMLElement {
  return screen.getByRole('heading', { name: title }).closest('section') as HTMLElement;
}

/** 卡片里某一行的值。 */
function fact(root: HTMLElement, label: string): string {
  const row = within(root).getByText(label, { selector: 'dt' }).parentElement as HTMLElement;
  return within(row).getByText((_text, element) => element?.tagName === 'DD').textContent ?? '';
}

/** 软驱 A 卡片里的写保护开关。 */
function toggle(): HTMLElement {
  return within(card('软驱 A')).getByRole('switch', { name: '写保护' });
}

describe('设备边栏', () => {
  it('没有设备（没有开机或主机没有软驱）时只给一句提示', async () => {
    await setup([]);
    expect(screen.getByText('没有设备：整机机型开机后才有软驱与打印机。')).toBeTruthy();
    expect(screen.queryByRole('heading', { name: '软驱 A' })).toBeNull();
  });

  it('软驱 A、软驱 B 与打印机各一张卡片', async () => {
    await setup();
    expect(screen.getAllByRole('heading').map((item) => item.textContent)).toEqual(['软驱 A', '软驱 B', '打印机']);
  });

  it('没有盘片：盘片“空”、状态“无盘片”，只有“载入镜像”可用', async () => {
    await setup();
    const drive = card('软驱 A');
    expect([fact(drive, '盘片'), fact(drive, '状态')]).toEqual(['空', '无盘片']);
    expect((within(drive).getByRole('button', { name: '载入镜像' }) as HTMLButtonElement).disabled).toBe(false);
    for (const name of ['取出', '保存镜像']) expect((within(drive).getByRole('button', { name }) as HTMLButtonElement).disabled).toBe(true);
    expect((within(drive).getByRole('switch', { name: '写保护' }) as HTMLInputElement).disabled).toBe(true);
  });

  it('有盘片：显示镜像名与状态，按钮全部可用；读写中带磁道，有未保存的修改时注明', async () => {
    await setup([floppyStatus(0, 'sys.img', { motor: true, cylinder: 7, modified: true }), floppyStatus(1)]);
    const drive = card('软驱 A');
    expect([fact(drive, '盘片'), fact(drive, '状态')]).toEqual(['sys.img（有未保存的修改）', '读写中（磁道 7）']);
    for (const name of ['载入镜像', '取出', '保存镜像']) expect((within(drive).getByRole('button', { name }) as HTMLButtonElement).disabled).toBe(false);
  });

  it('点“载入镜像”选定文件后盘片显示镜像名', async () => {
    const { fake } = await setup();
    fake.pickedFile = 'sys.img';
    await userEvent.click(within(card('软驱 B')).getByRole('button', { name: '载入镜像' }));
    await act(async () => settle());
    expect(fake.calls).toContain('media.openFloppy {"drive":1}');
    expect(fact(card('软驱 B'), '盘片')).toBe('sys.img');
  });

  it('点“取出”与“保存镜像”发出对应的请求', async () => {
    const { fake } = await setup([floppyStatus(0, 'sys.img', { modified: true }), floppyStatus(1)]);
    const drive = card('软驱 A');
    await userEvent.click(within(drive).getByRole('button', { name: '保存镜像' }));
    await act(async () => settle());
    expect(fake.calls).toContain('media.floppySave {"drive":0}');
    expect(fact(card('软驱 A'), '盘片')).toBe('sys.img');
    await userEvent.click(within(card('软驱 A')).getByRole('button', { name: '取出' }));
    await act(async () => settle());
    expect(fake.calls).toContain('media.floppyEject {"drive":0}');
    expect(fact(card('软驱 A'), '盘片')).toBe('空');
  });

  it('写保护开关发出对应的请求并随状态变化', async () => {
    const { fake } = await setup([floppyStatus(0, 'sys.img'), floppyStatus(1)]);
    expect(toggle().getAttribute('aria-checked')).toBe('false');
    await userEvent.click(toggle());
    await act(async () => settle());
    expect(fake.calls).toContain('media.floppyWriteProtect {"drive":0,"writeProtected":true}');
    expect(toggle().getAttribute('aria-checked')).toBe('true');
  });

  it('打印机的状态随核心的状态变化：空闲、打印中、打印完成', async () => {
    const { fake } = await setup();
    expect(fact(card('打印机'), '状态')).toBe('空闲');
    fake.printer = { state: 'printing', hasContent: true, pages: 0 };
    fake.emit('runtime.stateChanged', { power: 'on', mode: 'running', reason: 'powerOn' });
    await act(async () => settle());
    expect(fact(card('打印机'), '状态')).toBe('打印中');
    fake.printer = { state: 'done', hasContent: false, pages: 1, lastFile: 'p.png' };
    fake.emit('media.printed', { fileName: 'p.png', page: 1 });
    await act(async () => settle());
    expect(fact(card('打印机'), '状态')).toBe('打印完成');
  });
});
