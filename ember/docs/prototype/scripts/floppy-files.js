// ------------------------------------------------------------------------
// 名称：floppy-files.js
// 说明：右边栏：查看与管理软驱里的文件，拖入添加、右键删除与改名，并与运行画面同步。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：原型脚本；文件内容都是示意，没有真实读写镜像。
// ------------------------------------------------------------------------

(function () {
  'use strict';

  const Ember = window.Ember;
  const { $, h } = Ember;

  const CAPACITY = 1457664;
  const SECTOR = 512;
  const MAX_DIR_LINES = 7;
  const NAME_PATTERN = /^[A-Z0-9_$~!#-]{1,8}(\.[A-Z0-9_$~!#-]{1,3})?$/;
  const NAME_RULE = '名称不符合 8.3 规则：主名 1 到 8 个字符，扩展名最多 3 个字符，只能用字母、数字和 _ $ ~ ! # -。';
  const CHANGED_MILLISECONDS = 1600;
  const GUEST_DEMO_MILLISECONDS = 12000;

  function entry(name, size, modified = new Date()) {
    return { name, size, modified };
  }

  // 镜像里的文件按镜像名保存，取出再载入时内容保持
  const images = {
    'SB2K_DOS.IMG': [
      entry('HELLO.EXE', 4096, new Date(2026, 9, 28, 14, 5)),
      entry('HELLO.TXT', 512, new Date(2026, 9, 28, 14, 7)),
      entry('DEMO.BAS', 2048, new Date(2026, 10, 2, 20, 41)),
      entry('SONG01.MID', 18432, new Date(2026, 10, 9, 9, 30)),
    ],
    'DATA01.IMG': [entry('NOTES.TXT', 1024, new Date(2026, 10, 5, 18, 12)), entry('SAVE01.DAT', 8192, new Date(2026, 10, 11, 22, 3))],
  };
  const unsaved = new Set();
  const changed = new Set();
  const view = { drive: 'A', selected: null, renaming: null };

  const panel = $('#secondarySide');
  const body = $('#ffBody');

  function disk(drive) {
    return Ember.devices.floppy(drive);
  }

  function filesOf(drive) {
    const image = disk(drive).image;
    if (!images[image]) images[image] = [];
    return images[image];
  }

  function usedBytes(files) {
    return files.reduce((sum, item) => sum + Math.ceil(item.size / SECTOR) * SECTOR, 0);
  }

  function formatSize(bytes) {
    if (bytes < 1024) return bytes + ' B';
    if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(bytes % 1024 ? 1 : 0) + ' KB';
    return (bytes / 1024 / 1024).toFixed(2) + ' MB';
  }

  function formatModified(date) {
    const two = (value) => String(value).padStart(2, '0');
    return date.getFullYear() + '-' + two(date.getMonth() + 1) + '-' + two(date.getDate()) + ' ' + two(date.getHours()) + ':' + two(date.getMinutes());
  }

  function machineRunning() {
    const machine = Ember.machine.state;
    return Boolean(machine.cartridge && machine.power && !machine.paused);
  }

  // ---------- 运行画面里的 DIR ----------

  function dirLine(item) {
    const dot = item.name.indexOf('.');
    const base = dot < 0 ? item.name : item.name.slice(0, dot);
    const extension = dot < 0 ? '' : item.name.slice(dot + 1);
    const kilobytes = Math.max(1, Math.ceil(item.size / 1024));
    return ' ' + base.padEnd(9) + extension.padEnd(4) + (kilobytes + 'K').padStart(7);
  }

  function dirLines() {
    if (!disk('A')) return [' 软驱 A 没有盘片'];
    const files = filesOf('A');
    if (!files.length) return [' 文件未找到'];
    const lines = files.slice(0, MAX_DIR_LINES).map(dirLine);
    if (files.length > MAX_DIR_LINES) lines.push(' …还有 ' + (files.length - MAX_DIR_LINES) + ' 个文件');
    return lines;
  }

  // ---------- 确认对话框 ----------

  function confirm(title, text, okLabel) {
    $('#confirmTitle').textContent = title;
    $('#confirmText').textContent = text;
    $('#confirmOk').textContent = okLabel;
    return new Promise((resolve) => {
      const ok = $('#confirmOk');
      let settled = false;
      const finish = (value) => {
        if (settled) return;
        settled = true;
        ok.removeEventListener('click', onOk);
        document.removeEventListener('dialogs-closed', onClosed);
        resolve(value);
      };
      const onOk = () => {
        finish(true);
        Ember.dialog.closeAll();
      };
      const onClosed = () => finish(false);
      ok.addEventListener('click', onOk);
      document.addEventListener('dialogs-closed', onClosed);
      Ember.dialog.open('confirmDialog');
    });
  }

  // ---------- 显示 ----------

  function renderDrive() {
    const select = $('#ffDrive');
    select.replaceChildren(
      ...['A', 'B'].map((drive) => {
        const option = h('option', '', '软驱 ' + drive + '（' + (disk(drive) ? disk(drive).image : '无盘片') + '）');
        option.value = drive;
        option.disabled = !disk(drive);
        return option;
      }),
    );
    select.value = view.drive;
  }

  function renderInfo() {
    if (!disk(view.drive)) return;
    const image = disk(view.drive).image;
    $('#ffImage').textContent = image + (unsaved.has(image) ? '（有未保存的修改）' : '');
    $('#ffUsed').textContent = formatSize(usedBytes(filesOf(view.drive))) + ' / ' + formatSize(CAPACITY);
    let sync = '未运行，直接改镜像';
    if (Ember.devices.writeProtected(view.drive)) sync = '写保护，不能修改';
    else if (machineRunning()) sync = '运行中，实时同步';
    $('#ffSync').textContent = sync;
  }

  function rowOf(item) {
    const row = h('tr', (view.selected === item.name ? 'is-selected' : '') + (changed.has(item.name) ? ' is-changed' : ''));
    row.dataset.row = '';
    row.dataset.name = item.name;
    const label = h('span', 'ff-name');
    label.append(Ember.icon('i-file'));
    if (view.renaming === item.name) {
      const input = h('input', 'ui-input');
      input.type = 'text';
      input.value = item.name;
      input.maxLength = 12;
      input.setAttribute('aria-label', '新名称');
      input.addEventListener('keydown', (event) => {
        event.stopPropagation();
        if (event.key === 'Enter') commitRename(input, false);
        if (event.key === 'Escape') cancelRename();
      });
      input.addEventListener('input', () => input.removeAttribute('aria-invalid'));
      input.addEventListener('click', (event) => event.stopPropagation());
      input.addEventListener('blur', () => {
        if (view.renaming === item.name) commitRename(input, true);
      });
      label.append(input);
      setTimeout(() => {
        input.focus();
        input.select();
      });
    } else {
      label.append(h('span', '', item.name));
    }
    const nameCell = h('td');
    nameCell.append(label);
    row.append(nameCell, h('td', 'ff-size', formatSize(item.size)), h('td', 'ff-time', formatModified(item.modified)));
    return row;
  }

  function renderRows() {
    const files = filesOf(view.drive);
    if (!files.length) {
      const empty = h('tr');
      const cell = h('td', 'ff-empty', '盘里没有文件');
      cell.colSpan = 3;
      empty.append(cell);
      body.replaceChildren(empty);
      return;
    }
    body.replaceChildren(...files.map(rowOf));
  }

  function refresh() {
    const available = ['A', 'B'].some((drive) => disk(drive));
    Ember.shell.setSecondaryAvailable(available);
    if (!available) {
      view.selected = null;
      view.renaming = null;
      return;
    }
    if (!disk(view.drive)) {
      view.drive = disk('A') ? 'A' : 'B';
      view.selected = null;
      view.renaming = null;
    }
    renderDrive();
    renderInfo();
    renderRows();
  }

  // 修改完成后：标记未保存、短暂高亮变化的行，并让运行画面跟着变
  function afterChange(drive, names) {
    unsaved.add(disk(drive).image);
    names.forEach((name) => changed.add(name));
    setTimeout(() => {
      names.forEach((name) => changed.delete(name));
      if (disk(view.drive)) renderRows();
    }, CHANGED_MILLISECONDS);
    refresh();
    Ember.machine.refreshScreen();
  }

  // ---------- 改名与删除 ----------

  function cancelRename() {
    view.renaming = null;
    renderRows();
  }

  function commitRename(input, fromBlur) {
    const oldName = view.renaming;
    const newName = input.value.trim().toUpperCase();
    if (!newName || newName === oldName) {
      cancelRename();
      return;
    }
    const drive = view.drive;
    const files = filesOf(drive);
    let problem = '';
    if (!NAME_PATTERN.test(newName)) problem = NAME_RULE;
    else if (files.some((item) => item.name === newName)) problem = '软盘里已有名为 ' + newName + ' 的文件。';
    if (problem) {
      if (fromBlur) {
        cancelRename();
        return;
      }
      input.setAttribute('aria-invalid', 'true');
      Ember.notify(problem, 'warning');
      return;
    }
    files.find((item) => item.name === oldName).name = newName;
    view.renaming = null;
    view.selected = newName;
    afterChange(drive, [newName]);
    Ember.notify('已将 ' + oldName + ' 改名为 ' + newName);
    Ember.log('信息', '软驱 ' + drive + '：' + oldName + ' 改名为 ' + newName);
  }

  async function deleteFile(name) {
    const drive = view.drive;
    const ok = await confirm('删除文件', '删除“' + name + '”？软盘镜像会立即改变，无法撤销。', '删除');
    if (!ok || !disk(drive)) return;
    const files = filesOf(drive);
    const index = files.findIndex((item) => item.name === name);
    if (index < 0) return;
    files.splice(index, 1);
    view.selected = null;
    afterChange(drive, []);
    Ember.notify('已删除 ' + name);
    Ember.log('信息', '软驱 ' + drive + '：已删除 ' + name);
  }

  // ---------- 拖入添加 ----------

  async function addFiles(dropped) {
    const drive = view.drive;
    if (Ember.devices.writeProtected(drive)) {
      Ember.notify('软驱 ' + drive + ' 写保护，不能添加文件。', 'warning');
      return;
    }
    const added = [];
    for (const file of dropped) {
      if (!disk(drive)) break;
      const name = file.name.toUpperCase();
      if (!NAME_PATTERN.test(name)) {
        Ember.notify('“' + file.name + '”的' + NAME_RULE, 'warning');
        continue;
      }
      const files = filesOf(drive);
      const existing = files.find((item) => item.name === name);
      const free = CAPACITY - usedBytes(files) + (existing ? Math.ceil(existing.size / SECTOR) * SECTOR : 0);
      if (Math.ceil(file.size / SECTOR) * SECTOR > free) {
        Ember.notify('软盘空间不足，没有添加“' + file.name + '”。', 'warning');
        continue;
      }
      if (existing) {
        const ok = await confirm('替换文件', '软盘里已有“' + name + '”，要替换吗？', '替换');
        if (!ok) continue;
        existing.size = file.size;
        existing.modified = new Date(file.lastModified);
      } else {
        files.push(entry(name, file.size, new Date(file.lastModified)));
      }
      added.push(name);
    }
    if (!added.length) return;
    afterChange(drive, added);
    Ember.notify(added.length === 1 ? '已添加 ' + added[0] + ' 到软驱 ' + drive : '已添加 ' + added.length + ' 个文件到软驱 ' + drive);
    Ember.log('信息', '软驱 ' + drive + '：已添加 ' + added.join('、'));
  }

  function hasFiles(event) {
    return Array.from(event.dataTransfer ? event.dataTransfer.types : []).includes('Files');
  }

  ['dragenter', 'dragover'].forEach((type) => {
    panel.addEventListener(type, (event) => {
      if (!hasFiles(event)) return;
      event.preventDefault();
      event.dataTransfer.dropEffect = 'copy';
      panel.classList.add('is-drag-over');
    });
  });

  panel.addEventListener('dragleave', (event) => {
    if (!panel.contains(event.relatedTarget)) panel.classList.remove('is-drag-over');
  });

  panel.addEventListener('drop', (event) => {
    if (!hasFiles(event)) return;
    event.preventDefault();
    panel.classList.remove('is-drag-over');
    addFiles(Array.from(event.dataTransfer.files));
  });

  // ---------- 选择与右键菜单 ----------

  $('#ffDrive').addEventListener('change', (event) => {
    view.drive = event.target.value;
    view.selected = null;
    view.renaming = null;
    refresh();
  });

  body.addEventListener('click', (event) => {
    const row = event.target.closest('tr[data-row]');
    if (!row || view.renaming) return;
    view.selected = row.dataset.name;
    renderRows();
  });

  body.addEventListener('contextmenu', (event) => {
    const row = event.target.closest('tr[data-row]');
    if (!row || view.renaming) return;
    event.preventDefault();
    const name = row.dataset.name;
    view.selected = name;
    renderRows();
    const locked = Ember.devices.writeProtected(view.drive);
    Ember.menu.popup(event.clientX, event.clientY, [
      {
        label: '重命名',
        disabled: locked,
        action: () => {
          view.renaming = name;
          renderRows();
        },
      },
      { label: '删除', disabled: locked, action: () => deleteFile(name) },
    ]);
  });

  // ---------- 学习机写盘（原型用定时模拟） ----------

  function markSaved(drive) {
    if (disk(drive)) unsaved.delete(disk(drive).image);
    refresh();
  }

  setInterval(renderInfo, 1000);

  const guestDemo = setInterval(() => {
    if (!machineRunning() || !disk('A') || Ember.devices.writeProtected('A') || view.renaming) return;
    clearInterval(guestDemo);
    const files = filesOf('A');
    if (files.some((item) => item.name === 'SAVE01.DAT')) return;
    files.push(entry('SAVE01.DAT', 2048));
    afterChange('A', ['SAVE01.DAT']);
    Ember.log('信息', '软驱 A：学习机写入 SAVE01.DAT');
  }, GUEST_DEMO_MILLISECONDS);

  Ember.floppyFiles = { refresh, dirLines, markSaved };
  refresh();
})();
