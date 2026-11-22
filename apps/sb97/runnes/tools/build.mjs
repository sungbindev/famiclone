// 构建 NES.EXE 并生成测试软盘。用法：node tools/build.mjs [游戏文件.nes]
// 汇编与链接用仓库里的 cc65（build/cc65/bin）；没有给游戏文件时用 reference/mario.nes。
import { execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, statSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const famiclone = resolve(root, '..', '..', '..');
const cc65 = join(famiclone, 'build', 'cc65', 'bin');
const out = join(root, 'build');
const src = join(root, 'src', 'nes.s');
const cfg = join(root, 'src', 'sb97-exe.cfg');
const obj = join(out, 'nes.o');
const exe = join(out, 'NES.EXE');
const disk = join(out, 'RUNNES.ima');
const game = resolve(process.argv[2] ?? join(famiclone, 'reference', 'mario.nes'));

mkdirSync(out, { recursive: true });
execFileSync(join(cc65, 'ca65.exe'), ['--cpu', '6502', '-o', obj, src], { stdio: 'inherit' });
execFileSync(join(cc65, 'ld65.exe'), ['-C', cfg, '-m', join(out, 'NES.map'), '-o', exe, obj], { stdio: 'inherit' });

const size = statSync(exe).size;
if (size !== 32768) {
  throw new Error(`NES.EXE 应为 32768 字节，实际 ${size}`);
}
if (!existsSync(game)) {
  throw new Error(`找不到游戏文件：${game}`);
}
execFileSync(process.execPath, [join(root, 'tools', 'make-disk.mjs'), disk, exe, game], { stdio: 'inherit' });
console.log(`已生成 ${exe} 与 ${disk}`);
