// ------------------------------------------------------------------------
// 名称：pages.js
// 说明：各页面的交互：卡带库、设备（软驱与磁带）、mapper 插件、首选项。
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

  libraryTable.addEventListener('click', (event) => {
    const row = event.target.closest('tr[data-row]');
    if (!row) return;
    selectRow(libraryTable, row);
    const callout = $('#libMissing');
    callout.hidden = row.dataset.status !== 'missing';
    if (!callout.hidden) {
      const info = readCartridge(row);
      $('span', callout).textContent = '选中的卡带缺少 mapper ' + info.mapper + '、变体 ' + info.variant + ' 的插件。在“mapper 插件”页安装后即可运行。';
    }
  });

  libraryTable.addEventListener('dblclick', (event) => {
    const row = event.target.closest('tr[data-row]');
    if (!row) return;
    const info = readCartridge(row);
    if (row.dataset.status === 'missing') {
      Ember.notify('无法运行 ' + info.name + '：缺少 mapper ' + info.mapper + '、变体 ' + info.variant + ' 的插件。', 'warning');
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
      facts: [['标识', 'nrom'], ['版本', '1.0.0'], ['发布者', 'Ember'], ['匹配', 'mapper 0'], ['主机型号', 'fc'], ['存储区', 'prg（ROM，来自文件）、chr（ROM；文件没有 CHR 时用 8K RAM）'], ['设备连线', '无'], ['寄存器范围', '无'], ['状态声明', '无']],
    },
    sb2000: {
      title: '小霸王 SB2000',
      facts: [['标识', 'sb2000'], ['版本', '1.0.0'], ['发布者', 'Ember'], ['匹配', 'mapper 171，变体 1'], ['主机型号', 'sb2000'], ['存储区', '待按真机资料核对'], ['设备连线', '软驱控制器 fdc（待按真机资料核对）'], ['寄存器范围', '待按真机资料核对'], ['状态声明', '待按真机资料核对']],
    },
    demo: {
      title: '示意 mapper',
      facts: [['标识', 'demo-bank'], ['版本', '0.1.0'], ['发布者', '示例发布者'], ['匹配', 'mapper 999'], ['主机型号', 'fc'], ['存储区', 'prg、chr'], ['设备连线', '无'], ['寄存器范围', 'cpu $8000-$FFFF（写）'], ['状态声明', 'prgBank（8 位）'], ['位置', 'D:\\GitCode\\my-mappers\\demo-bank']],
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

  // ---------- 设备 ----------

  const DEVICES = {
    floppyA: { title: '软驱 A', description: '软驱控制器在主机上，盘片可随时插入或取出，与电源无关。' },
    floppyB: { title: '软驱 B', description: '软驱控制器在主机上，盘片可随时插入或取出，与电源无关。' },
    tape: { title: '磁带机', description: '数据磁带用于 FBASIC 的 SAVE 与 LOAD；声像磁带带一路声音和一路画面数据，只能播放。' },
    printer: { title: '打印机', description: '并口上接的是虚拟针式打印机，按 ESC/P 命令打印，每一页保存为一张 PNG。' },
  };
  const floppies = { A: { image: 'SB2K_DOS.IMG' }, B: null };
  const SPARE_IMAGES = { A: 'SB2K_DOS.IMG', B: 'DATA01.IMG' };

  function selectDevice(key) {
    $$('[data-device]').forEach((button) => {
      const selected = button.dataset.device === key;
      button.classList.toggle('is-selected', selected);
      button.setAttribute('aria-selected', String(selected));
    });
    $$('[data-device-panel]').forEach((panel) => {
      panel.hidden = panel.dataset.devicePanel !== key;
    });
    $('#devTitle').textContent = DEVICES[key].title;
    $('#devDescription').textContent = DEVICES[key].description;
  }

  function renderFloppy(drive) {
    const disk = floppies[drive];
    const text = disk ? '已插入盘片' : '空';
    $('#floppy' + drive + 'State').textContent = text;
    $('#floppy' + drive + 'Image').textContent = disk ? disk.image : '—';
    $('[data-device-state="floppy' + drive + '"]').textContent = disk ? '已插入盘片' : '空';
    $('#floppy' + drive + 'Eject').disabled = !disk;
    const size = $('#floppy' + drive + 'Size');
    if (size) size.textContent = disk ? '1.44 MB' : '—';
    const save = $('#floppy' + drive + 'Save');
    if (save) save.disabled = !disk;
    const protect = $('#floppy' + drive + 'Wp');
    if (protect) protect.disabled = !disk;
    const track = $('#floppy' + drive + 'Track');
    if (track && !disk) track.textContent = '—';
  }

  $$('[data-device]').forEach((button) => {
    button.addEventListener('click', () => selectDevice(button.dataset.device));
  });

  $('#floppyAWp').addEventListener('change', (event) => {
    Ember.log('信息', '软驱 A 写保护' + (event.target.checked ? '已开启' : '已关闭'));
  });

  setInterval(() => {
    const machine = Ember.machine.state;
    if (floppies.A && machine.cartridge && machine.power && !machine.paused) {
      $('#floppyATrack').textContent = '第 ' + Math.floor(Math.random() * 80) + ' 磁道';
    }
  }, 1400);

  const tape = { kind: 'data', mode: 'stopped', position: 0, total: 200 };
  const TAPE_FILES = { data: 'PROGRAM.vtp', av: 'QLS.m4a' };
  const TAPE_TOTALS = { data: 200, av: 312 };
  const TAPE_MODE_TEXT = { stopped: '已停止', playing: '播放中', recording: '录音中' };

  function formatTime(seconds) {
    const whole = Math.floor(seconds);
    return String(Math.floor(whole / 60)).padStart(2, '0') + ':' + String(whole % 60).padStart(2, '0');
  }

  function renderTape() {
    const stopped = tape.mode === 'stopped';
    $('#tapeState').textContent = TAPE_MODE_TEXT[tape.mode];
    $('[data-device-state="tape"]').textContent = TAPE_MODE_TEXT[tape.mode];
    $('#tapeFile').textContent = TAPE_FILES[tape.kind];
    $('#tapeTime').textContent = formatTime(tape.position) + ' / ' + formatTime(tape.total);
    const percent = Math.min(100, (tape.position / tape.total) * 100);
    $('#tapeProgress').setAttribute('aria-valuenow', String(Math.round(percent)));
    $('#tapeFill').style.width = percent + '%';
    $('#tapeLevels').hidden = tape.kind !== 'av';
    $('#tapePlay').disabled = !stopped;
    $('#tapeRecord').disabled = !stopped || tape.kind === 'av';
    $('#tapeStop').disabled = stopped;
  }

  function startTape(mode) {
    if (tape.mode !== 'stopped') return;
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

  const printer = { pending: true, count: 0, last: '—', idleSeconds: 0 };
  const PRINTER_IDLE_SECONDS = 6;

  // 文件名按打印时的日期时间生成
  function printerFileName(date) {
    const two = (value) => String(value).padStart(2, '0');
    const day = date.getFullYear() + two(date.getMonth() + 1) + two(date.getDate());
    const time = two(date.getHours()) + two(date.getMinutes()) + two(date.getSeconds());
    return 'printer-' + day + '-' + time + '.png';
  }

  function renderPrinter() {
    $('#printerState').textContent = printer.pending ? '正在打印，当前页有内容' : '空闲';
    $('[data-device-state="printer"]').textContent = printer.pending ? '打印中' : '空闲';
    $('#printerCount').textContent = printer.count + ' 页';
    $('#printerLast').textContent = printer.last;
    $('#printerDir').textContent = $('#prefPrinterDir').value;
    $('#printerFlush').disabled = !printer.pending;
  }

  function flushPrinter() {
    if (!printer.pending) return;
    const name = printerFileName(new Date());
    printer.pending = false;
    printer.idleSeconds = 0;
    printer.count += 1;
    printer.last = name;
    renderPrinter();
    Ember.log('信息', '打印机输出一页：' + name);
    Ember.notify('已打印：' + name);
  }

  setInterval(() => {
    const machine = Ember.machine.state;
    if (printer.pending || !machine.cartridge || !machine.power || machine.paused) return;
    printer.idleSeconds += 1;
    if (printer.idleSeconds >= PRINTER_IDLE_SECONDS) {
      printer.pending = true;
      renderPrinter();
    }
  }, 1000);

  $('#prefPrinterDir').addEventListener('input', renderPrinter);

  document.addEventListener('segmented-change', (event) => {
    const { name, value } = event.detail;
    if (name === 'theme') Ember.theme.set(value);
    if (name === 'tapeKind') {
      tape.kind = value;
      tape.mode = 'stopped';
      tape.position = 0;
      tape.total = TAPE_TOTALS[value];
      renderTape();
    }
  });

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
    floppies[drive] = { image: SPARE_IMAGES[drive] };
    renderFloppy(drive);
    Ember.log('信息', '软驱 ' + drive + ' 已插入 ' + SPARE_IMAGES[drive]);
  });
  commands.register('media.floppyEject', (source) => {
    const drive = source.dataset.drive;
    floppies[drive] = null;
    renderFloppy(drive);
    Ember.log('信息', '软驱 ' + drive + ' 已取出盘片');
  });
  commands.register('media.floppySave', (source) => {
    Ember.notify('软驱 ' + source.dataset.drive + ' 的镜像已保存');
  });
  commands.register('media.tapePlay', () => startTape('playing'));
  commands.register('media.tapeRecord', () => startTape('recording'));
  commands.register('media.tapeStop', stopTape);
  commands.register('media.printerFlush', flushPrinter);
  commands.register('media.printerOpenFolder', () => Ember.notify('原型：这里会在系统的文件管理器里打开保存位置。'));
  commands.register('printer.chooseFolder', () => Ember.notify('原型：这里会打开系统的文件夹选择窗口。'));

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
