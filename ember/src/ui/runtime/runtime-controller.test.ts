/*
 * ------------------------------------------------------------------------
 * 名称：runtime-controller.test.ts
 * 说明：运行页控制器的测试：初始读取、上电与断电、暂停与继续、复位、关闭卡带、打开卡带与最近打开、音量与静音（静音记住原音量）、核心事件触发重读、
 *       消息接入（装入成功、上电、断电、暂停、继续、软硬复位、卡带弹出、核心重启）、失败时通知加消息、核心重启后状态刷新、断开后不再处理。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用假的核心接口（runtime-fakes.ts）与假的外壳能力，记录消息、通知与命令刷新次数。
 * ------------------------------------------------------------------------
 */

import { beforeEach, describe, expect, it } from 'vitest';
import type { ShellMessageInput } from '../shell/shell-registry';
import { FakeRuntimeBackend, machineStatus, settle } from './runtime-fakes';
import { RuntimeController } from './runtime-controller';

/** 假的外壳能力：记录消息、通知与命令刷新。 */
function createHost() {
  const messages: ShellMessageInput[] = [];
  const notices: string[] = [];
  const refreshes = { count: 0 };
  return {
    messages,
    notices,
    refreshes,
    host: {
      addMessage: (message: ShellMessageInput) => messages.push(message),
      notify: (text: string) => notices.push(text),
      refreshCommands: () => {
        refreshes.count += 1;
      }
    }
  };
}

let fake: FakeRuntimeBackend;
let controller: RuntimeController;
let context: ReturnType<typeof createHost>;

/** 接上假核心并等初始读取完成。 */
async function attach(): Promise<() => void> {
  const detach = controller.attach(fake.backend, context.host);
  await settle();
  return detach;
}

/** 消息的文字。 */
function texts(): string[] {
  return context.messages.map((message) => message.text);
}

beforeEach(() => {
  fake = new FakeRuntimeBackend();
  controller = new RuntimeController();
  context = createHost();
});

describe('初始读取', () => {
  it('接上后读出机器状态、最近打开的清单与音量', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\nestest.nes');
    fake.recentPaths = ['D:\\roms\\nestest.nes', 'D:\\roms\\mario.nes'];
    fake.volume = 40;
    fake.muted = true;
    await attach();
    const state = controller.snapshot();
    expect(state.status?.cartridge?.path).toBe('D:\\roms\\nestest.nes');
    expect(state.recent).toEqual([
      { id: 1, name: 'nestest.nes' },
      { id: 2, name: 'mario.nes' }
    ]);
    expect(state.volume).toBe(40);
    expect(state.muted).toBe(true);
  });

  it('没有接上时状态是“还没读到”，音量为默认值', () => {
    expect(controller.snapshot()).toEqual({ status: undefined, recent: [], volume: 100, muted: false });
    expect(controller.machineState()).toBe('unknown');
  });

  it('核心还读不到状态时不报错，状态保持“还没读到”', async () => {
    fake.status = undefined;
    await attach();
    expect(controller.snapshot().status).toBeUndefined();
    expect(context.notices).toEqual([]);
    expect(context.messages).toEqual([]);
  });

  it('状态变化时通知订阅者与外壳刷新命令', async () => {
    let changes = 0;
    controller.subscribe(() => {
      changes += 1;
    });
    await attach();
    expect(changes).toBeGreaterThan(0);
    expect(context.refreshes.count).toBeGreaterThan(0);
  });
});

