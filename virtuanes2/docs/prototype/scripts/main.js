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

  const VN2 = window.VN2;
  const commands = VN2.commands;
  const machine = VN2.machine;

  // ---------- 命令 ----------

  commands.register('file.open', () => VN2.notify('原型：这里会打开系统的文件选择窗口，选择 NES 文件。'));
  commands.register('file.close', () => machine.eject());
  commands.register('app.exit', () => VN2.notify('原型：这里会关闭程序。'));
  commands.register('machine.powerToggle', () => machine.powerToggle());
  commands.register('machine.pauseToggle', () => machine.pauseToggle());
  commands.register('machine.softReset', () => machine.softReset());
  commands.register('machine.hardReset', () => machine.hardReset());
  commands.register('machine.muteToggle', () => machine.muteToggle());
  commands.register('machine.screenshot', () => machine.screenshot());
  commands.register('view.toggleSideBar', () => VN2.shell.toggleSide());
  commands.register('view.togglePanel', () => VN2.shell.togglePanel());
  commands.register('view.fullscreen', () => {
    if (document.fullscreenElement) document.exitFullscreen();
    else document.documentElement.requestFullscreen();
  });
  ['run', 'library', 'media', 'mappers'].forEach((id) => {
    commands.register('view.page.' + id, () => VN2.shell.openPage(id));
  });
  commands.register('settings.open', () => VN2.shell.openPage('preferences'));
  commands.register('settings.debug', () => VN2.pages.openPreferences('debug'));
  commands.register('help.logs', () => VN2.notify('原型：这里会打开日志所在的文件夹。'));
  commands.register('help.about', () => VN2.dialog.open('aboutDialog'));

  // ---------- 快捷键 ----------

  VN2.keys.bind('Ctrl+O', 'file.open');
  VN2.keys.bind('F9', 'machine.powerToggle');
  VN2.keys.bind('F8', 'machine.pauseToggle');
  VN2.keys.bind('F7', 'machine.softReset');
  VN2.keys.bind('Ctrl+F7', 'machine.hardReset');
  VN2.keys.bind('Ctrl+Shift+S', 'machine.screenshot');
  VN2.keys.bind('Ctrl+B', 'view.toggleSideBar');
  VN2.keys.bind('Ctrl+J', 'view.togglePanel');
  VN2.keys.bind('F11', 'view.fullscreen');
  VN2.keys.bind('Ctrl+,', 'settings.open');

  // ---------- 菜单 ----------

  const SEPARATOR = { separator: true };

  VN2.menu.register('file', () => [
    { label: '打开卡带…', command: 'file.open' },
    { label: '最近打开', arrow: true },
    { label: '关闭卡带', command: 'file.close', disabled: !machine.state.cartridge },
    SEPARATOR,
    { label: '退出', command: 'app.exit', shortcut: 'Alt+F4' },
  ]);

  VN2.menu.register('machine', () => {
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

  VN2.menu.register('view', () => [
    { label: '运行', command: 'view.page.run' },
    { label: '卡带库', command: 'view.page.library' },
    { label: '设备', command: 'view.page.media' },
    { label: 'mapper 插件', command: 'view.page.mappers' },
    SEPARATOR,
    { label: '边栏', command: 'view.toggleSideBar', checked: VN2.shell.sideVisible(), disabled: !VN2.shell.sideAvailable() },
    { label: '底部面板', command: 'view.togglePanel', checked: !VN2.shell.state.panelCollapsed },
    SEPARATOR,
    { label: '全屏', command: 'view.fullscreen' },
  ]);

  VN2.menu.register('settings', () => [
    { label: '首选项', command: 'settings.open' },
    { label: 'mapper 插件', command: 'view.page.mappers' },
  ]);

  VN2.menu.register('help', () => [
    { label: '打开日志目录', command: 'help.logs' },
    SEPARATOR,
    { label: '关于 VirtuaNES2', command: 'help.about' },
  ]);

  // ---------- 启动 ----------

  machine.refresh();
  VN2.shell.start();
})();
