/*
 * ------------------------------------------------------------------------
 * 名称：dialog-window.ts
 * 说明：对话框窗口：只放行组件库打开的空白对话框窗口（名称 ui-dialog-序号），按窗口特性设置模态、可否关闭、
 *       可否调整大小与最大化、最小尺寸；内容量好尺寸后经 resizeTo 请求时调整内容区、在所属窗口上居中后显示，显示后内容变化再请求时
 *       上边不动、向下增长，超出屏幕可用区域才整体上移；修改全部对话框窗口的底色；有焦点的对话框关闭后把焦点还给打开它的窗口。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-18
 * 备注：对话框窗口没有系统标题栏，标题栏由组件库绘制（拖动用 app-region）；不能最小化，不在任务栏单独显示；
 *       非模态的属于主窗口，模态的属于打开时拥有焦点的主窗口或对话框窗口；对话框窗口禁止跳转与再打开页面窗口，HTTP 与 HTTPS 外链交给系统浏览器。
 *       窗口特性的写法见组件库的 dialog-popup.ts。
 * ------------------------------------------------------------------------
 */

import { BrowserWindow, screen, shell } from 'electron';
import { isOpenableExternalUrl } from '../help/help-site';

/** 对话框窗口的名称：组件库按序号命名。 */
const DIALOG_NAME_PATTERN = /^ui-dialog-\d{1,9}$/;
/** 内容区尺寸的允许范围（像素）。 */
const MIN_SIZE = 40;
const MAX_SIZE = 4000;
/** 窗口特性缺失时的内容区尺寸。 */
const DEFAULT_WIDTH = 420;
const DEFAULT_HEIGHT = 160;
/** 新窗口相对已显示的对话框错开的距离。 */
const CASCADE_OFFSET = 24;
/** 内容一直没有请求尺寸时（如出错），最晚多久显示窗口（毫秒）。 */
const SHOW_FALLBACK_MS = 1500;

/** 可以判断是否已销毁的窗口。 */
interface Destroyable {
  isDestroyed(): boolean;
}

/**
 * 有焦点的对话框关闭后要聚焦的窗口：打开它的窗口，已销毁时为主窗口。
 * @param opener 打开对话框时有焦点的主窗口或对话框窗口。
 * @param parent 主窗口。
 * @returns 要聚焦的窗口；都已销毁时为 undefined。
 */
export function returnFocusTarget<T extends Destroyable>(opener: T, parent: T): T | undefined {
  if (!opener.isDestroyed()) return opener;
  return parent.isDestroyed() ? undefined : parent;
}

/** 解析后的对话框窗口参数（尺寸为内容区像素）。 */
export interface DialogWindowRequest {
  readonly width: number;
  readonly height: number;
  readonly minWidth: number;
  readonly minHeight: number;
  readonly modal: boolean;
  readonly closable: boolean;
  readonly resizable: boolean;
  readonly maximizable: boolean;
}

/** 创建对话框窗口的参数。 */
export interface DialogWindowOptions {
  /** 当前主题的窗口底色。 */
  readonly background: () => string;
  /** 窗口图标；不传时用可执行文件的图标。 */
  readonly icon?: string;
}

/** 各主窗口的对话框窗口（按名称）。 */
const dialogsByParent = new WeakMap<BrowserWindow, Map<string, BrowserWindow>>();

/**
 * 把允许的 HTTP 或 HTTPS 地址交给系统浏览器。
 * @param url 对话框请求打开的地址。
 * @returns 是否是允许的外链。
 */
export function openDialogExternalUrl(url: string): boolean {
  if (!isOpenableExternalUrl(url)) return false;
  void shell.openExternal(url);
  return true;
}

/** 限定范围内的整数；不是有限数字时为 undefined。 */
function readSize(value: string | undefined): number | undefined {
  const size = Number(value);
  if (value === undefined || value === '' || !Number.isFinite(size)) return undefined;
  return Math.min(Math.max(Math.round(size), MIN_SIZE), MAX_SIZE);
}

/**
 * 解析打开窗口的请求：只接受组件库的空白对话框窗口。
 * @param details 打开窗口的地址、名称与窗口特性。
 * @returns 参数；不是对话框窗口时为 undefined。
 */
