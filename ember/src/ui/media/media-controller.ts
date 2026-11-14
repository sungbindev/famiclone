/*
 * ------------------------------------------------------------------------
 * 名称：media-controller.ts
 * 说明：设备边栏的控制器：保存各软驱与打印机的状态，执行载入镜像、取出、保存镜像、写保护等操作，订阅核心事件，并把结果写进底部面板的“消息”。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：不依赖 React，状态放在外部存储里（subscribe 与 snapshot，配合 useSyncExternalStore）；由常驻组件 MediaHost 在外壳启动后接上预加载脚本的接口与外壳的能力（attach）。
 *       软驱的读写状态与磁道、打印机的打印状态随机器运行不断变化，核心不推送，所以设备边栏显示期间由 startPolling 定时读取；边栏没有显示时不读取。
 *       失败时用通知提示并写一条错误消息；打印机每输出一页，用通知告知文件名并写一条消息（规则见 docs/05-界面.md 第 4.3 节）。
 * ------------------------------------------------------------------------
 */

import type { FloppyStatus, PrinterStatus } from '../../contracts/media/generated/media';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { ShellMessageInput } from '../shell/shell-registry';

/** 控制器用到的预加载脚本接口。 */
export type MediaBackend = Pick<EmberBridge, 'core' | 'main' | 'onCoreEvent' | 'onCoreStatusChange' | 'getCoreStatus'>;

/** 控制器需要外壳提供的能力。 */
export interface MediaHostActions {
  /** 往底部面板的“消息”页签追加一条消息。 */
  addMessage(message: ShellMessageInput): void;
  /** 用通知提示。 */
  notify(text: string, kind: 'info' | 'error'): void;
}

/** 设备边栏需要显示的全部状态。 */
export interface MediaViewState {
  /** 各软驱的状态，按编号排列；还没读到、没有开机或主机没有软驱时为空。 */
  readonly floppies: readonly FloppyStatus[];
  /** 打印机的状态；还没读到时为 undefined。 */
  readonly printer: PrinterStatus | undefined;
}

/** 定时读取状态的间隔（毫秒）。 */
const POLL_INTERVAL_MS = 500;

/** 用户可见的文字。 */
const TEXT = {
  failed: (action: string, reason: string) => `${action}失败：${reason}`,
  actions: {
    insert: '载入软盘镜像',
    eject: '取出软盘',
    save: '保存软盘镜像',
    writeProtect: '设置写保护',
    printer: '打印机输出'
  },
  messages: {
    inserted: (drive: string, name: string) => `已把 ${name} 载入软驱 ${drive}`,
    ejected: (drive: string) => `已取出软驱 ${drive} 的盘片`,
    saved: (drive: string, name: string) => `已保存软驱 ${drive} 的镜像 ${name}`,
    protectOn: (drive: string) => `软驱 ${drive} 的盘片已设为写保护`,
    protectOff: (drive: string) => `软驱 ${drive} 的盘片已取消写保护`,
    printed: (file: string, page: number) => `打印机输出了第 ${page} 页：${file}`
  }
} as const;

/** 驱动器的显示名：0 为 A，1 为 B。 */
export function driveLabel(drive: number): string {
  return String.fromCharCode(65 + drive);
}

/** 错误的说明文字：核心请求失败时以 {code, message} 拒绝。 */
function errorText(error: unknown): string {
  if (typeof error === 'object' && error !== null && 'message' in error) return String((error as { message: unknown }).message);
  return String(error);
}

/** 事件数据里的字符串与数字字段；格式不对时为 undefined。 */
function field(data: unknown, key: string, type: 'string' | 'number'): string | number | undefined {
  if (typeof data !== 'object' || data === null) return undefined;
  const value = (data as Record<string, unknown>)[key];
  return typeof value === type ? (value as string | number) : undefined;
}

/** 接上之后的依赖与取消订阅的函数。 */
interface Attachment {
  readonly backend: MediaBackend;
  readonly host: MediaHostActions;
  readonly stop: () => void;
}

/** 设备边栏的控制器。 */
export class MediaController {
  private state: MediaViewState = { floppies: [], printer: undefined };
  private readonly listeners = new Set<() => void>();
  private attachment: Attachment | undefined;
  /** 最近一次读取的编号：迟到的旧结果不覆盖新结果。 */
  private floppyRequest = 0;
  private printerRequest = 0;
  /** 正在显示设备边栏的数量（读取状态的定时器只在大于 0 时运行）。 */
  private pollers = 0;
  private timer: ReturnType<typeof setInterval> | undefined;

  /** 订阅状态变化；返回取消订阅的函数。 */
  readonly subscribe = (listener: () => void): (() => void) => {
    this.listeners.add(listener);
    return () => {
      this.listeners.delete(listener);
    };
  };

  /** 当前状态；内容变化时返回新对象。 */
  readonly snapshot = (): MediaViewState => this.state;

  /**
   * 接上预加载脚本的接口与外壳的能力，订阅核心事件与核心运行状态，并读取初始状态。
   * @returns 断开的函数。
   */
  attach(backend: MediaBackend, host: MediaHostActions): () => void {
    this.detach();
    const stopEvents = backend.onCoreEvent((name, data) => this.handleEvent(name, data));
    const stopStatus = backend.onCoreStatusChange((status) => {
      if (status.state === 'ready') this.refreshAll();
      else this.clear();
    });
    this.attachment = {
      backend,
      host,
      stop: () => {
        stopEvents();
        stopStatus();
      }
    };
    this.refreshAll();
    if (this.pollers > 0 && this.timer === undefined) this.timer = setInterval(() => this.refreshAll(), POLL_INTERVAL_MS);
    return () => this.detach();
  }

