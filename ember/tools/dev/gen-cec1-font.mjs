// 生成中华学习机的内置汉字字库：node gen-cec1-font.mjs <unifont_all.hex.gz> <输出目录>
// 字库布局按汉字程序的寻址：字模 N 占 32 字节，N 从 0 起：
//   N 为 ASCII 码（$20-$7E）；N = 256 + 94 × 区序号 + 位 - 1，
//   区 1 到 9 的区序号是 0 到 8，区 16 到 87 的是 9 到 80；
//   每个字模 16 行，每行 2 个字节：左半、右半，字节的位 0 在最左；ASCII 只用左半。
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';

const [hexPath, outDir] = process.argv.slice(2);
if (!hexPath || !outDir) {
  console.error('用法：node gen-cec1-font.mjs <unifont_all.hex.gz> <输出目录>');
  process.exit(1);
}

const GLYPH_BYTES = 32;
const GLYPH_COUNT = 8192;
const FIRST_HANZI_GLYPH = 256;
const AREA_POSITIONS = 94;

const hexText = zlib.gunzipSync(fs.readFileSync(hexPath)).toString('utf8');
const glyphs = new Map();
for (const line of hexText.split('\n')) {
  const split = line.indexOf(':');
  if (split <= 0) continue;
  glyphs.set(parseInt(line.slice(0, split), 16), line.slice(split + 1).trim());
}

const reverseBits = (value) => {
  let result = 0;
  for (let bit = 0; bit < 8; bit += 1) if ((value >> bit) & 1) result |= 0x80 >> bit;
  return result;
};

const font = new Uint8Array(GLYPH_COUNT * GLYPH_BYTES);
const missing = [];

// 把 Unifont 的一个字形（32 或 64 个十六进制字符，每行高位在左）写进字模 index。
const putGlyph = (index, hex) => {
  const wide = hex.length === 64;
  for (let row = 0; row < 16; row += 1) {
    const left = parseInt(hex.slice(row * (wide ? 4 : 2), row * (wide ? 4 : 2) + 2), 16);
    font[index * GLYPH_BYTES + row * 2] = reverseBits(left);
    if (wide) font[index * GLYPH_BYTES + row * 2 + 1] = reverseBits(parseInt(hex.slice(row * 4 + 2, row * 4 + 4), 16));
  }
};

for (let code = 0x20; code <= 0x7e; code += 1) {
  const hex = glyphs.get(code);
  if (hex === undefined || hex.length !== 32) missing.push(`ASCII ${code.toString(16)}`);
  else putGlyph(code, hex);
}

const decoder = new TextDecoder('gb2312');
let hanziCount = 0;
for (let area = 1; area <= 87; area += 1) {
  if (area > 9 && area < 16) continue;
  const areaIndex = area <= 9 ? area - 1 : area - 7;
  for (let position = 1; position <= AREA_POSITIONS; position += 1) {
    const text = decoder.decode(Uint8Array.from([0xa0 + area, 0xa0 + position]));
    const codePoint = text.codePointAt(0);
    const hex = codePoint === undefined || text.length === 0 || codePoint === 0xfffd ? undefined : glyphs.get(codePoint);
    if (hex === undefined) {
      missing.push(`区位 ${area}-${position}`);
      continue;
    }
    putGlyph(FIRST_HANZI_GLYPH + areaIndex * AREA_POSITIONS + position - 1, hex);
    hanziCount += 1;
  }
}
console.log(`汉字与符号 ${hanziCount} 个，缺字 ${missing.length} 个`);
if (missing.length > 0) console.log(missing.slice(0, 20).join('、'));
// 开机封面用区位 2-1 当空白填充，所以这个字模留空。
font.fill(0, (FIRST_HANZI_GLYPH + AREA_POSITIONS) * GLYPH_BYTES, (FIRST_HANZI_GLYPH + AREA_POSITIONS + 1) * GLYPH_BYTES);

const header = `/*
 * ------------------------------------------------------------------------
 * 名称：cec1-font.hpp
 * 说明：中华学习机 CEC-I 的内置汉字字库声明：没有接 U33、U34 字库芯片时汉字程序读这份字模。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：数据在 cec1-font.cpp，由 tools/dev/gen-cec1-font.mjs 从 GNU Unifont 的点阵生成，不手改；
 *       布局与汉字程序的寻址一致：字模序号 N 占 32 字节（16 行，每行左半、右半各一个字节，
 *       字节的位 0 在最左），序号 N 为 ASCII 码（$20-$7E），或 256 + 94 × 区序号 + 位 - 1
 *       （区 1 到 9 的区序号是 0 到 8，区 16 到 87 的是 9 到 80）；ASCII 只用左半；
 *       数组下标是 N × 32 + 字节号，就是汉字程序经 $4000-$7FFF 读到的内容
 *       （位序已是 CPU 看到的）。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace ember::bootstrap::cec1_font {

/** 字库的大小：16 个 16K 的页。 */
inline constexpr std::size_t FONT_SIZE = 256 * 1024;

extern const std::array<std::uint8_t, FONT_SIZE> HANZI_FONT;

}  // namespace ember::bootstrap::cec1_font
`;

const lines = [];
for (let offset = 0; offset < font.length; offset += 32) {
  lines.push('    ' + Array.from(font.subarray(offset, offset + 32), (value) => `0x${value.toString(16).padStart(2, '0')}`).join(', ') + ',');
}
const source = `/*
 * ------------------------------------------------------------------------
 * 名称：cec1-font.cpp
 * 说明：中华学习机 CEC-I 的内置汉字字库数据（GNU Unifont 的点阵，SIL OFL 1.1）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：由 tools/dev/gen-cec1-font.mjs 生成，不手改，布局见 cec1-font.hpp。
 * ------------------------------------------------------------------------
 */

#include "bootstrap/machines/cec1/cec1-font.hpp"

namespace ember::bootstrap::cec1_font {

const std::array<std::uint8_t, FONT_SIZE> HANZI_FONT = {
${lines.join('\n')}
};

}  // namespace ember::bootstrap::cec1_font
`;
fs.writeFileSync(path.join(outDir, 'cec1-font.hpp'), header);
fs.writeFileSync(path.join(outDir, 'cec1-font.cpp'), source);
console.log('已写入', outDir);