export function parseDialogWindowRequest(details: { url: string; frameName: string; features: string }): DialogWindowRequest | undefined {
  if (details.url !== 'about:blank' || !DIALOG_NAME_PATTERN.test(details.frameName)) return undefined;
  const values = new Map<string, string>();
  for (const part of details.features.split(',')) {
    const [key = '', value = ''] = part.split('=');
    values.set(key.trim().toLowerCase(), value.trim());
  }
  const flag = (key: string, fallback: boolean) => (values.has(key) ? values.get(key) === '1' : fallback);
  const width = readSize(values.get('width')) ?? DEFAULT_WIDTH;
  const height = readSize(values.get('height')) ?? DEFAULT_HEIGHT;
  const modal = flag('ui-modal', false);
  const resizable = flag('ui-resizable', true);
  return {
    width,
    height,
    minWidth: Math.min(readSize(values.get('ui-min-width')) ?? width, width),
    minHeight: Math.min(readSize(values.get('ui-min-height')) ?? height, height),
    modal,
    closable: flag('ui-closable', true),
    resizable,
    maximizable: flag('ui-maximizable', false) && resizable && !modal
  };
}

/** 把窗口放在所属窗口中间，按已显示的对话框数错开，并留在屏幕工作区内。 */
function placeOver(child: BrowserWindow, owner: BrowserWindow, cascade: number): void {
  if (owner.isDestroyed()) return;
  const ownerBounds = owner.getBounds();
  const size = child.getBounds();
  const area = screen.getDisplayMatching(ownerBounds).workArea;
  const offset = CASCADE_OFFSET * cascade;
  const x = Math.round(ownerBounds.x + (ownerBounds.width - size.width) / 2 + offset);
  const y = Math.round(ownerBounds.y + (ownerBounds.height - size.height) / 2 + offset);
  child.setPosition(
    Math.min(Math.max(x, area.x), Math.max(area.x, area.x + area.width - size.width)),
    Math.min(Math.max(y, area.y), Math.max(area.y, area.y + area.height - size.height))
  );
}

/** 一个对话框窗口相关的窗口：所属窗口、打开它的窗口与主窗口；以及记录与读取“最后获得焦点的窗口”。 */
interface DialogWindowRelations {
  readonly owner: BrowserWindow;
  readonly opener: BrowserWindow;
  readonly parent: BrowserWindow;
  readonly focus: {
    get(): BrowserWindow;
    set(window: BrowserWindow): void;
  };
}

/** 设置新建的对话框窗口：最小尺寸、安全限制、按内容请求调整尺寸并显示、关闭后还回焦点。 */
function setupDialogWindow(child: BrowserWindow, name: string, relations: DialogWindowRelations, request: DialogWindowRequest, dialogs: Map<string, BrowserWindow>): void {
  const { owner, opener, parent, focus } = relations;
  dialogs.set(name, child);
  child.on('focus', () => focus.set(child));
  const [outerWidth = 0, outerHeight = 0] = child.getSize();
  const [contentWidth = 0, contentHeight = 0] = child.getContentSize();
  child.setMinimumSize(request.minWidth + outerWidth - contentWidth, request.minHeight + outerHeight - contentHeight);
  child.webContents.setWindowOpenHandler(({ url }) => {
    openDialogExternalUrl(url);
    return { action: 'deny' };
  });
  child.webContents.on('will-navigate', (event, url) => {
    event.preventDefault();
    openDialogExternalUrl(url);
  });

  let shown = false;
  const show = () => {
    if (shown || child.isDestroyed()) return;
    shown = true;
    const cascade = [...dialogs.values()].filter((dialog) => dialog !== child && !dialog.isDestroyed() && dialog.isVisible()).length;
    placeOver(child, owner, cascade);
    child.show();
  };
  const fallback = setTimeout(show, SHOW_FALLBACK_MS);
  // 组件库量好内容后用 resizeTo 传入内容区尺寸，不按浏览器的外框尺寸处理。
  child.webContents.on('content-bounds-updated', (event, bounds) => {
    event.preventDefault();
    if (child.isDestroyed()) return;
    const area = screen.getDisplayMatching(child.getBounds()).workArea;
    const width = Math.min(Math.max(Math.round(bounds.width), request.minWidth), area.width);
    const height = Math.min(Math.max(Math.round(bounds.height), request.minHeight), area.height);
    child.setContentSize(width, height);
    if (!shown) {
      clearTimeout(fallback);
      show();
      return;
    }
    // 内容变高后窗口上边不动、向下增长；下边超出屏幕可用区域时才整体上移。
    const placed = child.getBounds();
    const overflow = placed.y + placed.height - (area.y + area.height);
    if (overflow > 0) child.setPosition(placed.x, Math.max(area.y, placed.y - overflow));
  });
  child.once('closed', () => {
    clearTimeout(fallback);
    if (dialogs.get(name) === child) dialogs.delete(name);
    // 组件库从主窗口脚本关闭对话框时不发 close 事件，系统又可能把激活交给别的应用；最后有焦点的是它时，把焦点还给打开它的窗口。
    if (focus.get() !== child) return;
    const target = returnFocusTarget(opener, parent);
    if (target === undefined) return;
    focus.set(target);
    target.focus();
  });
}

