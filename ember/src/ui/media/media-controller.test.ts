/*
 * ------------------------------------------------------------------------
 * 名称：media-controller.test.ts
 * 说明：设备边栏控制器的测试：初始状态读取、载入镜像（选定、取消、失败）、取出、保存、写保护、事件刷新、打印机输出一页的通知与消息、打印失败、定时读取只在边栏显示期间运行、核心重启后清空并重新读取、断开后不再响应。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的核心接口与假的计时器；每个用例结束前断开控制器。
 * ------------------------------------------------------------------------
 */

import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { FloppyStatus } from '../../contracts/media/generated/media';
import { FakeMediaBackend, floppyStatus } from './media-fakes';
import { MediaController, type MediaHostActions } from './media-controller';

/** 等已发出的请求与它们的后续处理完成（微任务队列清空）。 */
async function settle(): Promise<void> {
  for (let index = 0; index < 8; index += 1) await Promise.resolve();
}

/** 接上假核心；floppies 是软驱的初始状态。 */
async function setup(floppies?: FloppyStatus[]) {
  const fake = new FakeMediaBackend();
  if (floppies !== undefined) fake.floppies = floppies;
  const messages: Array<{ level: string; text: string }> = [];
  const notices: Array<{ text: string; kind: string }> = [];
  const refreshes = { count: 0 };
  const host: MediaHostActions = {
    addMessage: (message) => messages.push({ level: message.level, text: message.text }),
    notify: (text, kind) => notices.push({ text, kind }),
    refresh: () => {
      refreshes.count += 1;
    }
  };
  const controller = new MediaController();
  const detach = controller.attach(fake.backend, host);
  await settle();
  return { fake, controller, detach, messages, notices, refreshes };
}