describe('控制与消息', () => {
  it('上电：没有开机时不发请求；断电的机器上电后直接运行并写“上电”', async () => {
    await attach();
    fake.status = machineStatus('off', 'paused');
    fake.calls.length = 0;
    controller.togglePower();
    await settle();
    expect(fake.calls).toEqual([]);

    fake.status = machineStatus('off', 'paused', 'D:\\roms\\a.nes');
    controller.attach(fake.backend, context.host);
    await settle();
    fake.calls.length = 0;
    controller.togglePower();
    await settle();
    expect(fake.calls[0]).toBe('runtime.powerOn {"resume":true}');
    expect(controller.snapshot().status).toMatchObject({ power: 'on', mode: 'running' });
    expect(texts()).toEqual(['上电']);
  });

  it('断电：通电时断电，写“断电”', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    await attach();
    controller.togglePower();
    await settle();
    expect(fake.calls).toContain('runtime.powerOff {}');
    expect(controller.snapshot().status).toMatchObject({ power: 'off' });
    expect(texts()).toEqual(['断电']);
  });

  it('暂停与继续：连续运行时暂停，暂停时继续；断电时不发请求', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    await attach();
    controller.togglePause();
    await settle();
    expect(controller.snapshot().status?.mode).toBe('paused');
    controller.togglePause();
    await settle();
    expect(controller.snapshot().status?.mode).toBe('running');
    expect(texts()).toEqual(['已暂停', '继续运行']);

    controller.togglePower();
    await settle();
    fake.calls.length = 0;
    controller.togglePause();
    await settle();
    expect(fake.calls).toEqual([]);
  });

  it('软复位与硬复位：软复位只在通电时可用，硬复位已开机就可用，各写一条消息', async () => {
    fake.status = machineStatus('off', 'paused', 'D:\\roms\\a.nes');
    await attach();
    controller.softReset();
    await settle();
    expect(fake.calls.filter((call) => call.startsWith('runtime.reset'))).toEqual([]);
    controller.hardReset();
    await settle();
    expect(fake.calls).toContain('runtime.reset {"kind":"hard"}');

    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    controller.attach(fake.backend, context.host);
    await settle();
    controller.softReset();
    await settle();
    expect(fake.calls).toContain('runtime.reset {"kind":"soft"}');
    expect(texts()).toEqual(['硬复位', '软复位']);
  });

  it('没有开机时控制全部不可用，也不发请求', async () => {
    await attach();
    expect(controller.availability()).toEqual({ powerToggle: false, pauseToggle: false, softReset: false, hardReset: false, eject: false });
    fake.status = machineStatus('off', 'paused');
    fake.calls.length = 0;
    controller.togglePower();
    controller.togglePause();
    controller.softReset();
    controller.hardReset();
    controller.ejectCartridge();
    await settle();
    expect(fake.calls).toEqual([]);
  });

  it('关闭卡带：请求核心取出，写带卡带名的消息；没有卡带时不发请求', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\nestest.nes');
    await attach();
    controller.ejectCartridge();
    await settle();
    expect(fake.calls).toContain('cartridges.eject {}');
    expect(texts()).toEqual(['已关闭卡带 nestest.nes']);
    expect(controller.machineState()).toBe('none');
    fake.calls.length = 0;
    controller.ejectCartridge();
    await settle();
    expect(fake.calls).toEqual([]);
  });

  it('操作失败时用通知提示并写一条错误消息，不写成功消息', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    await attach();
    fake.failures.set('runtime.pause', { code: 'runtime.notPowered', message: '机器没有通电' });
    controller.togglePause();
    await settle();
    expect(context.notices).toEqual(['暂停失败：机器没有通电']);
    expect(context.messages).toEqual([{ level: 'error', text: '暂停失败：机器没有通电' }]);
  });

  it('操作结束后把焦点还给画面', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    await attach();
    let focused = 0;
    controller.setScreenFocus(() => {
      focused += 1;
    });
    controller.togglePause();
    await settle();
    expect(focused).toBe(1);
    controller.setScreenFocus(undefined);
    controller.togglePause();
    await settle();
    expect(focused).toBe(1);
  });
});

