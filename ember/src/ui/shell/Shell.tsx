/*
 * ------------------------------------------------------------------------
 * 名称：Shell.tsx
 * 说明：外壳：用组件库的外壳组件拼出窗口框架（标题栏与菜单、活动栏、边栏、标签栏与页面区、底部面板、右边栏、状态栏、通知），
 *       全部入口都来自登记表（页面、命令与快捷键、菜单项、活动栏按钮、底部面板页签、状态项），自己不写死业务入口；
 *       处理页面与标签、边栏随页面切换、命令与全局快捷键（焦点在文本输入控件里时不带 Ctrl 或 Alt 的快捷键不触发）、子菜单、模块的常驻组件、命令状态的刷新、窗口状态与底部面板高度的保存；全屏时只显示页面区。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：规则见 docs/05-界面.md 第 2、3、5、6、9 节；未激活的页面保持挂载、只隐藏，切回时状态不丢；
 *       页面区里每个页面在组件库的 DialogScope 里（页面内边距与以后的对话框范围）；
 *       画面获得焦点时键盘事件先交给画面（它处理过的按键 defaultPrevented，外壳不再响应）；
 *       各面板每次启动都是关闭的、展开状态不保存；面板展开与折叠时窗口向外伸缩、页面区不变，见 use-window-extents.ts。
 * ------------------------------------------------------------------------
 */

import {
  ActivityBar,
  AppFrame,
  DialogScope,
  DockPanel,
  EmptyWorkspace,
  iconLayoutBottombar,
  iconLayoutSidebar,
  iconLayoutSidebarRight,
  MenuBar,
  NotificationCenter,
  NotificationCenterButton,
  shortcutFromEvent,
  SideBar,
  State,
  StatusBar,
  TabBar,
  TitleBar,
  type MenuBarMenu,
  type MenuItem,
  type TitleBarTool
} from '@rujianjs/ui-kit';
import { createElement, useEffect, useLayoutEffect, useMemo, useRef, useState } from 'react';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import type { WindowState } from '../../contracts/shell/window-control';
import { buildShellRegistry, type ShellRegistry } from './build-registry';
import { isTextEditingTarget } from './editable-target';
import { EmberLogo } from './EmberLogo';
import { MessageLog } from './message-log';
import { MessageLogContext, ShellApiContext } from './shell-context';
import { MESSAGES_PANEL_ID, SHELL_COMMANDS, SHELL_MODULE } from './shell-module';
import { commandTitle, SHELL_MENUS, type ShellApi, type ShellMessageInput, type ShellModule, type ShellViewState } from './shell-registry';
import { activateTab, closeTab, EMPTY_TABS, openTab, type TabState } from './tab-state';
import { useWindowExtents } from './use-window-extents';
import './shell.css';

/** 应用名。 */
const APP_NAME = 'EMBER';

/** 边栏的默认宽度（像素），宽度不保存。 */
const SIDE_BAR_WIDTH = 240;
/** 底部面板的默认高度（像素），保存过高度时用保存的。 */
const BOTTOM_PANEL_HEIGHT = 160;
/** 右边栏的默认宽度（像素），宽度不保存。 */
const RIGHT_BAR_WIDTH = 340;
/**
 * 右边栏现在能不能打开：右边栏只显示软驱里的文件，软驱与盘片由学习机阶段的 media 模块提供，
 * 在那之前没有盘片，右边栏不能打开（折叠按钮与菜单项置灰）。
 */
const RIGHT_BAR_AVAILABLE = false;
/** 右边栏里软盘文件的页签标识。 */
const FLOPPY_PANEL_ID = 'shell.floppyFiles';

/** 用户可见的文字。 */
const TEXT = {
  sideBar: '切换边栏',
  bottomPanel: '切换底部面板',
  rightBar: '切换右边栏',
  activityLabel: '活动栏',
  tabsLabel: '已打开的页面',
  statusLabel: '状态栏',
  bottomPanelLabel: '底部面板',
  rightBarLabel: '右边栏',
  floppyFiles: '软盘文件',
  noDisk: '没有盘片',
  emptyHint: '没有打开的页面。点击活动栏的按钮，或在“视图”菜单里打开页面。',
  pageMissing: (id: string) => `页面不存在：${id}`,
  registryFailed: '外壳登记有误，界面无法显示，详情见开发者工具的控制台'
} as const;

