// ------------------------------------------------------------------------
// 名称：shell.js
// 说明：外壳行为：命令与快捷键、标签页、边栏、右边栏与底部面板、菜单、通知、消息日志、对话框、主题。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：原型脚本，只演示交互效果；页面通过 data-command、data-open-page 等属性声明行为，由这里统一分发。
// ------------------------------------------------------------------------

(function () {
  'use strict';

  const Ember = (window.Ember = window.Ember || {});
  const SVG_NS = 'http://www.w3.org/2000/svg';

  const $ = (selector, root = document) => root.querySelector(selector);
  const $$ = (selector, root = document) => Array.from(root.querySelectorAll(selector));
  Ember.$ = $;
  Ember.$$ = $$;

  // 创建带图标引用的 svg 元素
  Ember.icon = function (id) {
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
  Ember.h = createElement;

  // ---------- 命令与快捷键 ----------

  const commands = new Map();
  Ember.commands = {
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
  Ember.keys = {
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

  Ember.notify = function (message, kind) {
    const notice = createElement('div', 'ui-notice' + (kind === 'warning' ? ' ui-notice--warning' : ''), message);
    $('#notices').append(notice);
    setTimeout(() => notice.remove(), 4000);
  };

  Ember.log = function (level, text) {
    const item = createElement('li', 'shell-message' + (level === '警告' ? ' is-warning' : ''));
    const time = createElement('time', '', new Date().toTimeString().slice(0, 8));
    item.append(time, createElement('span', 'shell-message-level', level), createElement('span', '', text));
    const list = $('#messageList');
    list.append(item);
    list.scrollTop = list.scrollHeight;
  };

  Ember.commands.register('panel.clear', () => $('#messageList').replaceChildren());

  // ---------- 对话框 ----------

  let dialogOpener = null;
  Ember.dialog = {
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
      document.dispatchEvent(new CustomEvent('dialogs-closed'));
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

  Ember.theme = {
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
    mappers: { title: 'mapper 插件', icon: 'i-plug', side: true },
    preferences: { title: '首选项', icon: 'i-sliders', side: false },
  };
  // 页面的边栏随页面切换；设备边栏不属于任何页面，只经活动栏按钮打开，不切换页面区
  const SIDES = { run: '运行', library: '卡带库', media: '设备', mappers: 'mapper 插件' };
  // 右边栏固定显示软驱里的文件：有盘片才可用，默认折叠
  const state = { tabs: [], active: null, side: null, sideCollapsed: false, panelCollapsed: false, secondaryOpen: false, secondaryAvailable: false };

  function sideOfPage(id) {
    return id && PAGES[id].side ? id : null;
  }

  function sideVisible() {
    return Boolean(state.side) && !state.sideCollapsed;
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
      select.append(Ember.icon(page.icon), createElement('span', '', page.title));
      const close = createElement('button', 'ui-icon-button');
      close.type = 'button';
      close.setAttribute('aria-label', '关闭' + page.title);
      close.dataset.tabClose = id;
      close.append(Ember.icon('i-x'));
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
      button.classList.toggle('is-active', (button.dataset.openPage || button.dataset.openSide) === state.side);
    });
    $$('.side').forEach((side) => {
      side.hidden = side.dataset.side !== state.side;
    });
    $('#sideTitle').textContent = state.side ? SIDES[state.side] : '';
    $('#sideBar').hidden = !sideVisible();
    $('[data-command="view.toggleSideBar"]').disabled = !state.side;
    $('#bottomPanel').hidden = state.panelCollapsed;
    $('#secondarySide').hidden = !secondaryVisible();
    $('[data-command="view.toggleSecondarySide"]').disabled = !state.secondaryAvailable;
  }

  function openPage(id) {
    if (!state.tabs.includes(id)) state.tabs.push(id);
    state.active = id;
    state.side = sideOfPage(id);
    render();
  }

  function closeTab(id) {
    const index = state.tabs.indexOf(id);
    state.tabs.splice(index, 1);
    if (state.active === id) {
      state.active = state.tabs[index] || state.tabs[index - 1] || null;
      if (state.side !== 'media') state.side = sideOfPage(state.active);
    }
    render();
  }

  function showSide(id) {
    state.side = id;
    state.sideCollapsed = false;
    render();
  }

  function toggleSide() {
    if (!state.side) return;
    state.sideCollapsed = !state.sideCollapsed;
    render();
  }

  function togglePanel() {
    state.panelCollapsed = !state.panelCollapsed;
    render();
  }

  function secondaryVisible() {
    return state.secondaryOpen && state.secondaryAvailable;
  }

  function toggleSecondary() {
    if (!state.secondaryAvailable) return;
    state.secondaryOpen = !state.secondaryOpen;
    render();
  }

  function setSecondaryAvailable(available) {
    state.secondaryAvailable = available;
    render();
  }

  Ember.shell = {
    state,
    sideVisible,
    sideAvailable() {
      return Boolean(state.side);
    },
    openPage,
    showSide,
    toggleSide,
    togglePanel,
    secondaryVisible,
    toggleSecondary,
    setSecondaryAvailable,
    start() {
      openPage('run');
    },
  };

  // ---------- 菜单 ----------

  const menuDefs = {};
  let openMenuId = null;

  Ember.menu = {
    register(id, buildItems) {
      menuDefs[id] = buildItems;
    },
  };

  function closeMenu() {
    openMenuId = null;
    $('#menuLayer').replaceChildren();
    $$('.shell-menubar-button').forEach((button) => button.classList.remove('is-open'));
  }

  function buildMenu(items) {
    const menu = createElement('div', 'ui-menu');
    menu.setAttribute('role', 'menu');
    items.forEach((item) => {
      if (item.separator) {
        menu.append(createElement('div', 'ui-menu-separator'));
        return;
      }
      const row = createElement('button', 'ui-menu-item');
      row.type = 'button';
      row.setAttribute('role', 'menuitem');
      row.disabled = Boolean(item.disabled);
      const mark = createElement('span', 'ui-menu-mark');
      if (item.checked) mark.append(Ember.icon('i-check'));
      const shortcut = item.arrow ? '' : Ember.keys.of(item.command) || item.shortcut || '';
      row.append(mark, createElement('span', 'ui-menu-label', item.label), createElement('span', 'ui-menu-shortcut', shortcut));
      if (item.arrow) row.lastChild.append(Ember.icon('i-chevron-right'));
      row.addEventListener('click', () => {
        closeMenu();
        if (item.command) Ember.commands.run(item.command);
        if (item.action) item.action();
      });
      menu.append(row);
    });
    return menu;
  }

  function showMenu(button) {
    closeMenu();
    openMenuId = button.dataset.menu;
    button.classList.add('is-open');
    const menu = buildMenu(menuDefs[openMenuId]());
    const rect = button.getBoundingClientRect();
    menu.style.left = rect.left + 'px';
    menu.style.top = rect.bottom + 'px';
    $('#menuLayer').append(menu);
  }

  // 右键菜单：在鼠标位置弹出，放不下时贴着窗口边缘
  Ember.menu.popup = function (x, y, items) {
    closeMenu();
    const menu = buildMenu(items);
    menu.style.left = x + 'px';
    menu.style.top = y + 'px';
    $('#menuLayer').append(menu);
    const rect = menu.getBoundingClientRect();
    if (rect.right > window.innerWidth) menu.style.left = Math.max(0, window.innerWidth - rect.width) + 'px';
    if (rect.bottom > window.innerHeight) menu.style.top = Math.max(0, window.innerHeight - rect.height) + 'px';
  };

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
      Ember.commands.run(commandElement.dataset.command, commandElement);
      return;
    }
    const pageElement = target.closest('[data-open-page]');
    if (pageElement) {
      const id = pageElement.dataset.openPage;
      if (id === state.side && pageElement.classList.contains('shell-activity-button')) toggleSide();
      else openPage(id);
      return;
    }
    const sideElement = target.closest('[data-open-side]');
    if (sideElement) {
      const id = sideElement.dataset.openSide;
      if (id === state.side && sideElement.classList.contains('shell-activity-button')) toggleSide();
      else showSide(id);
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
      Ember.dialog.closeAll();
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
      Ember.dialog.closeAll();
      return;
    }
    const commandId = bindings.get(comboOf(event));
    if (commandId) {
      event.preventDefault();
      Ember.commands.run(commandId);
    }
  });

  window.addEventListener('blur', closeMenu);
})();