describe('打开卡带与最近打开', () => {
  it('打开卡带成功：写一条装入消息（卡带名、mapper 号与匹配的插件）并刷新最近打开，不重复写', async () => {
    await attach();
    fake.pickedPath = 'D:\\roms\\nestest.nes';
    controller.openCartridge();
    await settle();
    expect(texts()).toEqual(['已装入卡带 nestest.nes：mapper 0，匹配插件 NROM 1.0.0']);
    expect(controller.snapshot().recent).toEqual([{ id: 1, name: 'nestest.nes' }]);
    expect(controller.machineState()).toBe('running');
  });

  it('用户取消选择：没有消息也没有通知', async () => {
    await attach();
    fake.pickedPath = undefined;
    controller.openCartridge();
    await settle();
    expect(context.messages).toEqual([]);
    expect(context.notices).toEqual([]);
  });

  it('加载失败：通知提示、写错误消息，并重新读取最近打开的清单（主进程已把它移出）', async () => {
    fake.recentPaths = ['D:\\roms\\bad.nes'];
    await attach();
    expect(controller.snapshot().recent).toHaveLength(1);
    fake.recentPaths = [];
    fake.failures.set('cartridges.open', { code: 'cartridges.notInes', message: '文件不是 NES 卡带' });
    controller.openCartridge();
    await settle();
    expect(context.notices).toEqual(['打开卡带失败：文件不是 NES 卡带']);
    expect(context.messages).toEqual([{ level: 'error', text: '打开卡带失败：文件不是 NES 卡带' }]);
    expect(controller.snapshot().recent).toEqual([]);
  });

  it('重新打开最近的卡带：按编号请求，装入成功写消息', async () => {
    fake.recentPaths = ['D:\\roms\\a.nes', 'D:\\roms\\b.nes'];
    await attach();
    controller.openRecent(2);
    await settle();
    expect(fake.calls).toContain('cartridges.openRecent {"id":2}');
    expect(texts()).toEqual(['已装入卡带 b.nes：mapper 0，匹配插件 NROM 1.0.0']);
    expect(controller.snapshot().recent.map((item) => item.name)).toEqual(['b.nes', 'a.nes']);
  });

  it('最近打开的卡带加载失败时通知并写错误消息', async () => {
    fake.recentPaths = ['D:\\roms\\a.nes'];
    await attach();
    fake.failures.set('cartridges.openRecent', { code: 'cartridges.fileNotFound', message: '找不到文件' });
    controller.openRecent(1);
    await settle();
    expect(context.notices).toEqual(['打开卡带失败：找不到文件']);
  });

  it('其他来源装入的卡带（如启动参数）同样写消息并刷新状态与清单', async () => {
    await attach();
    fake.recentPaths = ['D:\\roms\\boot.nes'];
    fake.status = machineStatus('on', 'running', 'D:\\roms\\boot.nes');
    fake.emit('cartridges.loaded', { cartridge: fake.status.cartridge });
    await settle();
    expect(texts()).toEqual(['已装入卡带 boot.nes：mapper 0，匹配插件 NROM 1.0.0']);
    expect(controller.machineState()).toBe('running');
    expect(controller.snapshot().recent).toHaveLength(1);
  });

  it('装入事件的数据格式不对时只写“已装入卡带”', async () => {
    await attach();
    fake.emit('cartridges.loaded', { cartridge: 5 });
    await settle();
    expect(texts()).toEqual(['已装入卡带']);
  });

  it('runtime.stateChanged 事件触发重新读取状态', async () => {
    await attach();
    fake.status = machineStatus('on', 'paused', 'D:\\roms\\a.nes');
    fake.emit('runtime.stateChanged', { power: 'on', mode: 'paused', reason: 'pause' });
    await settle();
    expect(controller.machineState()).toBe('paused');
  });
});