describe('MediaController', () => {
  beforeEach(() => {
    vi.useFakeTimers();
  });
  afterEach(() => {
    vi.useRealTimers();
  });

  it('接上后读取各软驱与打印机的状态；没有开机时软驱为空', async () => {
    const { controller, detach } = await setup();
    expect(controller.snapshot().floppies.map((item) => item.drive)).toEqual([0, 1]);
    expect(controller.snapshot().printer?.state).toBe('idle');
    detach();
  });

  it('载入镜像：选定后写一条消息并刷新状态；用户取消时什么都不发生', async () => {
    const { fake, controller, detach, messages } = await setup();
    fake.pickedFile = 'sys.img';
    controller.insertFloppy(0);
    await settle();
    expect(fake.calls).toContain('media.openFloppy {"drive":0}');
    expect(messages).toEqual([{ level: 'info', text: '已把 sys.img 载入软驱 A' }]);
    expect(controller.snapshot().floppies[0]?.fileName).toBe('sys.img');

    fake.pickedFile = undefined;
    controller.insertFloppy(1);
    await settle();
    expect(messages).toHaveLength(1);
    detach();
  });

  it('载入镜像失败：用通知提示核心的说明，并写一条错误消息', async () => {
    const { fake, controller, detach, messages, notices } = await setup();
    fake.failures.set('media.openFloppy', { code: 'media.imageInvalid', message: '镜像文件是空的' });
    controller.insertFloppy(0);
    await settle();
    expect(notices).toEqual([{ text: '载入软盘镜像失败：镜像文件是空的', kind: 'error' }]);
    expect(messages).toEqual([{ level: 'error', text: '载入软盘镜像失败：镜像文件是空的' }]);
    detach();
  });

  it('取出、保存镜像、写保护：各发出对应的请求，写消息并刷新状态', async () => {
    const { fake, controller, detach, messages } = await setup([floppyStatus(0, 'sys.img', { modified: true }), floppyStatus(1)]);

    controller.setWriteProtected(0, true);
    await settle();
    controller.saveFloppy(0);
    await settle();
    controller.ejectFloppy(0);
    await settle();
    expect(fake.calls).toContain('media.floppyWriteProtect {"drive":0,"writeProtected":true}');
    expect(fake.calls).toContain('media.floppySave {"drive":0}');
    expect(fake.calls).toContain('media.floppyEject {"drive":0}');
    expect(messages.map((message) => message.text)).toEqual(['软驱 A 的盘片已设为写保护', '已保存软驱 A 的镜像 sys.img', '已取出软驱 A 的盘片']);
    expect(controller.snapshot().floppies[0]?.inserted).toBe(false);
    detach();
  });

  it('核心发 media.changed 后重新读取软驱状态', async () => {
    const { fake, controller, detach } = await setup();
    fake.floppies = [floppyStatus(0, 'a.img'), floppyStatus(1)];
    fake.emit('media.changed', { device: 'floppy', drive: 0 });
    await settle();
    expect(controller.snapshot().floppies[0]?.fileName).toBe('a.img');
    detach();
  });

  it('打印机输出一页：用通知告知文件名，写一条消息，并刷新状态', async () => {
    const { fake, controller, detach, messages, notices } = await setup();
    fake.printer = { state: 'done', hasContent: false, pages: 1, lastFile: 'page-1.png' };
    fake.emit('media.printed', { fileName: 'page-1.png', page: 1 });
    await settle();
    expect(notices).toEqual([{ text: '打印机输出了第 1 页：page-1.png', kind: 'info' }]);
    expect(messages).toEqual([{ level: 'info', text: '打印机输出了第 1 页：page-1.png' }]);
    expect(controller.snapshot().printer?.state).toBe('done');
    detach();
  });

  it('打印机输出的一页保存失败：用通知提示原因并写一条错误消息', async () => {
    const { fake, detach, messages, notices } = await setup();
    fake.emit('media.printFailed', { message: '目录无法写入' });
    expect(notices).toEqual([{ text: '打印机输出失败：目录无法写入', kind: 'error' }]);
    expect(messages).toEqual([{ level: 'error', text: '打印机输出失败：目录无法写入' }]);
    detach();
  });

  it('定时读取只在边栏显示期间运行，隐藏后停止', async () => {
    const { fake, controller, detach } = await setup();
    const before = fake.count('media.floppyStatus');
    await vi.advanceTimersByTimeAsync(2000);
    expect(fake.count('media.floppyStatus')).toBe(before);

    const stop = controller.startPolling();
    await settle();
    const started = fake.count('media.floppyStatus');
    await vi.advanceTimersByTimeAsync(1000);
    expect(fake.count('media.floppyStatus')).toBeGreaterThan(started);

    stop();
    const stopped = fake.count('media.floppyStatus');
    await vi.advanceTimersByTimeAsync(2000);
    expect(fake.count('media.floppyStatus')).toBe(stopped);
    detach();
  });

  it('读取到的状态没有变化时不通知订阅者', async () => {
    const { fake, controller, detach } = await setup();
    let changes = 0;
    controller.subscribe(() => {
      changes += 1;
    });
    fake.emit('media.changed', { device: 'floppy', drive: 0 });
    await settle();
    expect(changes).toBe(0);
    detach();
  });

  it('读取到的状态有变化时让外壳重新渲染（活动栏的读写灯），没有变化时不刷新', async () => {
    const { fake, refreshes, detach } = await setup();
    const before = refreshes.count;
    fake.emit('media.changed', { device: 'floppy', drive: 0 });
    await settle();
    expect(refreshes.count).toBe(before);
    fake.floppies = [floppyStatus(0, 'a.img', { motor: true }), floppyStatus(1)];
    fake.emit('media.changed', { device: 'floppy', drive: 0 });
    await settle();
    expect(refreshes.count).toBe(before + 1);
    detach();
  });

  it('核心重启：先清空旧状态，就绪后重新读取', async () => {
    const { fake, controller, detach } = await setup();
    fake.emitCoreStatus({ state: 'starting' });
    expect(controller.snapshot().floppies).toEqual([]);
    expect(controller.snapshot().printer).toBeUndefined();
    fake.emitCoreStatus({ state: 'ready', coreVersion: '0.1.0' });
    await settle();
    expect(controller.snapshot().floppies).toHaveLength(2);
    detach();
  });

  it('断开后取消订阅，之后的操作不再发请求', async () => {
    const { fake, controller, detach } = await setup();
    expect(fake.eventListenerCount).toBe(1);
    detach();
    expect(fake.eventListenerCount).toBe(0);
    const before = fake.calls.length;
    controller.ejectFloppy(0);
    await settle();
    expect(fake.calls).toHaveLength(before);
  });
});
