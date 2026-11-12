// ------------------------------------------------------------------------
// 名称：port-bios.mjs
// 说明：把原厂 Bios.asm 改成能在 SB2000（6576 模式）上运行的 src/Bios.asm。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用法 node tools/port-bios.mjs [--force]。读 machines/bbk/src/bios/Bios.asm（原版副本），按下面的规则替换后写入 src/Bios.asm。
//       src/Bios.asm 是提交进仓库的源码，可以直接手改；再运行本脚本会按规则重新生成，覆盖手工修改，
//       所以目标文件与生成结果不同时必须加 --force 才写。
//       文件按 latin1 读写，GBK 字节原样保留；只替换 ASCII 文字，硬件访问尽量一对一等长替换成对 HAL 的调用，
//       所以代码布局与原厂一致，只在加入 HAL、去掉监控器的地方变化。
//       “临时部分”（Begin 时复制到 $5800 运行的那段，到 MoveRAMEnding 标号为止）里的调用要写 RAM 地址（-$C000+$5800），
//       其余在 ROM 里运行的调用写 ROM 地址。
//       原厂源码里有“;comment %”开关：去掉分号就把调试监控器相关的代码块整块注释掉，这是正式版的配置，这里按它处理。
// ------------------------------------------------------------------------

import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { BIOS_SOURCE, ORIGINAL_BIOS_SOURCE } from './config.mjs';

/**
 * 把原厂 Bios.asm 的文字改成 SB2000 版。
 * @param {string} original 原厂源码（按 latin1 读出的文字）
 * @returns {{text: string, report: {label: string, count: number, expected: number}[]}} 改后的文字（与原文相同的换行）与各规则的命中情况
 */
