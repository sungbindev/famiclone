// ------------------------------------------------------------------------
// 名称：machine.js
// 说明：机器状态与画面：上电、断电、复位、暂停、速度、音量，以及示意画面的绘制和状态显示。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-13
// 备注：原型脚本；画面是被模拟的内容，颜色写在本文件的常量里，不随界面主题变化。
// ------------------------------------------------------------------------

(function () {
  'use strict';

  const VN2 = window.VN2;
  const { $, h } = VN2;

  const canvas = $('#screen');
  const context = canvas.getContext('2d');
  const WIDTH = canvas.width;
  const HEIGHT = canvas.height;

  // 示意画面的取值
  const SCREEN_BACKGROUND = '#10131f';
  const SCREEN_BLACK = '#000000';
  const SCREEN_TEXT = '#d9dde8';
  const GAME_SKY = '#5c94fc';
  const GAME_GROUND = '#c84c0c';
  const GAME_CLOUD = '#fcfcfc';
  const GAME_PLAYER = '#e02020';
  const PALETTE = ['#000000', '#e04b4b', '#4bc26a', '#e0c34b', '#4b7be0', '#c24bc2', '#4bc2c2', '#ffffff'];
  const BOOT_LINES = ['SUBOR SB-DOS  Version 1.0', '内存 512K    鼠标 已连接    软驱 A:', '', 'A>DIR', ' SB2K_DOS  IMG    1440K', ' HELLO     EXE       4K', '', 'A>'];
  const HOST_NAMES = { sb2000: 'sb2000 学习机', fc: '经典 FC' };
  const FRAME_RATES = { um6576: '50.0 fps', fc: '60.1 fps' };

  const machine = {
    power: true,
    paused: false,
    cartridge: 'SB2K_BIOS.NES',
    host: 'sb2000',
    plugin: 'sb2000 1.0.0',
    speed: '100%',
    volume: 80,
    muted: false,
  };

  let visibleLines = BOOT_LINES.length;
  let cursorOn = true;
  let bootTimer = 0;

  function drawDos() {
    context.fillStyle = SCREEN_BACKGROUND;
    context.fillRect(0, 0, WIDTH, HEIGHT);
    context.font = '20px "Cascadia Mono", Consolas, "Microsoft YaHei", monospace';
    context.textBaseline = 'top';
    context.fillStyle = SCREEN_TEXT;
    for (let i = 0; i < visibleLines; i += 1) context.fillText(BOOT_LINES[i], 24, 24 + i * 28);
    if (cursorOn && visibleLines === BOOT_LINES.length && !machine.paused) {
      const last = BOOT_LINES.length - 1;
      context.fillRect(24 + context.measureText(BOOT_LINES[last]).width + 2, 24 + last * 28 + 2, 12, 20);
    }
    const cell = (WIDTH - 48) / PALETTE.length;
    PALETTE.forEach((color, i) => {
      context.fillStyle = color;
      context.fillRect(24 + i * cell, HEIGHT - 64, cell, 40);
    });
  }

  function drawGame() {
    context.fillStyle = GAME_SKY;
    context.fillRect(0, 0, WIDTH, HEIGHT);
    context.fillStyle = GAME_CLOUD;
    [[80, 90], [320, 140], [420, 70]].forEach(([x, y]) => {
      context.fillRect(x, y, 96, 24);
      context.fillRect(x + 16, y - 16, 56, 16);
    });
    context.fillStyle = GAME_GROUND;
    context.fillRect(0, HEIGHT - 96, WIDTH, 96);
    context.fillStyle = GAME_PLAYER;
    context.fillRect(120, HEIGHT - 160, 32, 64);
  }

  function draw() {
    context.fillStyle = SCREEN_BLACK;
    context.fillRect(0, 0, WIDTH, HEIGHT);
    if (!machine.power || !machine.cartridge) return;
    if (machine.host === 'sb2000') drawDos();
    else drawGame();
  }

  function boot() {
    clearTimeout(bootTimer);
    if (machine.host !== 'sb2000') {
      draw();
      return;
    }
    let shown = 0;
    const step = () => {
      shown += 1;
      visibleLines = shown;
      draw();
      if (shown < BOOT_LINES.length) bootTimer = setTimeout(step, 180);
    };
    visibleLines = 0;
    step();
  }

  function setButton(button, iconId, label) {
    button.replaceChildren(VN2.icon(iconId), h('span', '', label));
  }

  function update() {
    const empty = !machine.cartridge;
    let label = '运行中';
    let dot = 'on';
    if (empty) {
      label = '无卡带';
      dot = 'off';
    } else if (!machine.power) {
      label = '已断电';
      dot = 'off';
    } else if (machine.paused) {
      label = '已暂停';
      dot = 'paused';
    }
    const running = !empty && machine.power && !machine.paused;
    const cartridgeText = machine.cartridge || '（空）';

    $('#statusPower').textContent = label;
    $('#statusDot').dataset.state = dot;
    $('#factState').textContent = label;
    $('#statusCartridge').textContent = cartridgeText;
    $('#factCartridge').textContent = cartridgeText;
    $('#statusHost').textContent = empty ? '—' : HOST_NAMES[machine.host];
    $('#factHost').textContent = empty ? '—' : HOST_NAMES[machine.host];
    $('#statusPlugin').textContent = empty ? '—' : machine.plugin;
    $('#factPlugin').textContent = empty ? '—' : machine.plugin;
    $('#statusFps').textContent = running ? FRAME_RATES[machine.host] : '— fps';
    $('#statusSpeed').textContent = '速度 ' + machine.speed;
    $('#statusVolume').textContent = machine.muted ? '已静音' : '音量 ' + machine.volume + '%';

    const power = $('#btnPower');
    setButton(power, 'i-power', machine.power ? '断电' : '上电');
    power.disabled = empty;
    const pause = $('#btnPause');
    setButton(pause, machine.paused ? 'i-play' : 'i-pause', machine.paused ? '继续' : '暂停');
    pause.disabled = empty || !machine.power;
    $('.side [data-command="machine.softReset"]').disabled = empty || !machine.power;
    $('.side [data-command="machine.hardReset"]').disabled = empty;
    $('#btnMute use').setAttribute('href', machine.muted ? '#i-volume-off' : '#i-volume');
    $('#mouseTag').hidden = empty || !machine.power || machine.host !== 'sb2000';

    const overlay = $('#screenOverlay');
    let icon = '';
    let title = '';
    let text = '';
    if (empty) {
      icon = 'i-cartridge';
      title = '未插入卡带';
      text = '在“文件”菜单里打开 NES 文件，或在“卡带库”页双击卡带。';
    } else if (!machine.power) {
      icon = 'i-power';
      title = '已断电';
      text = '点击边栏的“上电”。';
    } else if (machine.paused) {
      icon = 'i-pause';
      title = '已暂停';
    }
    overlay.hidden = !title;
    $('use', overlay).setAttribute('href', '#' + (icon || 'i-pause'));
    $('#overlayTitle').textContent = title;
    $('#overlayText').textContent = text;
    draw();
  }

  VN2.machine = {
    state: machine,
    powerToggle() {
      if (!machine.cartridge) return;
      machine.power = !machine.power;
      machine.paused = false;
      if (machine.power) boot();
      update();
      VN2.log('信息', machine.power ? '上电' : '断电');
    },
    pauseToggle() {
      if (!machine.cartridge || !machine.power) return;
      machine.paused = !machine.paused;
      update();
      VN2.log('信息', machine.paused ? '已暂停' : '继续运行');
    },
    softReset() {
      if (!machine.cartridge || !machine.power) return;
      machine.paused = false;
      boot();
      update();
      VN2.log('信息', '软复位');
      VN2.notify('已软复位');
    },
    hardReset() {
      if (!machine.cartridge) return;
      machine.power = true;
      machine.paused = false;
      boot();
      update();
      VN2.log('信息', '硬复位');
      VN2.notify('已硬复位');
    },
    muteToggle() {
      machine.muted = !machine.muted;
      update();
    },
    eject() {
      if (!machine.cartridge) return;
      VN2.log('信息', '已取出卡带 ' + machine.cartridge);
      machine.cartridge = null;
      update();
    },
    load(info) {
      machine.cartridge = info.name;
      machine.host = info.host;
      machine.plugin = info.plugin;
      machine.power = true;
      machine.paused = false;
      boot();
      update();
      VN2.log('信息', '已装配主机 ' + HOST_NAMES[info.host]);
      VN2.log('信息', 'mapper ' + info.mapper + '、变体 ' + info.variant + ' 匹配到插件 ' + info.plugin);
      VN2.shell.openPage('run');
    },
    screenshot() {
      if (!machine.cartridge || !machine.power) {
        VN2.notify('机器没有画面，无法截图。', 'warning');
        return;
      }
      VN2.log('信息', '截图已保存');
      VN2.notify('截图已保存到图片文件夹');
    },
  };

  $('#selSpeed').addEventListener('change', (event) => {
    machine.speed = event.target.value;
    update();
    VN2.log('信息', '速度改为 ' + machine.speed);
  });

  $('#sliderVolume').addEventListener('input', (event) => {
    machine.volume = Number(event.target.value);
    $('#volumeValue').textContent = machine.volume + '%';
    update();
  });

  $('#selAspect').addEventListener('change', (event) => {
    const [width, height] = event.target.value.split(':');
    const stage = $('#runStage');
    stage.style.setProperty('--screen-ratio-w', width);
    stage.style.setProperty('--screen-ratio-h', height);
  });

  setInterval(() => {
    if (machine.cartridge && machine.power && !machine.paused && machine.host === 'sb2000') {
      cursorOn = !cursorOn;
      draw();
    }
  }, 530);

  VN2.machine.refresh = update;
})();
