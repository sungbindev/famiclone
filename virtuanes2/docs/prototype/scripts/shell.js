// ------------------------------------------------------------------------
// 名称：shell.js
// 说明：外壳行为：命令与快捷键、标签页、边栏与底部面板、菜单、通知、消息日志、对话框、主题。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：原型脚本，只演示交互效果；页面通过 data-command、data-open-page 等属性声明行为，由这里统一分发。
// ------------------------------------------------------------------------

(function () {
  'use strict';

  const VN2 = (window.VN2 = window.VN2 || {});
  const SVG_NS = 'http://www.w3.org/2000/svg';

  const $ = (selector, root = document) => root.querySelector(selector);
  const $$ = (selector, root = document) => Array.from(root.querySelectorAll(selector));
  VN2.$ = $;
  VN2.$$ = $$;

  // 创建带图标引用的 svg 元素
  VN2.icon = function (id) {
    const svg = document.createElementNS(SVG_NS, 'svg');
    svg.setAttribute('class', 'ui-icon');
    const use = document.createElementNS(SVG_NS, 'use');
    use.setAttribute('href', '#' + id);
    svg.append(use);
    return svg;
  };

  function createElement(tag, className, text) {
    const element = document.createElement(tag);
    if (className) element.className = className;
    if (text !== undefined) element.textContent = text;
    return element;
  }
  VN2.h = createElement;

  // ---------- 命令与快捷键 ----------

  const commands = new Map();
  VN2.commands = {
    register(id, handler) {
      commands.set(id, handler);
    },
    run(id, source) {
      const handler = commands.get(id);
      if (handler) handler(source);
    },
  };

  const bindings = new Map();
  const shortcutOf = new Map();
  VN2.keys = {
    bind(combo, commandId) {
      bindings.set(combo, commandId);
      shortcutOf.set(commandId, combo);
    },
    of(commandId) {
      return shortcutOf.get(commandId) || '';
    },
  };

  function comboOf(event) {
    const parts = [];
    if (event.ctrlKey || event.metaKey) parts.push('Ctrl');
    if (event.altKey) parts.push('Alt');
    if (event.shiftKey) parts.push('Shift');
    parts.push(event.key.length === 1 ? event.key.toUpperCase() : event.key);
    return parts.join('+');
  }

  // ---------- 通知与消息 ----------

  VN2.notify = function (message, kind) {
    const notice = createElement('div', 'ui-notice' + (kind === 'warning' ? ' ui-notice--warning' : ''), message);
    $('#notices').append(notice);
    setTimeout(() => notice.remove(), 4000);
  };

  VN2.log = function (level, text) {
    const item = createElement('li', 'shell-message' + (level === '警告' ? ' is-warning' : ''));
    const time = createElement('time', '', new Date().toTimeString().slice(0, 8));
    item.append(time, createElement('span', 'shell-message-level', level), createElement('span', '', text));
    const list = $('#messageList');
    list.append(item);
    list.scrollTop = list.scrollHeight;
  };

  VN2.commands.register('panel.clear', () => $('#messageList').replaceChildren());

  // ---------- 对话框 ----------

  let dialogOpener = null;
  VN2.dialog = {
    open(id) {
      const dialog = document.getElementById(id);
      dialogOpener = document.activeElement;
      dialog.hidden = false;
      $('[data-close-dialog]', dialog).focus();
    },
    closeAll() {
      $$('.ui-dialog-scrim').forEach((dialog) => {
        dialog.hidden = true;
      });
      if (dialogOpener) dialogOpener.focus();
    },
  };

  // ---------- 主题 ----------

  const darkQuery = window.matchMedia('(prefers-color-scheme: dark)');
  let themeMode = 'dark';

  function applyTheme() {
    const resolved = themeMode === 'system' ? (darkQuery.matches ? 'dark' : 'light') : themeMode;
    document.documentElement.classList.remove('theme-dark', 'theme-light');
    document.documentElement.classList.add('theme-' + resolved);
  }

  VN2.theme = {
    set(mode) {
      themeMode = mode;
      applyTheme();
    },
  };
  darkQuery.addEventListener('change', () => {
    if (themeMode === 'system') applyTheme();
  });

  // ---------- 标签页、边栏、底部面板 ----------

  const PAGES = {
    run: { title: '运行', icon: 'i-play', side: true },
    library: { title: '卡带库', icon: 'i-library', side: true },
    media: { title: '设备', icon: 'i-drive', side: true },
    mappers: { title: 'mapper 插件', icon: 'i-plug', side: true },
    preferences: { title: '首选项', icon: 'i-sliders', side: false },
  };
  const state = { tabs: [], active: null, sideCollapsed: false, panelCollapsed: false };

  function hasSide(id) {
    return Boolean(id && PAGES[id].side);
  }

  function sideVisible() {
    return hasSide(state.active) && !state.sideCollapsed;
  }

  function renderTabs() {
    const bar = $('#tabBar');
    bar.replaceChildren();
    state.tabs.forEach((id) => {
      const page = PAGES[id];
      const active = id === state.active;
      const tab = createElement('div', 'shell-tab' + (active ? ' is-active' : ''));
      const select = createElement('button', 'shell-tab-select');
      select.type = 'button';
      select.setAttribute('role', 'tab');
      select.setAttribute('aria-selected', String(active));
      select.dataset.tabSelect = id;
      select.append(VN2.icon(page.icon), createElement('span', '', page.title));
      const close = createElement('button', 'ui-icon-button');
      close.type = 'button';
      close.setAttribute('aria-label', '关闭' + page.title);
      close.dataset.tabClose = id;
      close.append(VN2.icon('i-x'));
      tab.append(select, close);
      bar.append(tab);
    });
  }

  function render() {
    renderTabs();
    $$('.page').forEach((page) => {
      page.hidden = page.dataset.page !== state.active;
    });
    $('#emptyWorkspace').hidden = Boolean(state.active);
    $$('.shell-activity-button').forEach((button) => {
      button.classList.toggle('is-active', button.dataset.openPage === state.active);
    });
    $$('.side').forEach((side) => {
      side.hidden = side.dataset.side !== state.active;
    });
    $('#sideTitle').textContent = state.active ? PAGES[state.active].title : '';
    $('#sideBar').hidden = !sideVisible();
    $('[data-command="view.toggleSideBar"]').disabled = !hasSide(state.active);
    $('#bottomPanel').hidden = state.panelCollapsed;
  }

  function openPage(id) {
    if (!state.tabs.includes(id)) state.tabs.push(id);
    state.active = id;
    render();
  }

  function closeTab(id) {
    const index = state.tabs.indexOf(id);
    state.tabs.splice(index, 1);
    if (state.active === id) state.active = state.tabs[index] || state.tabs[index - 1] || null;
    render();
  }

  function toggleSide() {
    if (!hasSide(state.active)) return;
    state.sideCollapsed = !state.sideCollapsed;
    render();
  }

  function togglePanel() {
    state.panelCollapsed = !state.panelCollapsed;
    render();
  }

  VN2.shell = {
    state,
    sideVisible,
    sideAvailable() {
      return hasSide(state.active);
    },
    openPage,
    toggleSide,
    togglePanel,
    start() {
      openPage('run');
    },
  };

  // ---------- 菜单 ----------

  const menuDefs = {};
  let openMenuId = null;

  VN2.menu = {
    register(id, buildItems) {
      menuDefs[id] = buildItems;
    },
  };

  function closeMenu() {
    openMenuId = null;
    $('#menuLayer').replaceChildren();
    $$('.shell-menubar-button').forEach((button) => button.classList.remove('is-open'));
  }

  function showMenu(button) {
    closeMenu();
    openMenuId = button.dataset.menu;
    button.classList.add('is-open');
    const menu = createElement('div', 'ui-menu');
    menu.setAttribute('role', 'menu');
    menuDefs[openMenuId]().forEach((item) => {
      if (item.separator) {
        menu.append(createElement('div', 'ui-menu-separator'));
        return;
      }
      const row = createElement('button', 'ui-menu-item');
      row.type = 'button';
      row.setAttribute('role', 'menuitem');
      row.disabled = Boolean(item.disabled);
      const mark = createElement('span', 'ui-menu-mark');
      if (item.checked) mark.append(VN2.icon('i-check'));
      const shortcut = item.arrow ? '' : VN2.keys.of(item.command) || item.shortcut || '';
      row.append(mark, createElement('span', 'ui-menu-label', item.label), createElement('span', 'ui-menu-shortcut', shortcut));
      if (item.arrow) row.lastChild.append(VN2.icon('i-chevron-right'));
      row.addEventListener('click', () => {
        closeMenu();
        if (item.command) VN2.commands.run(item.command);
      });
      menu.append(row);
    });
    const rect = button.getBoundingClientRect();
    menu.style.left = rect.left + 'px';
    menu.style.top = rect.bottom + 'px';
    $('#menuLayer').append(menu);
  }

  $$('.shell-menubar-button').forEach((button) => {
    button.addEventListener('click', () => {
      if (openMenuId === button.dataset.menu) closeMenu();
      else showMenu(button);
    });
    button.addEventListener('mouseenter', () => {
      if (openMenuId && openMenuId !== button.dataset.menu) showMenu(button);
    });
  });

  // ---------- 全局事件 ----------

  document.addEventListener('click', (event) => {
    const target = event.target;
    if (!target.closest('.ui-menu') && !target.closest('.shell-menubar-button')) closeMenu();

    const commandElement = target.closest('[data-command]');
    if (commandElement && !commandElement.disabled) {
      VN2.commands.run(commandElement.dataset.command, commandElement);
      return;
    }
    const pageElement = target.closest('[data-open-page]');
    if (pageElement) {
      const id = pageElement.dataset.openPage;
      if (id === state.active && hasSide(id) && pageElement.classList.contains('shell-activity-button')) toggleSide();
      else openPage(id);
      return;
    }
    const closeElement = target.closest('[data-tab-close]');
    if (closeElement) {
      closeTab(closeElement.dataset.tabClose);
      return;
    }
    const selectElement = target.closest('[data-tab-select]');
    if (selectElement) {
      openPage(selectElement.dataset.tabSelect);
      return;
    }
    if (target.closest('[data-close-dialog]')) {
      VN2.dialog.closeAll();
      return;
    }
    const segment = target.closest('.ui-segmented button');
    if (segment) {
      const group = segment.parentElement;
      $$('button', group).forEach((button) => button.setAttribute('aria-checked', String(button === segment)));
      document.dispatchEvent(new CustomEvent('segmented-change', { detail: { name: group.dataset.segmented, value: segment.dataset.value } }));
    }
  });

  document.addEventListener('keydown', (event) => {
    if (event.key === 'Escape') {
      closeMenu();
      VN2.dialog.closeAll();
      return;
    }
    const commandId = bindings.get(comboOf(event));
    if (commandId) {
      event.preventDefault();
      VN2.commands.run(commandId);
    }
  });

  window.addEventListener('blur', closeMenu);
})();
