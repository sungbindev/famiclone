// ------------------------------------------------------------------------
// 名称：pages.js
// 说明：各页面与设备边栏的交互：卡带库、设备（软驱、磁带与打印机）、mapper 插件、首选项。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：原型脚本；数据都是示意，没有真实读写文件。
// ------------------------------------------------------------------------

(function () {
  'use strict';

  const Ember = window.Ember;
  const { $, $$, h } = Ember;

  function selectRow(table, row) {
    $$('tr[data-row]', table).forEach((item) => item.classList.toggle('is-selected', item === row));
  }

  function filterRows(table, text, accept) {
    const keyword = text.trim().toLowerCase();
    $$('tbody tr[data-row]', table).forEach((row) => {
      row.hidden = !((!keyword || row.textContent.toLowerCase().includes(keyword)) && accept(row));
    });
  }

  // ---------- 卡带库 ----------

  const libraryTable = $('#libTable');

  function readCartridge(row) {
    const cells = Array.from(row.cells).map((cell) => cell.textContent.trim());
    return { name: cells[0], mapper: cells[1], variant: cells[2], host: cells[3], plugin: cells[4] };
  }

  function filterLibrary() {
    const filter = $('#libFilter').value;
    filterRows(libraryTable, $('#libSearch').value, (row) => filter === 'all' || row.dataset.status === filter);
  }

  // mapper 171 是学习机的 BIOS，不是卡带：没有插件时只提示去“开机…”
  function missingHint(info) {
    return info.mapper === '171' ? '学习机 BIOS 不是卡带，请在“文件”菜单的“开机…”里选机型开机。' : '在“mapper 插件”页安装后即可运行。';
  }

  libraryTable.addEventListener('click', (event) => {
    const row = event.target.closest('tr[data-row]');
    if (!row) return;
    selectRow(libraryTable, row);
    const callout = $('#libMissing');
    callout.hidden = row.dataset.status !== 'missing';
    if (!callout.hidden) {
      const info = readCartridge(row);
      $('span', callout).textContent = '选中的文件缺少 mapper ' + info.mapper + '、变体 ' + info.variant + ' 的插件。' + missingHint(info);
    }
  });

  libraryTable.addEventListener('dblclick', (event) => {
    const row = event.target.closest('tr[data-row]');
    if (!row) return;
    const info = readCartridge(row);
    if (row.dataset.status === 'missing') {
      Ember.notify('无法运行 ' + info.name + '：缺少 mapper ' + info.mapper + '、变体 ' + info.variant + ' 的插件。' + (info.mapper === '171' ? missingHint(info) : ''), 'warning');
      return;
    }
    Ember.machine.load(info);
  });

  $('#libSearch').addEventListener('input', filterLibrary);
  $('#libFilter').addEventListener('change', filterLibrary);

  // ---------- mapper 插件 ----------

  const mapperTable = $('#mapTable');
  const MAPPER_DETAILS = {
    nrom: {
      title: 'NROM',
      facts: [['标识', 'nrom'], ['版本', '1.0.0'], ['发布者', 'Ember'], ['匹配', 'mapper 0'], ['主机型号', 'fc'], ['存储区', 'prg（ROM，来自文件）、chr（ROM；文件没有 CHR 时用 8K RAM）'], ['寄存器范围', '无'], ['状态声明', '无']],
    },
    demo: {
      title: '示意 mapper',
      facts: [['标识', 'demo-bank'], ['版本', '0.1.0'], ['发布者', '示例发布者'], ['匹配', 'mapper 999'], ['主机型号', 'fc'], ['存储区', 'prg、chr'], ['寄存器范围', 'cpu $8000-$FFFF（写）'], ['状态声明', 'prgBank（8 位）'], ['位置', 'D:\\GitCode\\my-mappers\\demo-bank']],
    },
  };

  function showMapperDetail(key) {
    const detail = MAPPER_DETAILS[key];
    $('#mapDetailTitle').textContent = detail.title;
    const rows = detail.facts.map(([label, value]) => {
      const row = h('div', 'ui-facts-row');
      row.append(h('dt', '', label), h('dd', '', value));
      return row;
    });
    $('#mapDetailFacts').replaceChildren(...rows);
  }

  function filterMappers() {
    const filter = $('#mapFilter').value;
    filterRows(mapperTable, $('#mapSearch').value, (row) => filter === 'all' || row.dataset.source === filter);
  }

  mapperTable.addEventListener('click', (event) => {
    const row = event.target.closest('tr[data-row]');
    if (!row) return;
    selectRow(mapperTable, row);
    showMapperDetail(row.dataset.detail);
  });

  $('#mapSearch').addEventListener('input', filterMappers);
  $('#mapFilter').addEventListener('change', filterMappers);
  selectRow(mapperTable, $('tr[data-detail="nrom"]', mapperTable));
  showMapperDetail('nrom');

  // ---------- 设备（边栏） ----------

  const floppies = { A: { image: 'SB2K_DOS.IMG', track: 12 }, B: null };
  const SPARE_IMAGES = { A: 'SB2K_DOS.IMG', B: 'DATA01.IMG' };

  // 右边栏读取软驱的盘片与写保护状态
  Ember.devices = {
    floppy: (drive) => floppies[drive],
    writeProtected: (drive) => $('#floppy' + drive + 'Wp').checked,
  };

  function machineRunning() {
    const machine = Ember.machine.state;
    return Boolean(machine.host && machine.power && !machine.paused);
  }

  // 原型只模拟软驱 A 的读写；B 有盘时显示空闲
  function floppyBusy(drive) {
    return drive === 'A' && Boolean(floppies.A) && machineRunning();
  }

  // 活动栏设备图标上的指示灯：任一软驱读写时亮；读写是断续的，所以随机闪动
  const driveLed = $('#driveLed');
  setInterval(() => {
    driveLed.classList.toggle('is-on', (floppyBusy('A') || floppyBusy('B')) && Math.random() < 0.7);
  }, 150);

  function renderFloppy(drive) {
    const disk = floppies[drive];
    let state = '无盘片';
    if (disk) state = floppyBusy(drive) ? '读写中，第 ' + disk.track + ' 磁道' : '空闲';
    $('#floppy' + drive + 'Image').textContent = disk ? disk.image : '空';
    $('#floppy' + drive + 'State').textContent = state;
    $('#floppy' + drive + 'Eject').disabled = !disk;
    $('#floppy' + drive + 'Wp').disabled = !disk;
  }

  ['A', 'B'].forEach((drive) => {
    $('#floppy' + drive + 'Wp').addEventListener('change', (event) => {
      Ember.log('信息', '软驱 ' + drive + ' 写保护' + (event.target.checked ? '已开启' : '已关闭'));
      Ember.floppyFiles.refresh();
    });
  });

  setInterval(() => {
    if (floppies.A && machineRunning()) floppies.A.track = Math.floor(Math.random() * 80);
    renderFloppy('A');
  }, 1400);

  const tape = { kind: 'data', loaded: false, mode: 'stopped', position: 0, total: 200 };
  const TAPE_FILES = { data: 'PROGRAM.wav', av: 'QLS.m4a' };
  const TAPE_TOTALS = { data: 200, av: 312 };
  const TAPE_MODE_TEXT = { stopped: '已停止', playing: '播放中', recording: '录音中' };

  function formatTime(seconds) {
    const whole = Math.floor(seconds);
    return String(Math.floor(whole / 60)).padStart(2, '0') + ':' + String(whole % 60).padStart(2, '0');
  }

  function renderTape() {
    const stopped = tape.mode === 'stopped';
    $('#tapeFile').textContent = tape.loaded ? TAPE_FILES[tape.kind] : '空';
    $('#tapeState').textContent = tape.loaded ? TAPE_MODE_TEXT[tape.mode] : '无磁带';
    $('#tapeTime').textContent = tape.loaded ? formatTime(tape.position) + ' / ' + formatTime(tape.total) : '—';
    const percent = tape.loaded ? Math.min(100, (tape.position / tape.total) * 100) : 0;
    $('#tapeProgress').setAttribute('aria-valuenow', String(Math.round(percent)));
    $('#tapeFill').style.width = percent + '%';
    $('#tapeLevels').hidden = tape.kind !== 'av';
    $('#tapeLoad').disabled = !stopped;
    $('#tapeEject').disabled = !tape.loaded || !stopped;
    $('#tapePlay').disabled = !tape.loaded || !stopped;
    $('#tapeRecord').disabled = !tape.loaded || !stopped || tape.kind === 'av';
    $('#tapeStop').disabled = stopped;
  }

  function startTape(mode) {
    if (!tape.loaded || tape.mode !== 'stopped') return;
    if (mode === 'playing' && tape.position >= tape.total) tape.position = 0;
    tape.mode = mode;
    renderTape();
    Ember.log('信息', mode === 'playing' ? '磁带机开始播放' : '磁带机开始录音');
  }

  function stopTape() {
    if (tape.mode === 'stopped') return;
    tape.mode = 'stopped';
    $('#levelL').style.width = '0';
    $('#levelR').style.width = '0';
    renderTape();
    Ember.log('信息', '磁带机已停止');
  }

  setInterval(() => {
    if (tape.mode === 'stopped') return;
    tape.position += 0.2;
    if (tape.mode === 'recording') tape.total = Math.max(tape.total, tape.position);
    if (tape.position >= tape.total) {
      tape.position = tape.total;
      stopTape();
      return;
    }
    if (tape.kind === 'av') {
      $('#levelL').style.width = 30 + Math.random() * 60 + '%';
      $('#levelR').style.width = 30 + Math.random() * 60 + '%';
    }
    renderTape();
  }, 200);

  // 打印机只显示实时状态：打印中、打印完成；机器没有运行时为空闲
  const PRINTER_TEXT = { idle: '空闲', printing: '打印中', done: '打印完成' };
  const PRINTER_PRINTING_SECONDS = 5;
  const PRINTER_DONE_SECONDS = 6;
  const printer = { phase: 'printing', seconds: 0 };

  // 文件名按打印时的日期时间生成
  function printerFileName(date) {
    const two = (value) => String(value).padStart(2, '0');
    const day = date.getFullYear() + two(date.getMonth() + 1) + two(date.getDate());
    const time = two(date.getHours()) + two(date.getMinutes()) + two(date.getSeconds());
    return 'printer-' + day + '-' + time + '.png';
  }

  function renderPrinter() {
    $('#printerState').textContent = PRINTER_TEXT[printer.phase];
  }

  setInterval(() => {
    if (!machineRunning()) {
      printer.phase = 'idle';
      printer.seconds = 0;
      renderPrinter();
      return;
    }
    if (printer.phase === 'idle') printer.phase = 'printing';
    printer.seconds += 1;
    if (printer.phase === 'printing' && printer.seconds >= PRINTER_PRINTING_SECONDS) {
      const name = printerFileName(new Date());
      printer.phase = 'done';
      printer.seconds = 0;
      Ember.log('信息', '打印机输出一页：' + name);
      Ember.notify('已打印：' + name);
    } else if (printer.phase === 'done' && printer.seconds >= PRINTER_DONE_SECONDS) {
      printer.phase = 'printing';
      printer.seconds = 0;
    }
    renderPrinter();
  }, 1000);

  document.addEventListener('segmented-change', (event) => {
    const { name, value } = event.detail;
    if (name === 'theme') Ember.theme.set(value);
    if (name === 'tapeKind') {
      tape.kind = value;
      tape.mode = 'stopped';
      tape.loaded = false;
      tape.position = 0;
      tape.total = TAPE_TOTALS[value];
      $('#levelL').style.width = '0';
      $('#levelR').style.width = '0';
      renderTape();
    }
  });

  // ---------- 机型与 BIOS（首选项） ----------

  // 示意数据：只有 sb2000 已支持并添加了 BIOS；添加、替换、移除只改这份数据，没有真实读写文件
  const SAMPLE_BIOS = { name: 'SB2K_BIOS.NES', size: '128 KB' };
  const biosMachines = [
    { id: 'sb97', name: '小霸王 97', supported: false, file: null },
    { id: 'sb2000', name: '小霸王 2000', supported: true, file: SAMPLE_BIOS },
    { id: 'bbk10', name: '步步高软驱一号', supported: true, file: null },
    { id: 'bbk20', name: '步步高 98', supported: true, file: null },
  ];

  // “文件”菜单的“开机…”读取这份数据
  Ember.bios = { list: () => biosMachines };

  function renderBios() {
    $$('[data-bios-row]').forEach((row) => {
      const item = biosMachines.find((candidate) => candidate.id === row.dataset.biosRow);
      row.classList.toggle('is-disabled', !item.supported);
      let state = '未添加';
      if (!item.supported) state = '暂未支持';
      else if (item.file) state = item.file.name + '（' + item.file.size + '）';
      $('[data-bios-state]', row).textContent = state;
      $$('button', row).forEach((button) => {
        button.disabled = !item.supported;
      });
      $('[data-command="machines.addBios"]', row).hidden = Boolean(item.file);
      $('[data-command="machines.replaceBios"]', row).hidden = !item.file;
      $('[data-command="machines.removeBios"]', row).hidden = !item.file;
    });
  }

  // ---------- 首选项 ----------

  function selectPreferenceGroup(group) {
    $$('[data-pref-group]').forEach((button) => button.classList.toggle('is-selected', button.dataset.prefGroup === group));
    $$('[data-pref-panel]').forEach((panel) => {
      panel.hidden = panel.dataset.prefPanel !== group;
    });
  }

  $$('[data-pref-group]').forEach((button) => {
    button.addEventListener('click', () => selectPreferenceGroup(button.dataset.prefGroup));
  });

  $('#swDebug').addEventListener('change', (event) => {
    const on = event.target.checked;
    $('#statusDebugText').textContent = on ? '调试 127.0.0.1:5688，1 个连接' : '调试已关闭';
    Ember.log('信息', on ? '调试服务已开启：127.0.0.1:5688' : '调试服务已关闭');
  });

  // ---------- 命令 ----------

  const commands = Ember.commands;
  commands.register('library.addFolder', () => Ember.notify('原型：这里会打开系统的文件夹选择窗口。'));
  commands.register('mappers.install', () => Ember.notify('原型：这里会打开系统的文件选择窗口，选择 .emap 插件包。'));
  commands.register('media.floppyInsert', (source) => {
    const drive = source.dataset.drive;
    floppies[drive] = { image: SPARE_IMAGES[drive], track: 0 };
    renderFloppy(drive);
    Ember.floppyFiles.refresh();
    Ember.machine.refreshScreen();
    Ember.log('信息', '软驱 ' + drive + ' 已载入 ' + SPARE_IMAGES[drive]);
  });
  commands.register('media.floppyEject', (source) => {
    const drive = source.dataset.drive;
    floppies[drive] = null;
    renderFloppy(drive);
    Ember.floppyFiles.refresh();
    Ember.machine.refreshScreen();
    Ember.log('信息', '软驱 ' + drive + ' 已取出盘片');
  });
  commands.register('media.tapeLoad', () => {
    tape.loaded = true;
    tape.position = 0;
    tape.total = TAPE_TOTALS[tape.kind];
    renderTape();
    Ember.log('信息', '磁带机已载入 ' + TAPE_FILES[tape.kind]);
  });
  commands.register('media.tapeEject', () => {
    tape.loaded = false;
    tape.position = 0;
    renderTape();
    Ember.log('信息', '磁带机已取出磁带');
  });
  commands.register('media.tapePlay', () => startTape('playing'));
  commands.register('media.tapeRecord', () => startTape('recording'));
  commands.register('media.tapeStop', stopTape);
  commands.register('printer.chooseFolder', () => Ember.notify('原型：这里会打开系统的文件夹选择窗口。'));

  function biosMachineOf(source) {
    return biosMachines.find((item) => item.id === source.dataset.machine);
  }

  commands.register('machines.addBios', (source) => {
    const item = biosMachineOf(source);
    item.file = SAMPLE_BIOS;
    renderBios();
    Ember.log('信息', '已为 ' + item.name + ' 添加 BIOS：' + SAMPLE_BIOS.name);
    Ember.notify('原型：这里会打开系统的文件选择窗口。选定的 BIOS 文件会复制到软件的 BIOS 目录，这里已示意添加 ' + SAMPLE_BIOS.name + '。');
  });
  commands.register('machines.replaceBios', (source) => {
    const item = biosMachineOf(source);
    renderBios();
    Ember.log('信息', '已替换 ' + item.name + ' 的 BIOS：' + SAMPLE_BIOS.name);
    Ember.notify('原型：这里会打开系统的文件选择窗口。新文件复制到 BIOS 目录后，才删除旧文件。');
  });
  commands.register('machines.removeBios', (source) => {
    const item = biosMachineOf(source);
    item.file = null;
    renderBios();
    Ember.log('信息', '已移除 ' + item.name + ' 的 BIOS');
    Ember.notify('已移除 ' + item.name + ' 的 BIOS（只删除软件 BIOS 目录里的文件，原文件不受影响）。开机时需要重新添加。');
  });
  renderBios();

  Ember.pages = {
    openPreferences(group) {
      Ember.shell.openPage('preferences');
      selectPreferenceGroup(group);
    },
  };

  renderFloppy('A');
  renderFloppy('B');
  renderTape();
  renderPrinter();
})();
