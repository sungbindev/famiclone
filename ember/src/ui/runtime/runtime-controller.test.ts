/*
 * ------------------------------------------------------------------------
 * 名称：runtime-controller.test.ts
 * 说明：运行模块控制器的测试：初始读取、选择机型（只改已选机型并保存、启动时沿用保存的）、开机与断电（按已选机型：学习机直接用 BIOS 开机，经典 FC 要先装入卡带）、暂停与继续、复位、关闭卡带、
 *       打开卡带与最近打开（成功后已选机型变成经典 FC）、音量与静音（静音记住原音量）、核心事件触发重读、失败时通知、核心重启后状态刷新、断开后不再处理。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用假的核心接口（runtime-fakes.ts）与假的外壳能力，记录通知与命令刷新次数。
 * ------------------------------------------------------------------------
 */

import { beforeEach, describe, expect, it } from 'vitest';
import type { RuntimeStatusResult } from '../../contracts/runtime/generated/runtime';
import { FakeRuntimeBackend, machineStatus, settle } from './runtime-fakes';
import { RuntimeController } from './runtime-controller';

/** 假的外壳能力：记录通知与命令刷新。 */
function createHost() {
  const notices: string[] = [];
  const refreshes = { count: 0 };
  return {
    notices,
    refreshes,
    host: {
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

/** 给小霸王 2000 加上 BIOS。 */
function addBios(): void {
  fake.machines = fake.machines.map((machine) => (machine.id === 'sb2000' ? { ...machine, bios: { fileName: 'SB2K_BIOS.NES', size: 524304 } } : machine));
}

/** 已开机的整机机型的状态。 */
function wholeMachine(power: 'off' | 'on', mode: 'paused' | 'running'): RuntimeStatusResult {
  return { ...machineStatus(power, mode), machine: 'sb2000' };
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

  it('没有接上时状态是“还没读到”，已选机型是经典 FC，音量为默认值', () => {
    expect(controller.snapshot()).toEqual({ status: undefined, recent: [], machines: [], selectedMachine: 'fc', volume: 100, muted: false, familyKeyboard: false });
    expect(controller.machineState()).toBe('unknown');
  });

  it('接上后读出机型清单；机型的 BIOS 变化（machines.changed）后重新读取', async () => {
    await attach();
    expect(controller.snapshot().machines.map((machine) => machine.id)).toEqual(['fc', 'sb97', 'sb2000', 'bbk10', 'bbk20']);
    expect(controller.snapshot().machines.find((machine) => machine.id === 'sb2000')?.bios).toBeUndefined();
    addBios();
    fake.emit('machines.changed', { machine: 'sb2000' });
    await settle();
    expect(controller.snapshot().machines.find((machine) => machine.id === 'sb2000')?.bios?.fileName).toBe('SB2K_BIOS.NES');
  });

  it('核心还读不到状态时不报错，状态保持“还没读到”', async () => {
    fake.status = undefined;
    await attach();
    expect(controller.snapshot().status).toBeUndefined();
    expect(context.notices).toEqual([]);
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

describe('选择机型', () => {
  it('只改已选机型并保存，不开机、不装入、不影响正在运行的机器', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    await attach();
    fake.calls.length = 0;
    controller.selectMachine('sb2000');
    await settle();
    expect(controller.snapshot().selectedMachine).toBe('sb2000');
    expect(fake.layoutPatches).toEqual([{ selectedMachine: 'sb2000' }]);
    expect(fake.calls).toEqual([]);
    expect(controller.machineState()).toBe('running');
  });

  it('选已经选中的机型什么都不做', async () => {
    await attach();
    controller.selectMachine('fc');
    expect(fake.layoutPatches).toEqual([]);
  });

  it('选中并开机：记住机型并用它的 BIOS 开机；已经是这个机型也重新开机，机型不重复保存', async () => {
    addBios();
    await attach();
    controller.selectAndBootMachine('sb2000');
    await settle();
    expect(controller.snapshot().selectedMachine).toBe('sb2000');
    expect(fake.layoutPatches).toEqual([{ selectedMachine: 'sb2000' }]);
    expect(fake.calls.filter((call) => call.startsWith('machines.boot'))).toEqual(['machines.boot {"machine":"sb2000"}']);
    expect(controller.snapshot().status?.machine).toBe('sb2000');
    controller.selectAndBootMachine('sb2000');
    await settle();
    expect(fake.layoutPatches).toEqual([{ selectedMachine: 'sb2000' }]);
    expect(fake.calls.filter((call) => call.startsWith('machines.boot'))).toHaveLength(2);
  });

  it('选中并开机失败时用通知提示，已选机型仍记住', async () => {
    addBios();
    await attach();
    fake.failures.set('machines.boot', { code: 'machines.biosInvalid', message: 'BIOS 文件无效' });
    controller.selectAndBootMachine('sb2000');
    await settle();
    expect(context.notices).toEqual(['开机失败：BIOS 文件无效']);
    expect(controller.snapshot().selectedMachine).toBe('sb2000');
  });

  it('启动时沿用上次保存的已选机型；没保存过是经典 FC', async () => {
    fake.savedMachine = 'sb2000';
    await attach();
    expect(controller.snapshot().selectedMachine).toBe('sb2000');
    const other = new RuntimeController();
    const fresh = new FakeRuntimeBackend();
    other.attach(fresh.backend, createHost().host);
    await settle();
    expect(other.snapshot().selectedMachine).toBe('fc');
  });
});

describe('开机与断电', () => {
  it('已选经典 FC、没有装入卡带：开机不可用，也不发请求', async () => {
    await attach();
    expect(controller.availability().powerToggle).toBe(false);
    fake.calls.length = 0;
    controller.togglePower();
    await settle();
    expect(fake.calls).toEqual([]);
  });

  it('已选经典 FC、卡带已装入且断电：开机直接上电运行（不重新选卡带）', async () => {
    fake.status = machineStatus('off', 'paused', 'D:\\roms\\a.nes');
    await attach();
    expect(controller.availability().powerToggle).toBe(true);
    fake.calls.length = 0;
    controller.togglePower();
    await settle();
    expect(fake.calls[0]).toBe('runtime.powerOn {"resume":true}');
    expect(controller.snapshot().status).toMatchObject({ power: 'on', mode: 'running' });
  });

  it('通电时是断电', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    await attach();
    controller.togglePower();
    await settle();
    expect(fake.calls).toContain('runtime.powerOff {}');
    expect(controller.snapshot().status).toMatchObject({ power: 'off' });
  });

  it('已选学习机、已添加 BIOS：不需要卡带，开机用 BIOS 装配主机并运行', async () => {
    addBios();
    await attach();
    controller.selectMachine('sb2000');
    expect(controller.availability().powerToggle).toBe(true);
    controller.togglePower();
    await settle();
    expect(fake.calls).toContain('machines.boot {"machine":"sb2000"}');
    expect(controller.machineState()).toBe('running');
    expect(controller.snapshot().status?.machine).toBe('sb2000');
    expect(controller.snapshot().status?.cartridge).toBeUndefined();
  });

  it('已选学习机、没有 BIOS：开机不可用，也不发请求', async () => {
    await attach();
    controller.selectMachine('sb2000');
    expect(controller.availability().powerToggle).toBe(false);
    fake.calls.length = 0;
    controller.togglePower();
    await settle();
    expect(fake.calls).toEqual([]);
  });

  it('已选的机型与装入的主机不一致：未通电时开机用已选机型重新开机', async () => {
    addBios();
    fake.status = machineStatus('off', 'paused', 'D:\\roms\\a.nes');
    await attach();
    controller.selectMachine('sb2000');
    controller.togglePower();
    await settle();
    expect(fake.calls).toContain('machines.boot {"machine":"sb2000"}');
    expect(controller.snapshot().status?.machine).toBe('sb2000');
  });

  it('已选经典 FC、装入的是学习机：开机不可用，要先打开卡带', async () => {
    fake.status = wholeMachine('off', 'paused');
    await attach();
    expect(controller.availability().powerToggle).toBe(false);
  });

  it('开机失败时用通知提示，状态不变', async () => {
    addBios();
    await attach();
    controller.selectMachine('sb2000');
    fake.failures.set('machines.boot', { code: 'machines.biosInvalid', message: 'BIOS 文件无效' });
    controller.togglePower();
    await settle();
    expect(context.notices).toEqual(['开机失败：BIOS 文件无效']);
    expect(controller.machineState()).toBe('none');
  });
});

describe('控制', () => {
  it('暂停与继续：连续运行时暂停，暂停时继续；断电时不发请求', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    await attach();
    controller.togglePause();
    await settle();
    expect(controller.snapshot().status?.mode).toBe('paused');
    controller.togglePause();
    await settle();
    expect(controller.snapshot().status?.mode).toBe('running');

    controller.togglePower();
    await settle();
    fake.calls.length = 0;
    controller.togglePause();
    await settle();
    expect(fake.calls).toEqual([]);
  });

  it('软复位与硬复位：都只在通电时可用，断电时点了不发请求', async () => {
    fake.status = machineStatus('off', 'paused', 'D:\\roms\\a.nes');
    await attach();
    controller.softReset();
    controller.hardReset();
    await settle();
    expect(fake.calls.filter((call) => call.startsWith('runtime.reset'))).toEqual([]);

    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    controller.attach(fake.backend, context.host);
    await settle();
    controller.softReset();
    await settle();
    expect(fake.calls).toContain('runtime.reset {"kind":"soft"}');
    controller.hardReset();
    await settle();
    expect(fake.calls).toContain('runtime.reset {"kind":"hard"}');
  });

  it('没有开机时暂停、复位、关闭卡带全部不可用，也不发请求', async () => {
    await attach();
    expect(controller.availability()).toEqual({ powerToggle: false, pauseToggle: false, softReset: false, hardReset: false, eject: false });
    fake.calls.length = 0;
    controller.togglePower();
    controller.togglePause();
    controller.softReset();
    controller.hardReset();
    controller.ejectCartridge();
    await settle();
    expect(fake.calls).toEqual([]);
  });

  it('关闭卡带：请求核心取出；没有卡带时不发请求', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\nestest.nes');
    await attach();
    controller.ejectCartridge();
    await settle();
    expect(fake.calls).toContain('cartridges.eject {}');
    expect(controller.machineState()).toBe('none');
    fake.calls.length = 0;
    controller.ejectCartridge();
    await settle();
    expect(fake.calls).toEqual([]);
  });

  it('操作失败时用通知提示', async () => {
    fake.status = machineStatus('on', 'running', 'D:\\roms\\a.nes');
    await attach();
    fake.failures.set('runtime.pause', { code: 'runtime.notPowered', message: '机器没有通电' });
    controller.togglePause();
    await settle();
    expect(context.notices).toEqual(['暂停失败：机器没有通电']);
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
  it('打开卡带成功：直接运行，刷新最近打开，已选机型变成经典 FC 并保存', async () => {
    addBios();
    await attach();
    controller.selectMachine('sb2000');
    fake.layoutPatches.length = 0;
    fake.pickedPath = 'D:\\roms\\nestest.nes';
    controller.openCartridge();
    await settle();
    expect(controller.snapshot().recent).toEqual([{ id: 1, name: 'nestest.nes' }]);
    expect(controller.machineState()).toBe('running');
    expect(controller.snapshot().selectedMachine).toBe('fc');
    expect(fake.layoutPatches).toEqual([{ selectedMachine: 'fc' }]);
  });

  it('用户取消选择：没有通知，已选机型不变', async () => {
    await attach();
    controller.selectMachine('sb2000');
    fake.pickedPath = undefined;
    controller.openCartridge();
    await settle();
    expect(context.notices).toEqual([]);
    expect(controller.snapshot().selectedMachine).toBe('sb2000');
  });

  it('打开拖入的卡带：请求带上拖入文件的编号，成功后直接运行并刷新最近打开', async () => {
    await attach();
    fake.droppedFiles.set(4, 'D:\\roms\\dropped.nes');
    controller.openDroppedCartridge(4);
    await settle();
    expect(fake.calls).toContain('cartridges.open {"droppedId":4}');
    expect(controller.snapshot().recent).toEqual([{ id: 1, name: 'dropped.nes' }]);
    expect(controller.machineState()).toBe('running');
    expect(controller.snapshot().selectedMachine).toBe('fc');
  });

  it('拖入的文件编号失效：通知提示，机器状态不变', async () => {
    await attach();
    const before = controller.snapshot().status;
    controller.openDroppedCartridge(9);
    await settle();
    expect(context.notices).toEqual(['打开卡带失败：拖入的文件已失效，请重新拖入']);
    expect(controller.snapshot().status).toEqual(before);
  });

  it('加载失败：通知提示，并重新读取最近打开的清单（主进程已把它移出）', async () => {
    fake.recentPaths = ['D:\\roms\\bad.nes'];
    await attach();
    expect(controller.snapshot().recent).toHaveLength(1);
    fake.recentPaths = [];
    fake.failures.set('cartridges.open', { code: 'cartridges.notInes', message: '文件不是 NES 卡带' });
    controller.openCartridge();
    await settle();
    expect(context.notices).toEqual(['打开卡带失败：文件不是 NES 卡带']);
    expect(controller.snapshot().recent).toEqual([]);
  });

  it('重新打开最近的卡带：按编号请求，装入成功', async () => {
    fake.recentPaths = ['D:\\roms\\a.nes', 'D:\\roms\\b.nes'];
    await attach();
    controller.openRecent(2);
    await settle();
    expect(fake.calls).toContain('cartridges.openRecent {"id":2}');
    expect(controller.snapshot().recent.map((item) => item.name)).toEqual(['b.nes', 'a.nes']);
    expect(controller.snapshot().status?.cartridge?.path).toBe('D:\\roms\\b.nes');
  });

  it('最近打开的卡带加载失败时通知', async () => {
    fake.recentPaths = ['D:\\roms\\a.nes'];
    await attach();
    fake.failures.set('cartridges.openRecent', { code: 'cartridges.fileNotFound', message: '找不到文件' });
    controller.openRecent(1);
    await settle();
    expect(context.notices).toEqual(['打开卡带失败：找不到文件']);
  });

  it('其他来源装入的卡带（如启动参数）同样刷新状态与清单，已选机型变成经典 FC', async () => {
    await attach();
    controller.selectMachine('sb2000');
    fake.recentPaths = ['D:\\roms\\boot.nes'];
    fake.status = machineStatus('on', 'running', 'D:\\roms\\boot.nes');
    fake.emit('cartridges.loaded', { cartridge: fake.status.cartridge });
    await settle();
    expect(controller.machineState()).toBe('running');
    expect(controller.snapshot().recent).toHaveLength(1);
    expect(controller.snapshot().selectedMachine).toBe('fc');
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

  it('调整音量失败时通知', async () => {
    await attach();
    fake.failures.set('output.setVolume', { code: 'core.internal', message: '声音设备不可用' });
    controller.toggleMute();
    await settle();
    expect(context.notices).toEqual(['调整音量失败：声音设备不可用']);
  });
});

describe('Family BASIC 键盘', () => {
  it('接上后读出首选项的取值；切换时保存到首选项，状态跟随返回值', async () => {
    fake.familyKeyboard = true;
    await attach();
    expect(controller.snapshot().familyKeyboard).toBe(true);
    controller.toggleFamilyKeyboard();
    await settle();
    expect(fake.calls).toContain('preferences.set {"id":"input.familyKeyboard","value":false}');
    expect(controller.snapshot().familyKeyboard).toBe(false);
    controller.toggleFamilyKeyboard();
    await settle();
    expect(controller.snapshot().familyKeyboard).toBe(true);
  });

  it('在其他地方（首选项页）改了取值同样跟随，别的设置项的变化不影响它', async () => {
    await attach();
    fake.emit('preferences.changed', { id: 'input.familyKeyboard', value: true });
    expect(controller.snapshot().familyKeyboard).toBe(true);
    fake.emit('preferences.changed', { id: 'input.captureMouse', value: false });
    expect(controller.snapshot().familyKeyboard).toBe(true);
  });

  it('保存失败时通知提示，状态不变', async () => {
    await attach();
    fake.failures.set('preferences.set', { code: 'core.internal', message: '写不了配置' });
    controller.toggleFamilyKeyboard();
    await settle();
    expect(context.notices).toEqual(['设置 Family BASIC 键盘失败：写不了配置']);
    expect(controller.snapshot().familyKeyboard).toBe(false);
  });
});

describe('核心重启', () => {
  it('核心进入启动中时清掉旧状态，就绪后重新读取状态、清单与音量', async () => {
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
  });

  it('核心不可用时状态保持“还没读到”', async () => {
    await attach();
    fake.emitCoreStatus({ state: 'unavailable', error: { code: 'core.unavailable', message: '核心反复崩溃，已停止重启' } });
    expect(controller.snapshot().status).toBeUndefined();
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
