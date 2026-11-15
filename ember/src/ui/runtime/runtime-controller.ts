/*
 * ------------------------------------------------------------------------
 * 名称：runtime-controller.ts
 * 说明：运行模块的控制器：保存机器状态、最近打开的清单、机型清单与已选机型、音量，执行开机、断电、暂停、复位、取出卡带、打开卡带、调音量等操作，订阅核心事件与核心运行状态。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：不依赖 React，状态放在外部存储里（subscribe 与 snapshot，配合 useSyncExternalStore），运行面板、状态栏、画面提示与菜单都从这里取数据；
 *       由常驻组件 RuntimeHost 在外壳启动后接上预加载脚本的接口与外壳的能力（attach），所以面板没有展开、显示区没有显示时状态也是最新的。
 *       “选择机型”只改已选机型（保存在界面状态里，下次启动沿用），不开机、不装入、不影响正在运行的机器；打开卡带成功（cartridges.loaded）后已选机型自动变成经典 FC。
 *       开机与断电是同一个操作（togglePower）：通电时断电；未通电时，已选机型是整机机型（学习机）就用它的 BIOS 开机（不需要卡带，当前装的是别的主机也用它重新开机），
 *       已选机型是经典 FC 就要求已经装入 FC 卡带（从“文件→打开卡带”选 nes，选后直接开机），断电后再开机直接上电。
 *       操作失败时用通知提示；核心进程重启时状态先清成“未读到”，核心就绪后重新读取状态、音量与最近打开的清单。
 * ------------------------------------------------------------------------
 */

import type { CoreStatus } from '../../contracts/core-gateway/core-messages';
import type { RuntimeStatusResult } from '../../contracts/runtime/generated/runtime';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { controlAvailability, machineState, powerToggleAction, type ControlAvailability, type MachineState } from './runtime-status';

/** 控制器用到的预加载脚本接口。 */
export type RuntimeBackend = Pick<EmberBridge, 'core' | 'main' | 'shell' | 'onCoreEvent' | 'onCoreStatusChange'>;

/** 最近打开的清单里的一项（取自预加载脚本接口的返回值，不直接引用 cartridges 模块的契约）：编号与显示名，没有路径。 */
export type RecentCartridge = Awaited<ReturnType<EmberBridge['main']['cartridges']['recent']>>['items'][number];

/** 机型清单里的一项（取自预加载脚本接口的返回值，不直接引用 machines 模块的契约）：标识、名称、是否需要 BIOS、是否已支持与已添加的 BIOS。 */
export type MachineEntry = Awaited<ReturnType<EmberBridge['core']['machines']['list']>>['machines'][number];

/** 控制器需要外壳提供的能力。 */
export interface RuntimeHostActions {
  /** 用通知提示错误。 */
  notify(text: string, kind: 'error'): void;
  /** 状态变化后让外壳重新计算菜单里命令的名称与可用状态。 */
  refreshCommands(): void;
}

/** 运行模块需要显示的全部状态。 */
export interface RuntimeViewState {
  /** 机器状态；还没读到（核心启动中、重启中或不可用）时为 undefined。 */
  readonly status: RuntimeStatusResult | undefined;
  /** 最近打开的卡带，最近的在前。 */
  readonly recent: readonly RecentCartridge[];
  /** 机型清单（经典 FC 与四个整机机型，顺序固定）；还没读到时为空。 */
  readonly machines: readonly MachineEntry[];
  /** 已选机型的标识（“文件→选择机型”），没选过时是 fc。 */
  readonly selectedMachine: string;
  /** 音量，0 到 100。 */
  readonly volume: number;
  /** 是否静音；静音时音量保持原值。 */
  readonly muted: boolean;
  /** 是否给经典 FC 接上了 Family BASIC 键盘（首选项 input.familyKeyboard）。 */
  readonly familyKeyboard: boolean;
}

/** 经典 FC 的机型标识：选它时要求已经装入 FC 卡带。 */
export const CLASSIC_MACHINE = 'fc';

