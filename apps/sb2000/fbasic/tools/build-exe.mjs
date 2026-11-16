// ------------------------------------------------------------------------
// 名称：build-exe.mjs
// 说明：构建 FBASIC.EXE：先汇编链接 ERAM 小段（build/stub.bin）、32K 映像（build/image.bin）与内置程序 GAME 0-3 的数据（build/game.bin，三页），再用 fcdev 构建外壳程序（exe/）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/build-exe.mjs；映像源码是 image/fbasic.s（由原版 reference/fbasic-nes.s 复制后手工维护）。
//       fcdev 的多页模板把库页固定链接在 $A000，放不下 $8000 起的整机映像，所以映像单独链接，再由 exe/ 里的加载器用 .incbin 放进程序文件。
//       输出 exe/build/sb2000/FBASIC.EXE。
// ------------------------------------------------------------------------
import { spawnSync } from 'node:child_process';
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { locateToolchain, runTool } from '../../../../packages/fcdev/src/toolchain/index.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const REPOSITORY = resolve(ROOT, '..', '..', '..');
const BUILD = join(ROOT, 'build');
const INCLUDES = ['-I', join(REPOSITORY, 'packages', 'fcdev', 'targets', 'sb2000', 'asminc'), '-I', join(ROOT, 'image')];
const IMAGE_SIZE = 0x8000;
const STUB_LIMIT = 0x1a0;

const STUB_CONFIG = `MEMORY { STUB: file = %O, start = $7E00, size = $01A0; }
SEGMENTS { STUB: load = STUB, type = ro; }
`;
const GAME_CONFIG = `MEMORY { GAME: file = %O, start = $0000, size = $3000, fill = yes, fillval = $00; }
SEGMENTS { GAME: load = GAME, type = ro; }
`;
const PRG_CONFIG = `MEMORY {
  IMAGE:   file = %O, start = $8000, size = $7FFA, fill = yes, fillval = $00;
  VECTORS: file = %O, start = $FFFA, size = $0006;
}
SEGMENTS { CODE: load = IMAGE, type = ro; VECTORS: load = VECTORS, type = ro; }
`;

mkdirSync(BUILD, { recursive: true });
const toolchain = locateToolchain({});

/** 汇编并链接一个源文件，返回二进制的字节数 */
async function assemble(name, source, config) {
  const objectFile = join(BUILD, `${name}.o`);
  const configFile = join(BUILD, `${name}.cfg`);
  const binaryFile = join(BUILD, `${name}.bin`);
  writeFileSync(configFile, config);
  const compiled = await runTool(toolchain, 'ca65', ['--cpu', '6502', ...INCLUDES, '-g', source, '-o', objectFile]);
  if (compiled.code !== 0) { console.error(`ca65 ${source} 失败：\n${compiled.stdout}${compiled.stderr}`.trim()); process.exit(1); }
  const linked = await runTool(toolchain, 'ld65', ['-C', configFile, '-m', join(BUILD, `${name}.map`), '--dbgfile', join(BUILD, `${name}.dbg`), '-o', binaryFile, objectFile]);
  if (linked.code !== 0) { console.error(`ld65 ${name} 失败：\n${linked.stdout}${linked.stderr}`.trim()); process.exit(1); }
  return readFileSync(binaryFile).length;
}

const stubSize = await assemble('stub', join(ROOT, 'image', 'stub.s'), STUB_CONFIG);
if (stubSize <= 0x100 || stubSize > STUB_LIMIT) { console.error(`小段 ${stubSize} 字节，要在 ${0x100 + 1} 到 ${STUB_LIMIT} 之间（加载器按这个范围复制）`); process.exit(1); }
const imageSize = await assemble('image', join(ROOT, 'image', 'fbasic.s'), PRG_CONFIG);
if (imageSize !== IMAGE_SIZE) { console.error(`映像 ${imageSize} 字节，应为 ${IMAGE_SIZE}`); process.exit(1); }
console.log(`小段 ${stubSize} 字节，映像 ${imageSize} 字节`);
const gameSize = await assemble('game', join(ROOT, 'image', 'game.s'), GAME_CONFIG);
if (gameSize !== 0x3000) { console.error(`GAME 数据 ${gameSize} 字节，应为 ${0x3000}`); process.exit(1); }

const built = spawnSync(process.execPath, [join(REPOSITORY, 'packages', 'fcdev', 'bin', 'fcdev.mjs'), 'build', 'src'], { cwd: join(ROOT, 'exe'), encoding: 'utf8' });
process.stdout.write(built.stdout ?? '');
process.stderr.write(built.stderr ?? '');
process.exit(built.status ?? 1);