/**
 * 允许主窗口的界面打开对话框窗口，其他新窗口一律拒绝。
 * @param parent 主窗口。
 * @param options 创建参数。
 */
export function allowDialogWindows(parent: BrowserWindow, options: DialogWindowOptions): void {
  const dialogs = new Map<string, BrowserWindow>();
  dialogsByParent.set(parent, dialogs);
  const pending = new Map<string, { request: DialogWindowRequest; owner: BrowserWindow; opener: BrowserWindow }>();
  let lastFocused: BrowserWindow = parent;
  const focus = {
    get: () => lastFocused,
    set: (window: BrowserWindow) => {
      lastFocused = window;
    }
  };
  parent.on('focus', () => focus.set(parent));
  // 打开时正在使用的窗口（主窗口或另一个对话框窗口）：模态窗口挡住它，关闭后焦点回到它。
  const focusedOwner = (): BrowserWindow => {
    const focused = BrowserWindow.getFocusedWindow();
    return focused !== null && (focused === parent || [...dialogs.values()].includes(focused)) ? focused : parent;
  };

  parent.webContents.setWindowOpenHandler((details) => {
    const request = parseDialogWindowRequest(details);
    if (request === undefined) return { action: 'deny' };
    const opener = focusedOwner();
    const owner = request.modal ? opener : parent;
    pending.set(details.frameName, { request, owner, opener });
    return {
      action: 'allow',
      overrideBrowserWindowOptions: {
        parent: owner,
        modal: request.modal,
        show: false,
        useContentSize: true,
        width: request.width,
        height: request.height,
        resizable: request.resizable,
        maximizable: request.maximizable,
        minimizable: false,
        fullscreenable: false,
        closable: request.closable,
        skipTaskbar: true,
        // 标题栏由组件库绘制：Windows 保留系统的边框、阴影与调整大小，macOS 不显示红绿灯。
        ...(process.platform === 'darwin' ? { frame: false } : { titleBarStyle: 'hidden' as const }),
        backgroundColor: options.background(),
        ...(options.icon ? { icon: options.icon } : {}),
        webPreferences: { contextIsolation: true, sandbox: true, nodeIntegration: false, webSecurity: true, spellcheck: false }
      }
    };
  });

  parent.webContents.on('did-create-window', (child, details) => {
    const entry = pending.get(details.frameName);
    pending.delete(details.frameName);
    if (entry === undefined) {
      child.destroy();
      return;
    }
    setupDialogWindow(child, details.frameName, { owner: entry.owner, opener: entry.opener, parent, focus }, entry.request, dialogs);
  });

  parent.once('closed', () => {
    for (const dialog of dialogs.values()) if (!dialog.isDestroyed()) dialog.destroy();
    dialogs.clear();
  });
}

/**
 * 修改主窗口全部对话框窗口的底色。
 * @param parent 主窗口。
 * @param color 底色。
 */
export function setDialogWindowsBackground(parent: BrowserWindow, color: string): void {
  for (const dialog of dialogsByParent.get(parent)?.values() ?? []) if (!dialog.isDestroyed()) dialog.setBackgroundColor(color);
}
