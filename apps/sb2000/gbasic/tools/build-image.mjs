// ------------------------------------------------------------------------
// 名称：build-image.mjs
// 说明：汇编并链接 GBASIC 映像（image/*.s）：用 image/image.cfg 链接成 32K 的 build/image.bin，窗口 0-7 整段运行。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/build-image.mjs；同时输出 build/image.map 与 build/image.dbg；fcdev 的多页模板把库页固定链接在 $A000，放不下整机映像，所以映像单独链接，再由 exe/ 里的加载器用 .incbin 放进程序文件。
// ------------------------------------------------------------------------
import { mkdirSync, readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';
import { locateToolchain, runTool } from '../../../../packages/fcdev/src/toolchain/index.mjs';
import { ROOT } from './lib/image-model.mjs';

const IMAGE_SIZE = 0x8000;
const SOURCE_DIR = join(ROOT, 'image');
const BUILD_DIR = join(ROOT, 'build');
const OBJECT_DIR = join(BUILD_DIR, 'image');
const INCLUDE_DIR = join(ROOT, '..', '..', '..', 'packages', 'fcdev', 'targets', 'sb2000', 'asminc');

mkdirSync(OBJECT_DIR, { recursive: true });
const toolchain = locateToolchain({});
const sources = readdirSync(SOURCE_DIR).filter((name) => name.endsWith('.s'));
const objects = [];
for (const name of sources) {
  const objectFile = join(OBJECT_DIR, name.replace(/\.s$/, '.o'));
  const result = await runTool(toolchain, 'ca65', ['--cpu', '6502', '-I', INCLUDE_DIR, '-I', SOURCE_DIR, '-g', join(SOURCE_DIR, name), '-o', objectFile]);
  if (result.code !== 0) { console.error(`ca65 ${name} 失败：\n${result.stdout}${result.stderr}`.trim()); process.exit(1); }
  objects.push(objectFile);
}
const binaryFile = join(BUILD_DIR, 'image.bin');
const link = await runTool(toolchain, 'ld65', ['-C', join(SOURCE_DIR, 'image.cfg'), '-m', join(BUILD_DIR, 'image.map'), '--dbgfile', join(BUILD_DIR, 'image.dbg'), '-o', binaryFile, ...objects]);
if (link.code !== 0) { console.error(`ld65 失败：\n${link.stdout}${link.stderr}`.trim()); process.exit(1); }
const size = readFileSync(binaryFile).length;
if (size !== IMAGE_SIZE) { console.error(`映像大小 ${size} 字节，应为 ${IMAGE_SIZE}`); process.exit(1); }
console.log(`已生成 ${binaryFile}：${size} 字节`);
