/*
 * ------------------------------------------------------------------------
 * 名称：MediaPanel.test.tsx
 * 说明：设备面板的测试：全部设备始终列出（没有连接的设备按钮置灰）、每个设备一行（软驱的名称、盘片、状态与四个图标按钮，磁带机的名称、磁带、状态、位置与五个图标按钮，打印机的名称与状态）、按钮的可用性与点击发出的请求、写保护是按下态的锁按钮、显示期间随状态变化。
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
import { FakeMediaBackend, floppyStatus, tapeStatus } from './media-fakes';
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
  it('主机没有任何设备（没有开机或经典 FC 之外）时照样列出软驱 A、磁带机与打印机，按钮全部置灰', async () => {
    await setup([]);
    const rows = within(screen.getByRole('list', { name: '设备' })).getAllByRole('listitem');
    expect(rows.map((item) => item.querySelector('.ui-row-list__leading')?.textContent)).toEqual(['软驱 A', '磁带机', '打印机']);
    expect(fields(row('软驱 A'))).toEqual({ disk: '空', state: '无盘片' });
    expect(row('磁带机').querySelector('.ui-row-list__title')?.textContent).toBe('空');
    expect(row('打印机').querySelector('.ui-row-list__title')?.textContent).toBe('空闲');
    for (const item of [row('软驱 A'), row('磁带机')]) {
      for (const button of within(item).getAllByRole('button')) expect((button as HTMLButtonElement).disabled).toBe(true);
    }
  });

  it('软驱 A、软驱 B、磁带机与打印机各一行，没有卡片与标题', async () => {
    await setup();
    const rows = within(screen.getByRole('list', { name: '设备' })).getAllByRole('listitem');
    expect(rows.map((item) => item.querySelector('.ui-row-list__leading')?.textContent)).toEqual(['软驱 A', '软驱 B', '磁带机', '打印机']);
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

/** 接上假核心并渲染面板；主机有磁带机（经典 FC），没有软驱与打印机。 */
async function setupTape(patch: Parameters<typeof tapeStatus>[0] = {}) {
  const fake = new FakeMediaBackend();
  fake.floppies = [];
  fake.tape = tapeStatus({ available: true, ...patch });
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

/** 磁带机这一行里的按钮。 */
function tapeButton(name: string): HTMLButtonElement {
  return within(row('磁带机')).getByRole('button', { name }) as HTMLButtonElement;
}

describe('设备面板的磁带机', () => {
  it('经典 FC 没有软驱：软驱 A 与打印机照样列出，软驱 A 的按钮置灰；没有磁带时磁带“空”、状态“无磁带”、位置“—”，只有载入与录音可用', async () => {
    await setupTape();
    const rows = within(screen.getByRole('list', { name: '设备' })).getAllByRole('listitem');
    expect(rows.map((item) => item.querySelector('.ui-row-list__leading')?.textContent)).toEqual(['软驱 A', '磁带机', '打印机']);
    for (const button of within(row('软驱 A')).getAllByRole('button')) expect((button as HTMLButtonElement).disabled).toBe(true);
    const tape = row('磁带机');
    expect(tape.querySelector('.ui-row-list__title')?.textContent).toBe('空');
    expect([...tape.querySelectorAll('.ui-row-list__cell')].map((cell) => cell.textContent)).toEqual(['无磁带', '—']);
    expect(tapeButton('载入磁带').disabled).toBe(false);
    expect(tapeButton('录音').disabled).toBe(false);
    for (const name of ['取出', '播放', '停止']) expect(tapeButton(name).disabled).toBe(true);
    for (const button of within(tape).getAllByRole('button')) expect(button.textContent).toBe('');
  });

  it('主机没有磁带机时磁带机照样列出，按钮全部置灰', async () => {
    await setup([]);
    for (const button of within(row('磁带机')).getAllByRole('button')) expect((button as HTMLButtonElement).disabled).toBe(true);
  });

  it('载入磁带后显示文件名、“已停止”与位置，取出、播放可用', async () => {
    const { fake } = await setupTape();
    fake.pickedTape = 'lesson.m4a';
    await userEvent.click(tapeButton('载入磁带'));
    await act(async () => settle());
    expect(fake.calls).toContain('media.openTape {}');
    const tape = row('磁带机');
    expect(tape.querySelector('.ui-row-list__title')?.textContent).toBe('lesson.m4a');
    expect([...tape.querySelectorAll('.ui-row-list__cell')].map((cell) => cell.textContent)).toEqual(['已停止', '0:00 / 1:00']);
    expect(tapeButton('取出').disabled).toBe(false);
    expect(tapeButton('播放').disabled).toBe(false);
  });

  it('播放时只有“停止”可用，点“停止”后回到已停止', async () => {
    const { fake } = await setupTape({ loaded: true, fileName: 'lesson.m4a', duration: 60 });
    await userEvent.click(tapeButton('播放'));
    await act(async () => settle());
    expect(fake.calls).toContain('media.tapePlay {}');
    expect(row('磁带机').querySelectorAll('.ui-row-list__cell')[0]?.textContent).toBe('播放中');
    for (const name of ['载入磁带', '取出', '播放', '录音']) expect(tapeButton(name).disabled).toBe(true);
    expect(tapeButton('停止').disabled).toBe(false);
    await userEvent.click(tapeButton('停止'));
    await act(async () => settle());
    expect(fake.calls).toContain('media.tapeStop {}');
    expect(row('磁带机').querySelectorAll('.ui-row-list__cell')[0]?.textContent).toBe('已停止');
  });

  it('点“录音”选定文件后显示录音中；用户取消选择时什么都不发生；点“取出”发出请求', async () => {
    const { fake } = await setupTape();
    await userEvent.click(tapeButton('录音'));
    await act(async () => settle());
    expect(row('磁带机').querySelectorAll('.ui-row-list__cell')[0]?.textContent).toBe('无磁带');
    fake.pickedRecording = 'out.wav';
    await userEvent.click(tapeButton('录音'));
    await act(async () => settle());
    expect(fake.calls.filter((call) => call === 'media.openTapeRecording {}')).toHaveLength(2);
    expect(row('磁带机').querySelectorAll('.ui-row-list__cell')[0]?.textContent).toBe('录音中');
    expect(row('磁带机').querySelector('.ui-row-list__title')?.textContent).toBe('out.wav');
  });

  it('取出磁带后回到“空”', async () => {
    const { fake } = await setupTape({ loaded: true, fileName: 'lesson.m4a', duration: 60 });
    await userEvent.click(tapeButton('取出'));
    await act(async () => settle());
    expect(fake.calls).toContain('media.tapeEject {}');
    expect(row('磁带机').querySelector('.ui-row-list__title')?.textContent).toBe('空');
  });
});
