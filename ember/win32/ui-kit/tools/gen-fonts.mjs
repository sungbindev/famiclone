/*
 * ------------------------------------------------------------------------
 * 名称：gen-fonts.mjs
 * 说明：字体生成器：把组件库里的 Quicksand 字体（WOFF2，标题栏应用名用）解码成 TrueType，生成 C++ 字节数组 font-data.generated.cpp，供 DirectWrite 从内存加载。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：DirectWrite 读不了 WOFF2，所以由 tools/lib/woff2.mjs 还原成 TrueType 再嵌入程序；字体来自 rujian-ui-kit 的 src/third_party/quicksand（SIL OFL 1.1），许可与清单在 third_party/quicksand。
 *       用法：node tools/gen-fonts.mjs 写文件，node tools/gen-fonts.mjs --check 只检查已提交的文件与生成结果一致。
 * ------------------------------------------------------------------------
 */

import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { findWebKit, KIT_ROOT } from './lib/kit-source.mjs';
import { decodeWoff2 } from './lib/woff2.mjs';

/** 生成的字节数组文件。 */
export const FONT_DATA_FILE = join(KIT_ROOT, 'src', 'generated', 'font-data.generated.cpp');

/** 要嵌入的字体：源文件名与数组名。 */
const FONTS = [
  { source: 'quicksand-latin-500.woff2', name: 'kQuicksand500' },
  { source: 'quicksand-latin-700.woff2', name: 'kQuicksand700' }
];

/** 每行写多少个字节。 */
const BYTES_PER_LINE = 16;

/**
 * 生成字体数据文件的内容。
 * @returns {string} 文件内容。
 */
export function generate() {
  const directory = join(findWebKit(), 'src', 'third_party', 'quicksand');
  const lines = [
    '/*',
    ' * ------------------------------------------------------------------------',
    ' * 名称：font-data.generated.cpp',
    ' * 说明：嵌入的字体数据：标题栏应用名用的 Quicksand（TrueType 字节），按 FONTS 的顺序排列。',
    ' * 作者：sungbin',
    ' * 邮箱：sungbin@3578.cn',
    ' * 日期：2026-11-19',
    ' * 备注：由 tools/gen-fonts.mjs 生成，不要手改；字体为 Quicksand（SIL OFL 1.1，见 third_party/quicksand）。',
    ' * ------------------------------------------------------------------------',
    ' */',
    '',
    '#include "base/font-data.hpp"',
    '',
    'namespace rujian::ui {',
    '',
    'namespace {',
    ''
  ];
  for (const font of FONTS) {
    const ttf = decodeWoff2(readFileSync(join(directory, font.source)));
    lines.push(`constexpr unsigned char ${font.name}[] = {`);
    for (let at = 0; at < ttf.length; at += BYTES_PER_LINE) {
      const row = [...ttf.subarray(at, at + BYTES_PER_LINE)].map((byte) => `0x${byte.toString(16).padStart(2, '0')}`);
      lines.push(`    ${row.join(', ')},`);
    }
    lines.push('};', '');
  }
  lines.push(
    '}  // namespace',
    '',
    'const std::vector<EmbeddedFont>& embeddedFonts() {',
    '  static const std::vector<EmbeddedFont> fonts = {',
    ...FONTS.map((font) => `      {${font.name}, sizeof(${font.name})},`),
    '  };',
    '  return fonts;',
    '}',
    '',
    '}  // namespace rujian::ui',
    ''
  );
  return lines.join('\n');
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  try {
    const text = generate();
    if (process.argv.includes('--check')) {
      if (!existsSync(FONT_DATA_FILE) || readFileSync(FONT_DATA_FILE, 'utf8') !== text) throw new Error('字体文件与生成结果不一致，运行 node tools/gen-fonts.mjs 重新生成');
      console.log('字体文件与生成结果一致');
    } else {
      writeFileSync(FONT_DATA_FILE, text);
      console.log(`已生成 ${FONTS.length} 个字体`);
    }
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exit(1);
  }
}
