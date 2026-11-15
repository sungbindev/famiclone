/*
 * ------------------------------------------------------------------------
 * 名称：MediaStatusItems.test.tsx
 * 说明：系统栏软盘项的测试：载入盘片后显示镜像名（有未保存的修改时注明），没有载入盘片或没有软驱时不显示，多个软驱时带驱动器名，取出后消失。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的核心接口与真实的控制器。
 * ------------------------------------------------------------------------
 */

import { UiProvider } from '@rujianjs/ui-kit';
import { act, render, screen } from '@testing-library/react';
import { describe, expect, it } from 'vitest';
import { FakeMediaBackend, floppyStatus } from './media-fakes';
import { MediaController } from './media-controller';
import { FloppyStatusItem } from './MediaStatusItems';

/** 等已发出的请求与它们的后续处理完成。 */
async function settle(): Promise<void> {
  for (let index = 0; index < 8; index += 1) await Promise.resolve();
}

/** 接上假核心并渲染软盘项。 */
async function setup(floppies: ReturnType<typeof floppyStatus>[]) {
  const fake = new FakeMediaBackend();
  fake.floppies = floppies;
  const controller = new MediaController();
  controller.attach(fake.backend, { notify: () => undefined, refresh: () => undefined });
  render(
    <UiProvider>
      <FloppyStatusItem controller={controller} />
    </UiProvider>
  );
  await act(async () => settle());
  return { fake };
}

describe('系统栏的软盘项', () => {
  it('载入盘片后显示镜像名，悬停提示是“软驱 A 的盘片”', async () => {
    await setup([floppyStatus(0, 'FLPPY1.IMG')]);
    expect(screen.getByText('FLPPY1.IMG').closest('[title]')?.getAttribute('title')).toBe('软驱 A 的盘片');
  });

  it('有未保存的修改时在镜像名后注明', async () => {
    await setup([floppyStatus(0, 'FLPPY1.IMG', { modified: true })]);
    expect(screen.getByText('FLPPY1.IMG（有未保存的修改）')).toBeTruthy();
  });

  it('没有载入盘片时不显示', async () => {
    await setup([floppyStatus(0)]);
    expect(screen.queryByTitle(/的盘片/)).toBeNull();
  });

  it('有多个软驱时每项前面带驱动器名，没有盘片的软驱不显示', async () => {
    await setup([floppyStatus(0, 'A.IMG'), floppyStatus(1), floppyStatus(2, 'C.IMG')]);
    expect(screen.getByText('A: A.IMG')).toBeTruthy();
    expect(screen.getByText('C: C.IMG')).toBeTruthy();
    expect(screen.queryByText(/B:/)).toBeNull();
  });

  it('取出盘片后消失', async () => {
    const { fake } = await setup([floppyStatus(0, 'FLPPY1.IMG')]);
    fake.floppies = [floppyStatus(0)];
    fake.emit('media.changed', { device: 'floppy', drive: 0 });
    await act(async () => settle());
    expect(screen.queryByText('FLPPY1.IMG')).toBeNull();
  });
});
