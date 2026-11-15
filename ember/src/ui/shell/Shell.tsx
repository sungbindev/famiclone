/*
 * ------------------------------------------------------------------------
 * 名称：Shell.tsx
 * 说明：外壳：用组件库的外壳组件拼出窗口框架（标题栏与菜单、中间固定的显示区、底部面板、状态栏、通知），
 *       全部入口都来自登记表（显示区、对话框、命令与快捷键、菜单项、底部面板页签、状态项），自己不写死业务入口；
 *       处理命令与全局快捷键（焦点在文本输入控件里时不带 Ctrl 或 Alt 的快捷键不触发）、子菜单、对话框、模块的常驻组件、命令状态的刷新、窗口状态与底部面板高度的保存；全屏时只显示显示区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：规则见 docs/05-界面.md 第 2、3、5、6、9 节；没有活动栏、边栏、标签栏与右边栏；
 *       底部面板只经系统栏左侧的面板按钮展开：面板关着时点按钮展开并切到该页签，开着时点同一个按钮收起，点另一个按钮切页签；没有快捷键，每次启动都是关闭，页签选择不保存；
 *       面板按钮只在面板开着并停在该页签时是按下状态，面板收起后恢复常态；
 *       面板的标题栏有“自动收缩”开关的（面板登记 autoCollapse），开关开着时，点在面板之外（包括显示区的画面）就收起面板，面板里的操作不收起；不算面板之外的有：标题栏、面板高度的拖动条、弹层（下拉选项、通知等）与系统栏里的面板按钮；开关状态按面板保存在界面状态里，下次启动沿用；
 *       其他界面（如首选项）都是对话框：组件库的独立窗口对话框，同一个对话框同时只开一个；
 *       显示区获得焦点时键盘事件先交给显示区（它处理过的按键 defaultPrevented，外壳不再响应）；
 *       文件可以直接拖进主窗口：外壳接住拖放，按扩展名交给模块登记的文件处理项（use-file-drop.ts）；
 *       主窗口里在任何地方点完鼠标（按钮、滑块、空白处等）后，把焦点还给显示区里的可聚焦元素（画面区），键盘才能继续输入到机器，Windows 的键盘独占也跟着焦点；
 *       菜单打开期间不抢焦点（菜单要用键盘操作），菜单关闭后还；点在文字输入控件上不还；面板向内展开，窗口大小不变，显示区里的画面按剩余空间自动缩放。
 * ------------------------------------------------------------------------
 */

import {
  AppFrame,
  Button,
  iconLayoutBottombarCollapse,
  iconX,
  MenuBar,
  NotificationCenter,
  NotificationCenterButton,
  ScrollArea,
  shortcutFromEvent,
  State,
  StatusBar,
  StatusBarItem,
  TitleBar,
  useDialog,
  useNotify,
  type DialogHandle,
  type MenuBarMenu,
  type MenuItem
} from '@rujianjs/ui-kit';
import { createElement, useCallback, useEffect, useLayoutEffect, useMemo, useRef, useState, type ComponentType, type ReactElement } from 'react';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { WindowState } from '../../contracts/shell/window-control';
import { buildShellRegistry, type ShellRegistry } from './build-registry';
import { isSelectTrigger, isTextEditingTarget } from './editable-target';
import { EmberLogo } from './EmberLogo';
import { ShellApiContext } from './shell-context';
import { SHELL_MODULE } from './shell-module';
import { commandTitle, SHELL_MENUS, type ShellApi, type ShellModule, type ShellPageProps, type ShellViewState } from './shell-registry';
import { useFileDrop } from './use-file-drop';
import './shell.css';

/** 应用名。 */
const APP_NAME = 'EMBER';

/** 底部面板的默认高度（像素），同时是最小高度：正好装下设备面板的三行设备（标题行 26、内容留边 18、三行 78、上边框 1，再留 3 像素余量），不出现滚动条；保存过更高的高度时用保存的。 */
const BOTTOM_PANEL_HEIGHT = 126;