/** 标题栏折叠按钮的标识。 */
const TOOLS = { side: 'side', bottom: 'bottom', right: 'right' } as const;
/** 折叠按钮对应的命令。 */
const TOOL_COMMANDS: Readonly<Record<string, string>> = {
  [TOOLS.side]: SHELL_COMMANDS.toggleSideBar,
  [TOOLS.bottom]: SHELL_COMMANDS.toggleBottomPanel,
  [TOOLS.right]: SHELL_COMMANDS.toggleRightBar
};

/** 外壳的属性。 */
export interface ShellProps {
  /** 预加载脚本暴露的接口。 */
  readonly bridge: EmberBridge;
  /** 渲染进程的模块清单里各模块的登记项。 */
  readonly modules: readonly ShellModule[];
}

/** 各面板是否展开：每次启动都是全部关闭。 */
interface OpenPanels {
  readonly side: boolean;
  readonly bottom: boolean;
  readonly right: boolean;
}

/** 全部面板关闭。 */
const ALL_CLOSED: OpenPanels = { side: false, bottom: false, right: false };

/** 外壳实时读取的状态：事件处理函数用它取最新值，不必随渲染重新创建。 */
interface LiveState {
  readonly tabs: TabState;
  readonly view: ShellViewState;
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

/** 登记表有效之后的外壳。 */
function ShellFrame({ bridge, registry }: { bridge: EmberBridge; registry: ShellRegistry }) {
  const [panels, setPanels] = useState<OpenPanels>(ALL_CLOSED);
  const [sideWidth, setSideWidth] = useState(SIDE_BAR_WIDTH);
  const [rightWidth, setRightWidth] = useState(RIGHT_BAR_WIDTH);
  // 底部面板的高度：读到保存的高度之前为 null，此时不显示界面。
  const [bottomHeight, setBottomHeight] = useState<number | null>(null);
  const [tabs, setTabs] = useState<TabState>(() =>
    registry.startPages.reduce((state, id) => {
      const page = registry.pages.get(id);
      return page === undefined ? state : openTab(state, page);
    }, EMPTY_TABS)
  );
  const [windowState, setWindowState] = useState<WindowState>({ maximized: false, fullScreen: false });
  const [bottomTab, setBottomTab] = useState<string>(MESSAGES_PANEL_ID);
  const [openMenuId, setOpenMenuId] = useState<string | null>(null);
  // 模块通过 refreshCommands 通知命令状态变了：递增一次让菜单重新计算。
  const [, setCommandVersion] = useState(0);
  const [log] = useState(() => new MessageLog());

  useEffect(() => {
    let active = true;
    void bridge.shell.getState().then((state) => {
      if (active) setBottomHeight(state.layout.bottomPanelHeight ?? BOTTOM_PANEL_HEIGHT);
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

  const activeTab = tabs.tabs.find((tab) => tab.id === tabs.activeId);
  const activePage = activeTab === undefined ? undefined : registry.pages.get(activeTab.pageId);
  const hasSideBar = activePage?.sideBar !== undefined;
  const sideBarOpen = hasSideBar && panels.side;
  const bottomPanelOpen = panels.bottom;
  const rightBarOpen = RIGHT_BAR_AVAILABLE && panels.right;
  const view: ShellViewState = { sideBarOpen, hasSideBar, bottomPanelOpen, rightBarOpen, canOpenRightBar: RIGHT_BAR_AVAILABLE, fullScreen: windowState.fullScreen };
  const live = useRef<LiveState>({ tabs, view });
  useLayoutEffect(() => {
    live.current = { tabs, view };
  });
  const { pagesRef, pinnedStyle, onPanelExtentsChange } = useWindowExtents(bridge, windowState.fullScreen);

  const api = useMemo<ShellApi>(() => {
    const shell: ShellApi = {
      openPage: (pageId) => {
        const page = registry.pages.get(pageId);
        if (page === undefined) {
          log.append({ level: 'error', text: TEXT.pageMissing(pageId) });
          return;
        }
        setTabs((state) => openTab(state, page));
      },
      closeActiveTab: () => setTabs((state) => (state.activeId === null ? state : closeTab(state, state.activeId))),
      toggleSideBar: () => {
        if (live.current.view.hasSideBar) setPanels((current) => ({ ...current, side: !current.side }));
      },
      toggleBottomPanel: () => setPanels((current) => ({ ...current, bottom: !current.bottom })),
      toggleRightBar: () => {
        if (RIGHT_BAR_AVAILABLE) setPanels((current) => ({ ...current, right: !current.right }));
      },
      toggleFullScreen: () => bridge.shell.windowAction('toggleFullScreen'),
      quit: () => bridge.shell.windowAction('quit'),
      showBottomPanel: (panelId) => {
        setBottomTab(panelId);
        setPanels((current) => (current.bottom ? current : { ...current, bottom: true }));
      },
      addMessage: (message: ShellMessageInput) => log.append(message),
      runCommand: (commandId) => {
        const command = registry.commands.get(commandId);
        if (command === undefined || command.enabled?.(live.current.view) === false) return;
        command.run(shell);
      },
      refreshCommands: () => setCommandVersion((version) => version + 1)
    };
    return shell;
  }, [bridge, registry, log]);

  // 全局快捷键与 Alt+助记字母；画面等已经处理过的按键（defaultPrevented）外壳不再响应。
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

  const shortcutOf = (commandId: string): string => registry.commands.get(commandId)?.defaultShortcut ?? '';
  const toolTitle = (label: string, commandId: string): string => `${label} ${shortcutOf(commandId)}`.trim();
  const tools: TitleBarTool[] = [
    { id: TOOLS.side, label: TEXT.sideBar, title: toolTitle(TEXT.sideBar, SHELL_COMMANDS.toggleSideBar), icon: iconLayoutSidebar, expanded: sideBarOpen, ...(hasSideBar ? {} : { disabled: true }) },
    { id: TOOLS.bottom, label: TEXT.bottomPanel, title: toolTitle(TEXT.bottomPanel, SHELL_COMMANDS.toggleBottomPanel), icon: iconLayoutBottombar, expanded: bottomPanelOpen },
    {
      id: TOOLS.right,
      label: TEXT.rightBar,
      title: toolTitle(TEXT.rightBar, SHELL_COMMANDS.toggleRightBar),
      icon: iconLayoutSidebarRight,
      expanded: rightBarOpen,
      ...(RIGHT_BAR_AVAILABLE ? {} : { disabled: true })
    }
  ];

  const handleActivitySelect = (activityId: string): void => {
    const item = registry.activityItems.find((entry) => entry.id === activityId);
    if (item === undefined) return;
    // 再点选中的按钮折叠或展开边栏；点别的按钮切到它的页面，页面有边栏而边栏折叠着时展开。
    if (item.page === activePage?.id) {
      api.toggleSideBar();
      return;
    }
    api.openPage(item.page);
    if (registry.pages.get(item.page)?.sideBar !== undefined && !panels.side) setPanels((current) => ({ ...current, side: true }));
  };

  const fullScreen = windowState.fullScreen;
  const start = registry.statusItems.filter((item) => item.align === 'start');
  const end = registry.statusItems.filter((item) => item.align === 'end');
  const activeActivityId = registry.activityItems.find((item) => item.page === activePage?.id)?.id;

  return (
    <ShellApiContext.Provider value={api}>
      <MessageLogContext.Provider value={log}>
        {registry.hosts.map(({ module, Host }) => createElement(Host, { key: module, bridge }))}
        <AppFrame
          titleBar={
            fullScreen ? undefined : (
              <TitleBar
                logo={<EmberLogo brand />}
                title={APP_NAME}
                menuBar={<MenuBar menus={menus} openMenuId={openMenuId} onOpenMenuChange={setOpenMenuId} onSelect={(_menuId, itemId) => handleMenuSelect(itemId)} />}
                tools={tools}
                onToolClick={(id) => api.runCommand(TOOL_COMMANDS[id] ?? '')}
              />
            )
          }
          activityBar={
            fullScreen ? undefined : (
              <ActivityBar
                aria-label={TEXT.activityLabel}
                items={registry.activityItems.map((item) => ({ id: item.id, label: item.label, icon: item.icon, ...(item.separatorBefore ? { separatorBefore: true } : {}) }))}
                {...(activeActivityId === undefined ? {} : { activeId: activeActivityId })}
                {...(hasSideBar ? { sideBarExpanded: sideBarOpen } : {})}
                onSelect={handleActivitySelect}
              />
            )
          }
          sideBar={tabs.tabs.map((tab) => {
            const sideBar = registry.pages.get(tab.pageId)?.sideBar;
            if (sideBar === undefined) return null;
            const title = sideBar.title ?? registry.pages.get(tab.pageId)?.title;
            return (
              <div key={tab.id} className="ember-shell-side" hidden={tab.id !== tabs.activeId}>
                <SideBar {...(title === undefined ? {} : { title })}>{createElement(sideBar.Component, { bridge })}</SideBar>
              </div>
            );
          })}
          sideBarCollapsed={fullScreen || !sideBarOpen}
          sideBarWidth={sideWidth}
          onSideBarWidthChange={(width) => setSideWidth(Math.round(width))}
          tabBar={
            fullScreen ? undefined : (
              <TabBar
                aria-label={TEXT.tabsLabel}
                tabs={tabs.tabs.map((tab) => {
                  const page = registry.pages.get(tab.pageId);
                  return { id: tab.id, title: page?.title ?? tab.pageId, ...(page ? { icon: page.icon } : {}) };
                })}
                {...(tabs.activeId === null ? {} : { activeId: tabs.activeId })}
                onSelect={(id) => setTabs((state) => activateTab(state, id))}
                onClose={(id) => setTabs((state) => closeTab(state, id))}
              />
            )
          }
          bottomPanel={
            <DockPanel
              aria-label={TEXT.bottomPanelLabel}
              tabs={registry.bottomPanels.map((panel) => ({
                id: panel.id,
                label: panel.title,
                ...(panel.Actions ? { actions: createElement(panel.Actions) } : {}),
                content: createElement(panel.Component)
              }))}
              activeTabId={bottomTab}
              onTabChange={setBottomTab}
              onCollapse={() => setPanels((current) => ({ ...current, bottom: false }))}
            />
          }
          bottomPanelOpen={!fullScreen && bottomPanelOpen}
          bottomPanelHeight={bottomHeight}
          onBottomPanelHeightChange={(height) => setBottomHeight(Math.round(height))}
          onBottomPanelHeightCommit={(height) => bridge.shell.setLayout({ bottomPanelHeight: Math.round(height) })}
          rightBar={
            <DockPanel
              aria-label={TEXT.rightBarLabel}
              tabs={[{ id: FLOPPY_PANEL_ID, label: TEXT.floppyFiles, content: <State text={TEXT.noDisk} /> }]}
              onCollapse={() => setPanels((current) => ({ ...current, right: false }))}
            />
          }
          rightBarOpen={!fullScreen && rightBarOpen}
          rightBarWidth={rightWidth}
          onRightBarWidthChange={(width) => setRightWidth(Math.round(width))}
          limitPanelsToViewport={false}
          onPanelExtentsChange={onPanelExtentsChange}
          statusBar={
            fullScreen ? undefined : (
              <StatusBar
                aria-label={TEXT.statusLabel}
                start={start.map((item) => createElement(item.Component, { key: item.id }))}
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
          <div ref={pagesRef} className="ember-shell-pages" style={pinnedStyle}>
            {tabs.tabs.length === 0 ? <EmptyWorkspace logo={<EmberLogo />} subtitle={TEXT.emptyHint} /> : null}
            {tabs.tabs.map((tab) => {
              const page = registry.pages.get(tab.pageId);
              return (
                <DialogScope key={tab.id} hidden={tab.id !== tabs.activeId}>
                  <div className="ember-shell-page">{page === undefined ? null : createElement(page.Component, { bridge })}</div>
                </DialogScope>
              );
            })}
          </div>
        </AppFrame>
        <NotificationCenter />
      </MessageLogContext.Provider>
    </ShellApiContext.Provider>
  );
}
