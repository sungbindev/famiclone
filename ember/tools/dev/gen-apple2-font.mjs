/*
 * ------------------------------------------------------------------------
 * 名称：gen-apple2-font.mjs
 * 说明：从苹果 IIe 增强版的视频 ROM 生成文本屏的字模源文件：node gen-apple2-font.mjs
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：输入是 reference/study_roms/apple2/Apple2e_Enhanced_Video.rom（4KB，位 0 在最左）；
 *       输出每个字符 8 行，每行低 7 位，位 0 在最左，写入 core/src/logic/hardware/video-apple2/apple2-font.cpp。
 * ------------------------------------------------------------------------
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const GLYPH_HEIGHT = 8;
const FIRST_CODE = 0x20;
const GLYPH_COUNT = 96;

const here = path.dirname(fileURLToPath(import.meta.url));
const romPath = path.resolve(here, '../../../reference/study_roms/apple2/Apple2e_Enhanced_Video.rom');
const rom = fs.readFileSync(romPath);
if (rom.length !== 4096) throw new Error('视频 ROM 的大小不对');

// ROM 里字符 $00-$3F 是 @、A-Z、符号与数字（对应字符码 $40-$7F 的前半与 $20-$3F），$60-$7F 是小写字母与符号。
const glyph = (code) => {
  const index = code >= 0x40 && code < 0x60 ? code - 0x40 : code;
  return Array.from({ length: GLYPH_HEIGHT }, (_, row) => rom[index * GLYPH_HEIGHT + row] & 0x7f);
};

const NAMES = [
  'space', '!', '"', '#', '$', '%', '&', "'", '(', ')', '*', '+', ',', '-', '.', '/',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
  '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', 'backslash', ']', '^', '_',
  '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', 'DEL',
];
if (NAMES.length !== GLYPH_COUNT) throw new Error(`名称数量不对：${NAMES.length}`);

const lines = NAMES.map((name, index) => {
  const code = FIRST_CODE + index;
  const bytes = glyph(code);
  const hex = bytes.map((byte) => `0x${byte.toString(16).toUpperCase().padStart(2, '0')}`).join(', ');
  return `    ${hex},  // $${code.toString(16).toUpperCase()} ${name}`;
});

const source = `/*
 * ------------------------------------------------------------------------
 * 名称：apple2-font.cpp
 * 说明：苹果 II 类文本屏的字模数据：ASCII $20 到 $7F 共 96 个字符。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：由 tools/dev/gen-apple2-font.mjs 生成，不手改；字形取自苹果 IIe 增强版的视频 ROM
 *       （AppleWin 附带的 Apple2e_Enhanced_Video.rom，原文件在 reference/study_roms/apple2）；
 *       每行只用低 7 位，位 0 在最左（5×7 点阵在第 1 到 5 列）。
 *       中华学习机的显示码 ROM 没有收集到，文本屏用这套字模代替。
 * ------------------------------------------------------------------------
 */

#include "logic/hardware/video-apple2/apple2-font.hpp"

namespace ember::logic::hardware::apple2 {

const std::array<std::uint8_t, FONT_GLYPH_COUNT * FONT_GLYPH_HEIGHT> TEXT_FONT = {
${lines.join('\n')}
};

}  // namespace ember::logic::hardware::apple2
`;

const target = path.resolve(here, '../../core/src/logic/hardware/video-apple2/apple2-font.cpp');
fs.writeFileSync(target, source, 'utf8');
console.log(`已写入 ${target}`);
