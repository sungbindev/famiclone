/*
 * ------------------------------------------------------------------------
 * 名称：media-module.test.tsx
 * 说明：介质模块登记项的测试：底部面板有“设备”页签（状态栏有对应的按钮）；软驱读写灯没有软驱时不显示，有软驱时马达转动就亮、停了就灭。
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
  it('底部面板只有一个“设备”面板', () => {
    const panels = createMediaModule(new MediaController()).bottomPanels;
    expect(panels).toHaveLength(1);
    expect(panels?.[0]?.title).toBe('设备');
    expect(panels?.[0]?.id).toBe('media.devices');
  });

  it('软驱读写灯：没有软驱时不显示；马达转动时亮，停了就灭', async () => {
    const fake = new FakeMediaBackend();
    fake.floppies = [];
    const controller = new MediaController();
    const detach = controller.attach(fake.backend, { notify: () => undefined, refresh: () => undefined });
    await settle();
    const lamp = createMediaModule(controller).bottomPanels?.[0]?.lamp;
    expect(lamp?.()).toBeUndefined();

    fake.floppies = [floppyStatus(0, 'sys.img')];
    fake.emit('media.changed', { device: 'floppy', drive: 0 });
    await settle();
    expect(lamp?.()).toEqual({ lit: false, label: '软驱读写灯', inIcon: { x: 50, y: 60 } });

    fake.floppies = [floppyStatus(0, 'sys.img', { motor: true })];
    fake.emit('media.changed', { device: 'floppy', drive: 0 });
    await settle();
    expect(lamp?.()).toEqual({ lit: true, label: '软驱读写灯', inIcon: { x: 50, y: 60 } });
    detach();
  });

  it('文件处理项：软盘镜像载入软驱 A，声音文件载入磁带机，扩展名不重叠', async () => {
    const fake = new FakeMediaBackend();
    fake.droppedFiles.set(5, 'sys.img');
    fake.droppedFiles.set(6, 'lesson.m4a');
    const controller = new MediaController();
    const detach = controller.attach(fake.backend, { notify: () => undefined, refresh: () => undefined });
    await settle();
    const handlers = createMediaModule(controller).fileHandlers ?? [];
    expect(handlers.map((item) => item.id)).toEqual(['media.floppyImage', 'media.tapeSound']);
    expect(handlers[0]?.extensions).toEqual(['img', 'ima', 'vfd', 'dsk', 'flp']);
    expect(handlers[1]?.extensions).toContain('m4a');
    handlers[0]?.open({ id: 5, name: 'sys.img' });
    handlers[1]?.open({ id: 6, name: 'lesson.m4a' });
    await settle();
    expect(fake.calls).toContain('media.openFloppy {"drive":0,"droppedId":5}');
    expect(fake.calls).toContain('media.openTape {"droppedId":6}');
    detach();
  });
});
