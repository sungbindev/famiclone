/*
 * ------------------------------------------------------------------------
 * 名称：media-module.test.tsx
 * 说明：介质模块登记项的测试：活动栏有只有边栏的“设备”按钮；软驱读写灯没有软驱时不显示，有软驱时马达转动就亮、停了就灭。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的核心接口与真实的控制器；灯的状态取自控制器读到的软驱状态（核心经 media.changed 通知马达变化）。
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import { FakeMediaBackend, floppyStatus } from './media-fakes';
import { MediaController } from './media-controller';
import { createMediaModule } from './media-module';

/** 等已发出的请求与它们的后续处理完成（微任务队列清空）。 */
async function settle(): Promise<void> {
  for (let index = 0; index < 8; index += 1) await Promise.resolve();
}

describe('介质模块的登记项', () => {
  it('活动栏只有一个“设备”按钮，只有边栏、没有页面', () => {
    const item = createMediaModule(new MediaController()).activityItems?.[0];
    expect(item?.label).toBe('设备');
    expect(item?.sideBar).toBeDefined();
    expect(item?.page).toBeUndefined();
  });

  it('软驱读写灯：没有软驱时不显示；马达转动时亮，停了就灭', async () => {
    const fake = new FakeMediaBackend();
    fake.floppies = [];
    const controller = new MediaController();
    const detach = controller.attach(fake.backend, { addMessage: () => undefined, notify: () => undefined, refresh: () => undefined });
    await settle();
    const lamp = createMediaModule(controller).activityItems?.[0]?.lamp;
    expect(lamp?.()).toBeUndefined();

    fake.floppies = [floppyStatus(0, 'sys.img')];
    fake.emit('media.changed', { device: 'floppy', drive: 0 });
    await settle();
    expect(lamp?.()).toEqual({ lit: false, label: '软驱读写灯' });

    fake.floppies = [floppyStatus(0, 'sys.img', { motor: true })];
    fake.emit('media.changed', { device: 'floppy', drive: 0 });
    await settle();
    expect(lamp?.()).toEqual({ lit: true, label: '软驱读写灯' });
    detach();
  });
});