/** 用户可见的文字。 */
const TEXT = {
  statusLabel: '系统栏',
  closePanel: '关闭',
  autoCollapse: '自动收缩',
  registryFailed: '外壳登记有误，界面无法显示，详情见开发者工具的控制台'
} as const;

/** 外壳的属性。 */
export interface ShellProps {
  /** 预加载脚本暴露的接口。 */
  readonly bridge: EmberBridge;
  /** 渲染进程的模块清单里各模块的登记项。 */
  readonly modules: readonly ShellModule[];
}

/** 底部面板的状态：是否展开与当前页签；两项都不保存。 */
interface PanelState {
  readonly open: boolean;
  readonly id: string;
}

/** 外壳实时读取的状态：事件处理函数用它取最新值，不必随渲染重新创建。 */
interface LiveState {
  readonly view: ShellViewState;
  /** 开着“自动收缩”的面板标识。 */
  readonly autoCollapse: readonly string[];
  /** 菜单是否打开着。 */
  readonly menuOpen: boolean;
}

/**
 * 外壳。
 * @param props 属性。
 */
export function Shell({ bridge, modules }: ShellProps) {
  const built = useMemo((): { registry: ShellRegistry } | { error: Error } => {
    try {
      return { registry: buildShellRegistry([SHELL_MODULE, ...modules]) };
    } catch (error) {
      return { error: error instanceof Error ? error : new Error(String(error)) };
    }
  }, [modules]);
  useEffect(() => {
    if ('error' in built) console.error(built.error);
  }, [built]);
  if ('error' in built) return <State kind="error" text={TEXT.registryFailed} />;
  return <ShellFrame bridge={bridge} registry={built.registry} />;
}

/** 对话框的内容：补上外壳接口的上下文。 */
function dialogContent(shell: ShellApi, Content: ComponentType<ShellPageProps>, bridge: EmberBridge): ReactElement {
  return (
    <ShellApiContext.Provider value={shell}>
      <Content bridge={bridge} />
    </ShellApiContext.Provider>
  );
}

/** 点在这些元素上不触发自动收缩：面板本身、面板高度的拖动条、弹层、标题栏、系统栏里的面板按钮。 */
const AUTO_COLLAPSE_IGNORED = '.ember-shell-panel, .ui-app-frame__resizer, .ui-layer, .ui-title-bar, [data-panel-button]';

