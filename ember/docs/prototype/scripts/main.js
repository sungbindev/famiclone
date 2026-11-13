// ------------------------------------------------------------------------
// 名称：main.js
// 说明：原型入口：登记命令、快捷键与菜单，打开初始页面。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：原型脚本；快捷键与菜单是拟定的默认值，正式实现按如见的命令登记方式重做。
// ------------------------------------------------------------------------

(function () {
  'use strict';

  const Ember = window.Ember;
  const commands = Ember.commands;
  const machine = Ember.machine;

  // ---------- 命令 ----------

  commands.register('file.open', () => Ember.notify('原型：这里会打开系统的文件选择窗口，选择 NES 文件。'));
  commands.register('file.close', () => machine.eject());
  commands.register('app.exit', () => Ember.notify('原型：这里会关闭程序。'));
  commands.register('machine.powerToggle', () => machine.powerToggle());
  commands.register('machine.pauseToggle', () => machine.pauseToggle());
  commands.register('machine.softReset', () => machine.softReset());
  commands.register('machine.hardReset', () => machine.hardReset());
  commands.register('machine.muteToggle', () => machine.muteToggle());
  commands.register('machine.screenshot', () => machine.screenshot());
  commands.register('view.toggleSideBar', () => Ember.shell.toggleSide());
  commands.register('view.togglePanel', () => Ember.shell.togglePanel());
  commands.register('view.toggleSecondarySide', () => Ember.shell.toggleSecondary());
  commands.register('view.fullscreen', () => {
    if (document.fullscreenElement) document.exitFullscreen();
    else document.documentElement.requestFullscreen();
  });
  ['run', 'library', 'mappers'].forEach((id) => {
    commands.register('view.page.' + id, () => Ember.shell.openPage(id));
  });
  commands.register('view.side.media', () => Ember.shell.showSide('media'));
  commands.register('settings.open', () => Ember.shell.openPage('preferences'));
  commands.register('settings.debug', () => Ember.pages.openPreferences('debug'));
  commands.register('help.logs', () => Ember.notify('原型：这里会打开日志所在的文件夹。'));
  commands.register('help.about', () => Ember.dialog.open('aboutDialog'));

  // ---------- 快捷键 ----------

  Ember.keys.bind('Ctrl+O', 'file.open');
  Ember.keys.bind('F9', 'machine.powerToggle');
  Ember.keys.bind('F8', 'machine.pauseToggle');
  Ember.keys.bind('F7', 'machine.softReset');
  Ember.keys.bind('Ctrl+F7', 'machine.hardReset');
  Ember.keys.bind('Ctrl+Shift+S', 'machine.screenshot');
  Ember.keys.bind('Ctrl+B', 'view.toggleSideBar');
  Ember.keys.bind('Ctrl+J', 'view.togglePanel');
  Ember.keys.bind('Ctrl+Alt+B', 'view.toggleSecondarySide');
  Ember.keys.bind('F11', 'view.fullscreen');
  Ember.keys.bind('Ctrl+,', 'settings.open');

  // ---------- 菜单 ----------

  const SEPARATOR = { separator: true };

  Ember.menu.register('file', () => [
    { label: '打开卡带…', command: 'file.open' },
    { label: '最近打开', arrow: true },
    { label: '关闭卡带', command: 'file.close', disabled: !machine.state.cartridge },
    SEPARATOR,
    { label: '退出', command: 'app.exit', shortcut: 'Alt+F4' },
  ]);

  Ember.menu.register('machine', () => {
    const state = machine.state;
    const empty = !state.cartridge;
    return [
      { label: state.power ? '断电' : '上电', command: 'machine.powerToggle', disabled: empty },
      { label: state.paused ? '继续' : '暂停', command: 'machine.pauseToggle', disabled: empty || !state.power },
      SEPARATOR,
      { label: '软复位', command: 'machine.softReset', disabled: empty || !state.power },
      { label: '硬复位', command: 'machine.hardReset', disabled: empty },
      SEPARATOR,
      { label: '静音', command: 'machine.muteToggle', checked: state.muted },
      { label: '截图', command: 'machine.screenshot' },
    ];
  });

  Ember.menu.register('view', () => [
    { label: '运行', command: 'view.page.run' },
    { label: '卡带库', command: 'view.page.library' },
    { label: '设备', command: 'view.side.media' },
    { label: 'mapper 插件', command: 'view.page.mappers' },
    SEPARATOR,
    { label: '边栏', command: 'view.toggleSideBar', checked: Ember.shell.sideVisible(), disabled: !Ember.shell.sideAvailable() },
    { label: '底部面板', command: 'view.togglePanel', checked: !Ember.shell.state.panelCollapsed },
    { label: '右边栏', command: 'view.toggleSecondarySide', checked: Ember.shell.secondaryVisible(), disabled: !Ember.shell.state.secondaryAvailable },
    SEPARATOR,
    { label: '全屏', command: 'view.fullscreen' },
  ]);

  Ember.menu.register('settings', () => [
    { label: '首选项', command: 'settings.open' },
    { label: 'mapper 插件', command: 'view.page.mappers' },
  ]);

  Ember.menu.register('help', () => [
    { label: '打开日志目录', command: 'help.logs' },
    SEPARATOR,
    { label: '关于 Ember', command: 'help.about' },
  ]);

  // ---------- 启动 ----------

  machine.refresh();
  Ember.shell.start();
})();