/** 用户可见的文字。 */
const TEXT = {
  failed: (action: string, reason: string) => `${action}失败：${reason}`,
  actions: {
    powerOn: '开机',
    powerOff: '断电',
    pause: '暂停',
    resume: '继续',
    softReset: '软复位',
    hardReset: '硬复位',
    eject: '关闭卡带',
    open: '打开卡带',
    boot: '开机',
    volume: '调整音量',
    familyKeyboard: '设置 Family BASIC 键盘'
  }
} as const;

/** 没有读到任何状态时的初始值：音量与核心启动后的默认值一致。 */
const INITIAL_STATE: RuntimeViewState = { status: undefined, recent: [], machines: [], selectedMachine: CLASSIC_MACHINE, volume: 100, muted: false, familyKeyboard: false };

/** 接入 Family BASIC 键盘的首选项标识。 */
const FAMILY_KEYBOARD_SETTING = 'input.familyKeyboard';

/** 错误的说明文字：核心请求失败时以 {code, message} 拒绝。 */
function errorText(error: unknown): string {
  if (typeof error === 'object' && error !== null && 'message' in error) return String((error as { message: unknown }).message);
  return String(error);
}

/** 接上之后的依赖与取消订阅的函数。 */
interface Attachment {
  readonly backend: RuntimeBackend;
  readonly host: RuntimeHostActions;
  readonly stop: () => void;
}

/** 运行模块的控制器。 */
export class RuntimeController {
  private state: RuntimeViewState = INITIAL_STATE;
  private readonly listeners = new Set<() => void>();
  private attachment: Attachment | undefined;
  /** 最近一次读取状态的编号：迟到的旧结果不覆盖新结果。 */
  private statusRequest = 0;
  /** 最近一次调整声音的编号：迟到的旧结果不覆盖新结果。 */
  private soundRequest = 0;
  /** 操作结束后把焦点还给画面。 */
  private screenFocus: (() => void) | undefined;

  /** 订阅状态变化；返回取消订阅的函数。 */
  readonly subscribe = (listener: () => void): (() => void) => {
    this.listeners.add(listener);
    return () => {
      this.listeners.delete(listener);
    };
  };

  /** 当前状态；内容变化时返回新对象。 */
  readonly snapshot = (): RuntimeViewState => this.state;

  /** 当前各控制是否可用。 */
  availability(): ControlAvailability {
    return controlAvailability(this.state.status, this.state.selectedMachine, this.state.machines);
  }

  /** 当前机器状态。 */
  machineState(): MachineState {
    return machineState(this.state.status);
  }

  /** 登记画面区的聚焦函数：操作结束后调用，键盘才能继续控制游戏。 */
  setScreenFocus(focus: (() => void) | undefined): void {
    this.screenFocus = focus;
  }

  /**
   * 接上预加载脚本的接口与外壳的能力，订阅核心事件与核心运行状态，并读取初始状态。
   * @returns 断开的函数。
   */
  attach(backend: RuntimeBackend, host: RuntimeHostActions): () => void {
    this.detach();
    const stopEvents = backend.onCoreEvent((name, data) => this.handleEvent(name, data));
    const stopStatus = backend.onCoreStatusChange((status) => this.handleCoreStatus(status));
    this.attachment = {
      backend,
      host,
      stop: () => {
        stopEvents();
        stopStatus();
      }
    };
    this.refreshAll();
    backend.shell.getState().then(
      (shellState) => {
        const saved = shellState.layout.selectedMachine;
        if (this.attachment?.backend === backend && saved !== null) this.setState({ selectedMachine: saved });
      },
      () => undefined
    );
    return () => this.detach();
  }

  /** 断开：取消订阅，之后的请求结果不再处理。 */
  detach(): void {
    this.attachment?.stop();
    this.attachment = undefined;
    this.statusRequest += 1;
  }

  /** 开机或断电：通电时断电；未通电时按已选机型开机（规则见文件头）。 */
  togglePower(): void {
    const action = powerToggleAction(this.state.status, this.state.selectedMachine, this.state.machines);
    if (action === 'powerOff') this.perform(TEXT.actions.powerOff, (backend) => backend.core.runtime.powerOff({}));
    else if (action === 'powerOn') this.perform(TEXT.actions.powerOn, (backend) => backend.core.runtime.powerOn({ resume: true }));
    else if (action === 'boot') this.bootMachine(this.state.selectedMachine);
  }