/** 登记表有效之后的外壳。 */
function ShellFrame({ bridge, registry }: { bridge: EmberBridge; registry: ShellRegistry }) {
  const [panel, setPanel] = useState<PanelState>({ open: false, id: registry.bottomPanels[0]?.id ?? '' });
  // 底部面板的初始高度：读到保存的高度之前为 null，此时不显示界面。
  const [bottomHeight, setBottomHeight] = useState<number | null>(null);
  const [windowState, setWindowState] = useState<WindowState>({ maximized: false, fullScreen: false });
  // 开着“自动收缩”的面板标识：读到保存的清单之前都是关闭。
  const [autoCollapse, setAutoCollapse] = useState<readonly string[]>([]);
  const [openMenuId, setOpenMenuId] = useState<string | null>(null);
  // 模块通过 refreshCommands 通知命令状态变了：递增一次让菜单与状态栏重新计算。
  const [, setCommandVersion] = useState(0);
  const dialogApi = useDialog();
  const notify = useNotify();
  const openDialogs = useRef(new Map<string, DialogHandle>());
  useFileDrop(bridge, registry.fileHandlers, notify);

  useEffect(() => {
    let active = true;
    void bridge.shell.getState().then((state) => {
      if (active) setBottomHeight(state.layout.bottomPanelHeight ?? BOTTOM_PANEL_HEIGHT);
      if (active) setAutoCollapse(state.layout.autoCollapsePanels);
    });
    void bridge.shell.getWindowState().then((state) => {
      if (active) setWindowState(state);
    });
    const stopWatching = bridge.shell.onWindowStateChange(setWindowState);
    return () => {
      active = false;
      stopWatching();
    };
  }, [bridge]);

  const bottomPanelOpen = panel.open;
  const view: ShellViewState = { bottomPanelOpen, activePanelId: panel.open ? panel.id : null, fullScreen: windowState.fullScreen };
  const live = useRef<LiveState>({ view, autoCollapse, menuOpen: openMenuId !== null });
  useLayoutEffect(() => {
    live.current = { view, autoCollapse, menuOpen: openMenuId !== null };
  });

  // 显示区里的画面区（显示区里第一个 tabindex=0 的元素）。
  const pageRef = useRef<HTMLDivElement>(null);
  const focusScreen = useCallback((): void => {
    pageRef.current?.querySelector<HTMLElement>('[tabindex="0"]')?.focus({ preventScroll: true });
  }, []);

  // 主窗口里在任何地方点完鼠标后把焦点还给画面区；延到点击处理之后，不影响点击本身的效果。
  useEffect(() => {
    let timer: ReturnType<typeof setTimeout> | undefined;
    const handlePointerUp = (event: PointerEvent): void => {
      if (isTextEditingTarget(event.target) || isSelectTrigger(event.target)) return;
      clearTimeout(timer);
      timer = setTimeout(() => {
        if (!live.current.menuOpen) focusScreen();
      }, 0);
    };
    window.addEventListener('pointerup', handlePointerUp);
    return () => {
      window.removeEventListener('pointerup', handlePointerUp);
      clearTimeout(timer);
    };
  }, [focusScreen]);

  // 菜单关闭后（选中菜单项、Esc、点到菜单外）把焦点还给画面区；菜单组件关闭时会把焦点放回菜单按钮，所以延后一步。
  const menuWasOpen = useRef(false);
  useEffect(() => {
    if (openMenuId !== null) {
      menuWasOpen.current = true;
      return undefined;
    }
    if (!menuWasOpen.current) return undefined;
    menuWasOpen.current = false;
    const timer = setTimeout(focusScreen, 0);
    return () => clearTimeout(timer);
  }, [openMenuId, focusScreen]);

  // 面板开着且它的“自动收缩”开着时，点在面板之外就收起面板；在捕获阶段处理，画面区等地方阻止了冒泡也照样收起。
  useEffect(() => {
    const handlePointerDown = (event: PointerEvent): void => {
      const { view: current, autoCollapse: enabled } = live.current;
      const openId = current.activePanelId;
      if (openId === null || !enabled.includes(openId)) return;
      if (event.target instanceof Element && event.target.closest(AUTO_COLLAPSE_IGNORED) !== null) return;
      setPanel((state) => (state.open && state.id === openId ? { ...state, open: false } : state));
    };
    window.addEventListener('pointerdown', handlePointerDown, true);
    return () => window.removeEventListener('pointerdown', handlePointerDown, true);
  }, []);

  const api = useMemo<ShellApi>(() => {
    const shell: ShellApi = {
      openDialog: (dialogId) => {
        const definition = registry.dialogs.get(dialogId);
        if (definition === undefined) {
          console.error(`对话框不存在：${dialogId}`);
          return;
        }
        const existing = openDialogs.current.get(dialogId);
        if (existing !== undefined) {
          existing.focus();
          return;
        }
        const handle = dialogApi.openDialog({
          title: definition.title,
          // 对话框内容在组件库的对话框层渲染，不在外壳的上下文里，所以这里再包一层。
          content: dialogContent(shell, definition.Component, bridge),
          width: definition.width,
          ...(definition.height === undefined ? {} : { height: definition.height }),
          ...(definition.minWidth === undefined ? {} : { minWidth: definition.minWidth }),
          ...(definition.minHeight === undefined ? {} : { minHeight: definition.minHeight })
        });
        openDialogs.current.set(dialogId, handle);
        void handle.closed.then(() => {
          if (openDialogs.current.get(dialogId) === handle) openDialogs.current.delete(dialogId);
        });
      },
      togglePanel: (panelId) => setPanel((current) => (current.open && current.id === panelId ? { ...current, open: false } : { open: true, id: panelId })),
      toggleFullScreen: () => bridge.shell.windowAction('toggleFullScreen'),
      quit: () => bridge.shell.windowAction('quit'),
      runCommand: (commandId) => {
        const command = registry.commands.get(commandId);
        if (command === undefined || command.enabled?.(live.current.view) === false) return;
        command.run(shell);
      },
      refreshCommands: () => setCommandVersion((version) => version + 1),
      notify
    };
    return shell;
  }, [bridge, registry, dialogApi, notify]);

  // 全局快捷键与 Alt+助记字母；显示区等已经处理过的按键（defaultPrevented）外壳不再响应。
  useEffect(() => {
    const handleKey = (event: KeyboardEvent): void => {
      if (event.defaultPrevented || event.repeat || event.isComposing) return;
      const shortcut = shortcutFromEvent(event);
      if (shortcut === null) return;
      // 焦点在输入控件里时，不带 Ctrl 或 Alt 的快捷键（如 F7）留给控件自己。
      const parts = shortcut.split('+');
      if (!parts.includes('Ctrl') && !parts.includes('Alt') && isTextEditingTarget(event.target)) return;
      const menu = SHELL_MENUS.find((entry) => `Alt+${entry.mnemonic}` === shortcut);
      if (menu !== undefined) {
        if (registry.menuItems.some((item) => item.menu === menu.id)) {
          event.preventDefault();
          setOpenMenuId(menu.id);
        }
        return;
      }
      const commandId = registry.shortcuts.get(shortcut);
      if (commandId === undefined) return;
      event.preventDefault();
      api.runCommand(commandId);
    };
    window.addEventListener('keydown', handleKey);
    return () => window.removeEventListener('keydown', handleKey);
  }, [registry, api]);

  if (bottomHeight === null) return null;

  const menus: MenuBarMenu[] = SHELL_MENUS.flatMap((menu): MenuBarMenu[] => {
    const items = registry.menuItems
      .filter((item) => item.menu === menu.id)
      .flatMap((item, index): MenuItem[] => {
        const separator = item.separatorBefore && index > 0 ? { separatorBefore: true } : {};
        const mnemonic = item.mnemonic ? { mnemonic: item.mnemonic } : {};
        if (item.submenu !== undefined) {
          const { id, title, entries } = item.submenu;
          const shown = entries();
          // 没有子菜单项时这一项置灰。
          if (shown.length === 0) return [{ id, label: title, ...mnemonic, ...separator, disabled: true }];
          return [
            {
              id,
              label: title,
              ...mnemonic,
              ...separator,
              items: shown.map((entry) => ({
                id: `${id}.${entry.id}`,
                label: entry.label,
                ...(entry.hint ? { hint: entry.hint } : {}),
                ...(entry.disabled ? { disabled: true } : {}),
                ...(entry.checked === undefined ? {} : { checked: entry.checked, radio: true }),
                ...(entry.separatorBefore ? { separatorBefore: true } : {})
              }))
            }
          ];
        }
        const command = registry.commands.get(item.command ?? '');
        if (command === undefined) return [];
        const shortcut = command.defaultShortcut;
        const checked = command.checked?.(view);
        return [
          {
            id: command.id,
            label: commandTitle(command),
            ...mnemonic,
            ...(shortcut ? { shortcut } : {}),
            ...separator,
            ...(checked === undefined ? {} : { checked }),
            ...(command.enabled?.(view) === false ? { disabled: true } : {})
          }
        ];
      });
    return items.length === 0 ? [] : [{ id: menu.id, label: menu.label, mnemonic: menu.mnemonic, items }];
  });

  /** 选中菜单项：子菜单的项按“子菜单标识.项标识”找到并执行（项随数据变化，选中时重新取），其余按命令执行。 */
  const handleMenuSelect = (itemId: string): void => {
    for (const item of registry.menuItems) {
      if (item.submenu === undefined || !itemId.startsWith(`${item.submenu.id}.`)) continue;
      const entry = item.submenu.entries().find((candidate) => `${item.submenu?.id}.${candidate.id}` === itemId);
      if (entry !== undefined && entry.disabled !== true) entry.run(api);
      return;
    }
    api.runCommand(itemId);
  };

  const fullScreen = windowState.fullScreen;
  // 面板收起时内容卸载（设备面板的定时读取随之停止）。
  const activePanel = panel.open ? registry.bottomPanels.find((item) => item.id === panel.id) : undefined;
  const start = registry.statusItems.filter((item) => item.align === 'start');
  const end = registry.statusItems.filter((item) => item.align === 'end');

  /** 切换面板的自动收缩开关并保存。 */
  const toggleAutoCollapse = (panelId: string): void => {
    const next = autoCollapse.includes(panelId) ? autoCollapse.filter((id) => id !== panelId) : [...autoCollapse, panelId];
    setAutoCollapse(next);
    bridge.shell.setLayout({ autoCollapsePanels: next });
  };

  return (
    <ShellApiContext.Provider value={api}>
      {registry.hosts.map(({ module, Host }) => createElement(Host, { key: module, bridge }))}
      <AppFrame
        titleBar={
          fullScreen ? undefined : (
            <TitleBar
              logo={<EmberLogo />}
              title={APP_NAME}
              menuBar={<MenuBar menus={menus} openMenuId={openMenuId} onOpenMenuChange={setOpenMenuId} onSelect={(_menuId, itemId) => handleMenuSelect(itemId)} />}
            />
          )
        }
        bottomPanel={
          activePanel === undefined ? undefined : (
            <section className="ember-shell-panel" aria-label={activePanel.title}>
              <div className="ember-shell-panel__header">
                <h2 className="ember-shell-panel__title">{activePanel.title}</h2>
                <div className="ember-shell-panel__actions">
                  {activePanel.autoCollapse === true ? (
                    <Button
                      variant="ghost"
                      iconOnly
                      compact
                      icon={iconLayoutBottombarCollapse}
                      pressed={autoCollapse.includes(activePanel.id)}
                      title={TEXT.autoCollapse}
                      onClick={() => toggleAutoCollapse(activePanel.id)}
                    >
                      {TEXT.autoCollapse}
                    </Button>
                  ) : null}
                  <Button variant="ghost" iconOnly compact icon={iconX} title={TEXT.closePanel} onClick={() => setPanel((current) => ({ ...current, open: false }))}>
                    {TEXT.closePanel}
                  </Button>
                </div>
              </div>
              <ScrollArea className="ember-shell-panel__body" viewportProps={{ className: 'ember-shell-panel__content' }}>
                {createElement(activePanel.Component)}
              </ScrollArea>
            </section>
          )
        }
        bottomPanelOpen={!fullScreen && bottomPanelOpen}
        defaultBottomPanelHeight={bottomHeight}
        bottomPanelMinHeight={BOTTOM_PANEL_HEIGHT}
        onBottomPanelHeightCommit={(height) => bridge.shell.setLayout({ bottomPanelHeight: Math.round(height) })}
        statusBarSize="large"
        statusBar={
          fullScreen ? undefined : (
            <StatusBar
              aria-label={TEXT.statusLabel}
              start={
                <>
                  {registry.bottomPanels.map((item) => {
                    const lamp = item.lamp?.();
                    const look = item.look?.();
                    return (
                      <StatusBarItem
                        key={item.id}
                        text={item.title}
                        icon={look?.icon ?? item.icon}
                        iconOnly
                        title={look?.hint ?? item.title}
                        {...(look?.tone === undefined ? {} : { tone: look.tone })}
                        pressed={panel.open && panel.id === item.id}
                        data-panel-button=""
                        {...(lamp === undefined ? {} : { lamp })}
                        onClick={() => api.togglePanel(item.id)}
                      />
                    );
                  })}
                  {start.map((item) => createElement(item.Component, { key: item.id }))}
                </>
              }
              end={
                <>
                  {end.map((item) => createElement(item.Component, { key: item.id }))}
                  <NotificationCenterButton />
                </>
              }
            />
          )
        }
      >
        {registry.screen === undefined ? null : <div className="ember-shell-page" ref={pageRef}>{createElement(registry.screen, { bridge })}</div>}
      </AppFrame>
      <NotificationCenter />
    </ShellApiContext.Provider>
  );
}