export function portBios(original) {
  const crlf = original.includes('\r\n');
  let text = original.replace(/\r\n/g, '\n');
  const report = [];

  const applyRule = (source, label, pattern, replacement, expected) => {
    let count = 0;
    const result = source.replace(pattern, (...args) => {
      count += 1;
      return replacement.replace(/\$([1-3])(?![0-9a-fA-F])/g, (_, n) => (typeof args[Number(n)] === 'string' ? args[Number(n)] : ''));
    });
    report.push({ label, count, expected });
    return result;
  };

  /** 按正则全部替换，并记录替换次数；$1-$3 引用捕获组 */
  const rule = (label, pattern, replacement, expected) => {
    text = applyRule(text, label, pattern, replacement, expected);
  };

  /** 在“临时部分”与其余部分分别替换：前者写 RAM 地址，后者写 ROM 地址 */
  const callRule = (label, pattern, name, expectedRam, expectedRom) => {
    const split = text.indexOf('\nMoveRAMEnding:');
    if (split < 0) throw new Error('找不到 MoveRAMEnding');
    const ram = applyRule(text.slice(0, split), `${label}（RAM）`, pattern, `jsr\t${name}-$C000+$5800`, expectedRam);
    const rom = applyRule(text.slice(split), `${label}（ROM）`, pattern, `jsr\t${name}`, expectedRom);
    text = ram + rom;
  };

  // 一、端口符号：软驱换成 SB2000 的软驱口，其余没有硬件的端口落到空地址
  rule('FDCDMADackIO', /(FDCDMADackIO\s+equ\s+)\$ff80/g, '$1$4200', 1);
  rule('FDCDMATcIO', /(FDCDMATcIO\s+equ\s+)\$ff88/g, '$1$4201', 1);
  rule('FDCDRQPortI', /(FDCDRQPortI\s+equ\s+)\$ff90/g, '$1$4202', 1);
  rule('FDCCtrlPortO', /(FDCCtrlPortO\s+equ\s+)\$ff90/g, '$1$4101', 1);
  rule('FDCIRQPortI', /(FDCIRQPortI\s+equ\s+)\$ff98/g, '$1$4203', 1);
  rule('FDCResetPortO', /(FDCResetPortO\s+equ\s+)\$ffa0/g, '$1$4204', 1);
  rule('FDCStatPortI', /(FDCStatPortI\s+equ\s+)\$ffa0/g, '$1$4204', 1);
  rule('FDCDataPortIO', /(FDCDataPortIO\s+equ\s+)\$ffa8/g, '$1$4205', 1);
  rule('FDCChangePortI', /(FDCChangePortI\s+equ\s+)\$ffb8/g, '$1$4207', 1);
  rule('FDCSpeedPortO', /(FDCSpeedPortO\s+equ\s+)\$ffb8/g, '$1$4100', 1);
  rule('语音口', /((?:SpeakInitPort|SpeakDataPort)\s+equ\s+)\$ff(?:10|18)/g, '$1vDummy', 2);
  rule('PC 并口', /((?:PCDaCtPortO|PCCDataPort|PCCtrlPortI|PCCStatsPortI|PCDataPortIH|PCDataPortIL|PCCCtrlPoutO)\s+equ\s+)\$ff(?:40|48|50)/g, '$1vDummy', 7);
  rule('键盘灯口', /(KebBoardLEDPort\s+equ\s+)\$ff00/g, '$1vDummy', 1);
  rule('源码里写死的 $ff40/$ff48/$ff50', /\$ff(?:40|48|50)(?![0-9a-f])/gi, 'vDummy', 11);

  // 二、正式版配置：去掉调试监控器的代码块，冷启动入口走复位向量
  rule('去掉监控器代码块', /^;comment\s+%[^\n]*\n[\s\S]*?^;\s+%[^\n]*\n/gm, '', 7);
  rule('冷启动走复位向量', /^\s*jmp\s+MonitorEntryROM\s*;\s*%\s*\n;\s*jmp\s+\(\$fffc\)\s*;\s*~/m, '\tjmp\t($fffc)\t\t\t; ~', 1);

  // 三、包含硬件适配层
  rule('包含 HAL 定义', /^(\s*include\s+tvdef\.asm[^\n]*\n)/m, '$1\tinclude\thal-defs.asm\n', 1);
  rule('包含常驻 RAM 的 HAL', /^MoveRAMEnding:/m, '\tinclude\thal-resident.asm\nMoveRAMEnding:', 1);
  rule('包含 ROM 里的 HAL', /^(\s*org\s+\$ff00\s*;\s*Reserve for I\/O space)/m, '\tinclude\thal-ppu.asm\n$1', 1);

  // 四、启动流程里的特殊处理（必须先于下面的通用替换）
  rule('Begin 先初始化 HAL', /(Begin:\s*lda\s+#0[^\n]*\n\s*)sta\s+\$2000/g, '$1jsr\tHalInit', 1);
  rule('去掉 $C000-$FFFF 的 RAM 页寄存器设置', /\s*stx\s+DRAMPagePort(?:EF|CD)[^\n]*/g, '\n\tnop\n\tnop\n\tnop', 2);
  rule('显存大小固定为 32K', /;-+; Check VRAM\.\n[\s\S]*?(?=necvm08:)/g, ';---------------------------------------; Check VRAM.(32K fixed)\n\tlda\t#$f0\t\t\t; 16 x 2K pages.\n', 1);
  rule('内存检测不写 ROM 页 0', /lda\s+#0([^\n]*)\n(lpcpm08:)/g, 'lda\t#$80$1\n$2', 1);
  rule('PC 通讯卡不支持', /(ConnectPCCard:[^\n]*\n\s*)jsr\s+EnableNMI[^\n]*/g, '$1lda\t#0\t\t\t; PC link not supported on SB2000.\n\trts\t\t\t\t;', 1);
  rule('键盘改用 SB2000 的扩展码', /CheckScanCodeInp:[^\n]*\n[\s\S]*?necsi20:\s*sec[^\n]*\n\s*rts[^\n]*\n/g, [
    'CheckScanCodeInp:\t\t\t\t;',
    '\tjsr\tHalKbdGet\t\t\t; SB2000: XT scan code from the IRQ queue.',
    '\tbcs\tnecsi12\t\t\t; No scan code.(Cy=1,Zy=0)',
    '\tcmp\t#$e0\t\t\t; Extend lead code?',
    '\tbne\tnecsi04\t\t\t;',
    '\tlda\tKeyStatus3\t\t;',
    '\tora\t#$12\t\t\t;',
    '\tsta\tKeyStatus3\t\t;',
    '\tlda\t#0\t\t\t; No output.(Cy=1,Zy=1)',
    '\tsec\t\t\t\t;',
    '\trts\t\t\t\t;',
    'necsi04:sta\tTempSave2\t\t;',
    '\tldy\t#0\t\t\t;',
    '\tsty\tAutoRepeatDelay\t\t;',
    '\tjsr\tGetKeyCode\t\t;',
    '\tbcc\tnecsi12\t\t\t;',
    '\tldy\t#0\t\t\t; Cy=1 and No output.',
    'necsi12:rts\t\t\t\t;',
    '',
  ].join('\n'), 1);
  rule('键盘中断', /(BRKINTEntry:[\s\S]*?and\s+#\$10[^\n]*\n)(\s*lda\s+BRKASave[^\n]*\n)(BRKAddress:)/g, '$1\tjsr\tHalKbdIrq-$C000+$5800\t; SB2000 keyboard IRQ.\n\tbcc\tnebrk04\t\t\t;\n\tlda\tBRKASave\t\t;\n\trti\t\t\t\t;\nnebrk04:$2$3', 1);

  // 五、通用替换：硬件寄存器访问改成调用 HAL
  callRule('视频控制口', /sta\s+VideoCtrlPort/g, 'HalVideoCtrl', 12, 5);
  callRule('页寄存器（X）', /stx\s+DRAMPagePort(?![A-Za-z])/g, 'HalPageX', 1, 6);
  callRule('页寄存器（A）', /sta\s+DRAMPagePort(?![A-Za-z])/g, 'HalPageA', 4, 24);
  callRule('$2000', /sta\s+\$2000(?![0-9a-f])/gi, 'HalW2000', 2, 7);
  callRule('$2008', /sta\s+\$2008/g, 'HalW2000T', 2, 0);
  callRule('sta $2006', /sta\s+\$2006/g, 'HalW2006', 0, 12);
  callRule('stx $2006', /stx\s+\$2006/g, 'HalW2006X', 0, 2);
  callRule('sty $2006', /sty\s+\$2006/g, 'HalW2006Y', 0, 2);
  callRule('sta $2007', /sta\s+\$2007/g, 'HalW2007', 0, 24);
  callRule('lda $2007', /lda\s+\$2007/g, 'HalR2007', 0, 0);
  callRule('图案槽 0', /stx\s+VideoDataPort(?![+A-Za-z,])/g, 'HalSlot0X', 0, 2);
  callRule('图案槽 1', /stx\s+VideoDataPort\+8(?![0-9])/g, 'HalSlot1X', 0, 2);
  callRule('图案槽 2', /stx\s+VideoDataPort\+16/g, 'HalSlot2X', 0, 1);
  callRule('图案槽 3', /stx\s+VideoDataPort\+24/g, 'HalSlot3X', 0, 1);
  callRule('图案槽 A,X', /sta\s+VideoDataPort,x/g, 'HalSlotAX', 0, 1);
  rule('分屏队列口', /sta\s+\$ff(?:12|0a|1a|22)/gi, 'sta\tvDummy', 4);

  return { text: crlf ? text.replace(/\n/g, '\r\n') : text, report };
}

if (process.argv[1] && fileURLToPath(import.meta.url) === process.argv[1]) {
  const original = readFileSync(ORIGINAL_BIOS_SOURCE, 'latin1');
  const { text, report } = portBios(original);
  let failed = false;
  for (const { label, count, expected } of report) {
    if (count !== expected) failed = true;
    console.log(`${count === expected ? 'ok ' : 'BAD'} ${String(count).padStart(3)}/${expected}  ${label}`);
  }
  const output = Buffer.from(text, 'latin1');
  if (failed) {
    console.error('有规则的命中次数与预期不符，没有写文件');
    process.exitCode = 1;
  } else if (existsSync(BIOS_SOURCE) && !readFileSync(BIOS_SOURCE).equals(output) && !process.argv.includes('--force')) {
    console.error(`${BIOS_SOURCE} 与生成结果不同，写入会覆盖手工修改；确认要覆盖请加 --force`);
    process.exitCode = 1;
  } else {
    writeFileSync(BIOS_SOURCE, output);
    console.log(`已写入 ${BIOS_SOURCE}`);
  }
}