describe('音量与静音', () => {
  it('调整音量：界面立即生效，核心返回的值为准，范围限制在 0 到 100', async () => {
    await attach();
    controller.setVolume(40.4);
    expect(controller.snapshot().volume).toBe(40);
    await settle();
    expect(fake.calls).toContain('output.setVolume {"volume":40}');
    controller.setVolume(250);
    await settle();
    expect(controller.snapshot().volume).toBe(100);
    controller.setVolume(-5);
    await settle();
    expect(controller.snapshot().volume).toBe(0);
  });

  it('静音记住原音量：静音时音量不变，取消静音回到原音量', async () => {
    await attach();
    controller.setVolume(60);
    await settle();
    controller.toggleMute();
    await settle();
    expect(controller.snapshot()).toMatchObject({ volume: 60, muted: true });
    expect(fake.calls).toContain('output.setVolume {"muted":true}');
    controller.toggleMute();
    await settle();
    expect(controller.snapshot()).toMatchObject({ volume: 60, muted: false });
  });

  it('调整音量失败时通知并写错误消息', async () => {
    await attach();
    fake.failures.set('output.setVolume', { code: 'core.internal', message: '声音设备不可用' });
    controller.toggleMute();
    await settle();
    expect(context.notices).toEqual(['调整音量失败：声音设备不可用']);
  });
});

describe('核心重启', () => {
  it('核心进入启动中时清掉旧状态，就绪后重新读取状态、清单与音量，并写“核心已重启”', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    fake.recentPaths = ['D:\\roms\\a.nes'];
    fake.volume = 30;
    await attach();
    expect(controller.machineState()).toBe('running');

    fake.emitCoreStatus({ state: 'starting' });
    expect(controller.snapshot().status).toBeUndefined();
    expect(controller.machineState()).toBe('unknown');

    // 新核心里没有卡带，音量回到默认值。
    fake.status = machineStatus('off', 'paused');
    fake.volume = 100;
    fake.emitCoreStatus({ state: 'ready', coreVersion: '0.1.0' });
    await settle();
    expect(controller.machineState()).toBe('none');
    expect(controller.snapshot().volume).toBe(100);
    expect(texts()).toEqual(['核心进程已重启，机器状态已重新读取']);
  });

  it('第一次启动就绪不写“核心已重启”，只读取状态', async () => {
    fake.coreStatus = { state: 'starting' };
    fake.status = undefined;
    await attach();
    fake.status = machineStatus('off', 'paused');
    fake.emitCoreStatus({ state: 'ready', coreVersion: '0.1.0' });
    await settle();
    expect(controller.machineState()).toBe('none');
    expect(context.messages).toEqual([]);
  });

  it('核心不可用时写错误消息，状态保持“还没读到”', async () => {
    await attach();
    fake.emitCoreStatus({ state: 'unavailable', error: { code: 'core.unavailable', message: '核心反复崩溃，已停止重启' } });
    expect(context.messages).toEqual([{ level: 'error', text: '核心不可用：核心反复崩溃，已停止重启' }]);
    expect(controller.snapshot().status).toBeUndefined();
  });

  it('核心连续重启两次，每次就绪都写一条消息', async () => {
    await attach();
    for (let round = 0; round < 2; round += 1) {
      fake.emitCoreStatus({ state: 'starting' });
      fake.emitCoreStatus({ state: 'ready', coreVersion: '0.1.0' });
      await settle();
    }
    expect(texts()).toEqual(['核心进程已重启，机器状态已重新读取', '核心进程已重启，机器状态已重新读取']);
  });
});

describe('断开', () => {
  it('断开后取消订阅，不再处理事件与请求结果', async () => {
    const detach = await attach();
    expect(fake.eventListenerCount).toBe(1);
    detach();
    expect(fake.eventListenerCount).toBe(0);
    fake.emit('cartridges.loaded', { cartridge: machineStatus('on', 'running', 'D:\\roms\\a.nes').cartridge });
    await settle();
    expect(context.messages).toEqual([]);
    fake.calls.length = 0;
    controller.togglePower();
    controller.openCartridge();
    await settle();
    expect(fake.calls).toEqual([]);
  });

  it('重新接上时先断开旧的订阅', async () => {
    await attach();
    await attach();
    expect(fake.eventListenerCount).toBe(1);
  });
});