  /** 选择机型：只改已选机型并保存，不开机、不装入、不影响正在运行的机器。 */
  selectMachine(id: string): void {
    if (this.state.selectedMachine === id) return;
    this.setState({ selectedMachine: id });
    this.attachment?.backend.shell.setLayout({ selectedMachine: id });
  }

  /** 暂停或继续：连续运行时暂停，否则继续；断电时不可用。 */
  togglePause(): void {
    const status = this.state.status;
    if (status === undefined || status.machine === undefined || status.power !== 'on') return;
    if (status.mode === 'running') this.perform(TEXT.actions.pause, (backend) => backend.core.runtime.pause({}));
    else this.perform(TEXT.actions.resume, (backend) => backend.core.runtime.resume({}));
  }

  /** 软复位：只发复位线，断电时不可用。 */
  softReset(): void {
    if (!this.availability().softReset) return;
    this.perform(TEXT.actions.softReset, (backend) => backend.core.runtime.reset({ kind: 'soft' }));
  }

  /** 硬复位：断电后再上电，通电时才可用。 */
  hardReset(): void {
    if (!this.availability().hardReset) return;
    this.perform(TEXT.actions.hardReset, (backend) => backend.core.runtime.reset({ kind: 'hard' }));
  }

  /** 关闭（取出）卡带；没有卡带时什么都不做。 */
  ejectCartridge(): void {
    if (this.state.status?.cartridge === undefined) return;
    this.perform(TEXT.actions.eject, (backend) => backend.core.cartridges.eject({}));
  }

  /** 弹出系统文件选择窗口打开卡带（选中后直接开机）；用户取消时什么都不发生。 */
  openCartridge(): void {
    this.perform(TEXT.actions.open, (backend) => backend.main.cartridges.open({}), true);
  }

  /** 重新打开最近打开清单里的卡带。 */
  openRecent(id: number): void {
    this.perform(TEXT.actions.open, (backend) => backend.main.cartridges.openRecent({ id }), true);
  }

  /** 整机机型开机：用已添加的 BIOS 装配主机并上电运行；没有这个机型时什么都不做。 */
  private bootMachine(id: string): void {
    if (!this.state.machines.some((item) => item.id === id)) return;
    this.perform(TEXT.actions.boot, (backend) => backend.core.machines.boot({ machine: id }));
  }

  /** 调整音量（0 到 100）：先在界面上生效，再交给核心，核心返回的值为准；静音状态不变。 */
  setVolume(value: number): void {
    const volume = Math.min(100, Math.max(0, Math.round(value)));
    this.setState({ volume });
    this.adjustSound({ volume });
  }

  /** 切换静音：音量保持原值，取消静音时回到原音量。 */
  toggleMute(): void {
    this.adjustSound({ muted: !this.state.muted });
  }

  /** 接入或拔下 Family BASIC 键盘：保存到首选项，核心据此接到经典 FC 的扩展口；状态以首选项变化事件为准。 */
  toggleFamilyKeyboard(): void {
    const backend = this.attachment?.backend;
    if (backend === undefined) return;
    backend.core.preferences.set({ id: FAMILY_KEYBOARD_SETTING, value: !this.state.familyKeyboard }).then(
      (result) => {
        if (this.attachment?.backend === backend) this.setState({ familyKeyboard: result.value === true });
      },
      (error: unknown) => this.report(TEXT.actions.familyKeyboard, error)
    );
  }

  /** 把声音设置交给核心，并以核心返回的值更新状态；连续调整时只采用最后一次的结果。 */
  private adjustSound(params: { volume?: number; muted?: boolean }): void {
    const backend = this.attachment?.backend;
    if (backend === undefined) return;
    this.soundRequest += 1;
    const current = this.soundRequest;
    backend.core.output.setVolume(params).then(
      (result) => {
        if (this.attachment?.backend === backend && this.soundRequest === current) this.setState({ volume: result.volume, muted: result.muted });
      },
      (error: unknown) => this.report(TEXT.actions.volume, error)
    );
  }