  /** 断开：取消订阅与定时读取，之后的请求结果不再处理。 */
  detach(): void {
    this.attachment?.stop();
    this.attachment = undefined;
    this.floppyRequest += 1;
    this.printerRequest += 1;
    this.stopTimer();
  }

  /**
   * 设备边栏显示期间定时读取软驱与打印机的状态。
   * @returns 停止的函数。
   */
  startPolling(): () => void {
    this.pollers += 1;
    if (this.timer === undefined && this.attachment !== undefined) this.timer = setInterval(() => this.refreshAll(), POLL_INTERVAL_MS);
    this.refreshAll();
    let stopped = false;
    return () => {
      if (stopped) return;
      stopped = true;
      this.pollers -= 1;
      if (this.pollers === 0) this.stopTimer();
    };
  }

  /** 弹出系统文件选择窗口，把选定的镜像载入软驱；用户取消时什么都不发生。 */
  insertFloppy(drive: number): void {
    const attachment = this.attachment;
    if (attachment === undefined) return;
    attachment.backend.main.media.openFloppy({ drive }).then(
      (result) => {
        if (!result.canceled && result.status !== undefined) {
          attachment.host.addMessage({ level: 'info', text: TEXT.messages.inserted(driveLabel(drive), result.status.fileName ?? '') });
        }
      },
      (error: unknown) => this.report(TEXT.actions.insert, error)
    );
  }

  /** 取出软驱里的盘片；没有保存的修改随之丢失。 */
  ejectFloppy(drive: number): void {
    this.perform(TEXT.actions.eject, (backend) => backend.core.media.floppyEject({ drive }), () => TEXT.messages.ejected(driveLabel(drive)));
  }

  /** 把软驱里的镜像写回放入时的文件。 */
  saveFloppy(drive: number): void {
    const name = this.state.floppies.find((item) => item.drive === drive)?.fileName ?? '';
    this.perform(TEXT.actions.save, (backend) => backend.core.media.floppySave({ drive }), () => TEXT.messages.saved(driveLabel(drive), name));
  }

  /** 设置或取消盘片的写保护。 */
  setWriteProtected(drive: number, writeProtected: boolean): void {
    this.perform(
      TEXT.actions.writeProtect,
      (backend) => backend.core.media.floppyWriteProtect({ drive, writeProtected }),
      () => (writeProtected ? TEXT.messages.protectOn(driveLabel(drive)) : TEXT.messages.protectOff(driveLabel(drive)))
    );
  }

  /**
   * 执行一个操作：成功后写一条消息，失败时用通知提示并写一条错误消息；结束后重新读取状态。
   * @param name 操作名，用在失败的说明里。
   * @param action 发出请求。
   * @param success 成功时写入消息的文字。
   */
  private perform(name: string, action: (backend: MediaBackend) => Promise<unknown>, success: () => string): void {
    const attachment = this.attachment;
    if (attachment === undefined) return;
    action(attachment.backend).then(
      () => attachment.host.addMessage({ level: 'info', text: success() }),
      (error: unknown) => this.report(name, error)
    ).finally(() => {
      if (this.attachment === attachment) this.refreshAll();
    });
  }

  /** 报告失败：通知加一条错误消息。 */
  private report(name: string, error: unknown): void {
    const text = TEXT.failed(name, errorText(error));
    this.attachment?.host.notify(text, 'error');
    this.attachment?.host.addMessage({ level: 'error', text });
  }

  private handleEvent(name: string, data: unknown): void {
    if (name === 'media.changed' || name === 'runtime.stateChanged' || name === 'machines.changed') {
      this.refreshAll();
    } else if (name === 'media.printed') {
      this.refreshPrinter();
      const file = field(data, 'fileName', 'string');
      const page = field(data, 'page', 'number');
      if (typeof file === 'string' && typeof page === 'number') {
        const text = TEXT.messages.printed(file, page);
        this.attachment?.host.notify(text, 'info');
        this.attachment?.host.addMessage({ level: 'info', text });
      }
    } else if (name === 'media.printFailed') {
      const message = field(data, 'message', 'string');
      this.report(TEXT.actions.printer, { message: typeof message === 'string' ? message : '' });
    }
  }

  /** 读取软驱与打印机的状态。 */
  private refreshAll(): void {
    this.refreshFloppies();
    this.refreshPrinter();
  }

  private refreshFloppies(): void {
    const backend = this.attachment?.backend;
    if (backend === undefined) return;
    this.floppyRequest += 1;
    const current = this.floppyRequest;
    backend.core.media.floppyStatus({}).then(
      (result) => {
        if (this.attachment?.backend === backend && this.floppyRequest === current) this.setState({ floppies: result.drives });
      },
      () => undefined
    );
  }

  private refreshPrinter(): void {
    const backend = this.attachment?.backend;
    if (backend === undefined) return;
    this.printerRequest += 1;
    const current = this.printerRequest;
    backend.core.media.printerStatus({}).then(
      (result) => {
        if (this.attachment?.backend === backend && this.printerRequest === current) this.setState({ printer: result.status });
      },
      () => undefined
    );
  }

  /** 核心不可用或重启中：清掉旧状态。 */
  private clear(): void {
    this.floppyRequest += 1;
    this.printerRequest += 1;
    this.setState({ floppies: [], printer: undefined });
  }

  private stopTimer(): void {
    if (this.timer !== undefined) clearInterval(this.timer);
    this.timer = undefined;
  }

  /** 合并新状态；内容没有变化时不通知，避免定时读取引起无谓的重新渲染。 */
  private setState(patch: Partial<MediaViewState>): void {
    const next = { ...this.state, ...patch };
    if (JSON.stringify(next) === JSON.stringify(this.state)) return;
    this.state = next;
    for (const listener of this.listeners) listener();
  }
}