  /**
   * 执行一个操作：结束后重新读取状态，失败时用通知提示，并把焦点还给画面。
   * @param name 操作名，用在失败的说明里。
   * @param action 发出请求。
   * @param refreshRecent 结束后是否重新读取最近打开的清单（打开卡带会改变清单）。
   */
  private perform(name: string, action: (backend: RuntimeBackend) => Promise<unknown>, refreshRecent = false): void {
    const attachment = this.attachment;
    if (attachment === undefined) return;
    action(attachment.backend).catch((error: unknown) => this.report(name, error)).finally(() => {
      if (this.attachment !== attachment) return;
      this.refreshStatus();
      if (refreshRecent) this.refreshRecent();
      this.screenFocus?.();
    });
  }

  /** 报告失败：用通知提示。 */
  private report(name: string, error: unknown): void {
    this.attachment?.host.notify(TEXT.failed(name, errorText(error)), 'error');
  }

  private handleEvent(name: string, data: unknown): void {
    if (name === 'preferences.changed') {
      const change = data as { id?: unknown; value?: unknown } | null;
      if (change?.id === FAMILY_KEYBOARD_SETTING) this.setState({ familyKeyboard: change.value === true });
    } else if (name === 'runtime.stateChanged') {
      this.refreshStatus();
    } else if (name === 'cartridges.loaded') {
      this.refreshStatus();
      this.refreshRecent();
      // 打开卡带成功后已选机型自动变成经典 FC。
      this.selectMachine(CLASSIC_MACHINE);
    } else if (name === 'machines.changed') {
      this.refreshMachines();
    }
  }

  /** 核心的运行状态变了：重启过程中先清掉旧状态，就绪后重新读取。 */
  private handleCoreStatus(status: CoreStatus): void {
    if (status.state === 'ready') {
      this.refreshAll();
      return;
    }
    this.statusRequest += 1;
    this.setState({ status: undefined });
  }

  /** 读取状态、最近打开的清单、机型清单与音量。 */
  private refreshAll(): void {
    this.refreshStatus();
    this.refreshRecent();
    this.refreshMachines();
    this.refreshVolume();
    this.refreshFamilyKeyboard();
  }

  /** 读取是否接入 Family BASIC 键盘。 */
  private refreshFamilyKeyboard(): void {
    const backend = this.attachment?.backend;
    if (backend === undefined) return;
    backend.core.preferences.list({}).then(
      (result) => {
        const setting = result.settings.find((item) => item.id === FAMILY_KEYBOARD_SETTING);
        if (this.attachment?.backend === backend && setting !== undefined) this.setState({ familyKeyboard: setting.value === true });
      },
      () => undefined
    );
  }

  /** 读取机型清单。 */
  private refreshMachines(): void {
    const backend = this.attachment?.backend;
    if (backend === undefined) return;
    backend.core.machines.list({}).then(
      (result) => {
        if (this.attachment?.backend === backend) this.setState({ machines: result.machines });
      },
      () => undefined
    );
  }

  /** 读取机器状态；核心没有就绪时读不到，等核心状态推送后再读，不报错。 */
  private refreshStatus(): void {
    const backend = this.attachment?.backend;
    if (backend === undefined) return;
    this.statusRequest += 1;
    const current = this.statusRequest;
    backend.core.runtime.status({}).then(
      (status) => {
        if (this.statusRequest === current && this.attachment?.backend === backend) this.setState({ status });
      },
      () => undefined
    );
  }

  /** 读取最近打开的清单。 */
  private refreshRecent(): void {
    const backend = this.attachment?.backend;
    if (backend === undefined) return;
    backend.main.cartridges.recent({}).then(
      (result) => {
        if (this.attachment?.backend === backend) this.setState({ recent: result.items });
      },
      () => undefined
    );
  }

  /** 读取核心里的音量与静音（setVolume 不带参数时只返回当前值）。 */
  private refreshVolume(): void {
    const backend = this.attachment?.backend;
    if (backend === undefined) return;
    this.soundRequest += 1;
    const current = this.soundRequest;
    backend.core.output.setVolume({}).then(
      (result) => {
        if (this.attachment?.backend === backend && this.soundRequest === current) this.setState({ volume: result.volume, muted: result.muted });
      },
      () => undefined
    );
  }

  private setState(patch: Partial<RuntimeViewState>): void {
    this.state = { ...this.state, ...patch };
    for (const listener of this.listeners) listener();
    this.attachment?.host.refreshCommands();
  }
}
