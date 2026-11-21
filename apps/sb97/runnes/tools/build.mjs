// ------------------------------------------------------------------------
// 名称：build.mjs
// 说明：构建 sb97 的 RUNNES.EXE：汇编、链接成 32K 程序映像，并可把它与游戏文件一起打包成 1.44MB 的软盘镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用法：node tools/build.mjs [--disk 游戏文件.NES ...]；未给 --disk 时只生成 build/sb97/RUNNES.img（只放程序）。
//       汇编器与链接器用仓库里 fcdev-cc65 随包的 cc65，镜像打包用 fcdev 的 fat-floppy 打包器。
// ------------------------------------------------------------------------
import { execFileSync } from 'node:child_process';
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { basename, dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const projectDir = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const repoDir = resolve(projectDir, '..', '..', '..');
const cc65Bin = join(repoDir, 'packages', 'fcdev-cc65', 'bin');
const srcDir = join(projectDir, 'src');
const outDir = join(projectDir, 'build', 'sb97');
const sources = ['runnes.s', 'ramcode.s', 'fdc.s', 'fat.s', 'screen.s'];

const args = process.argv.slice(2);
const diskIndex = args.indexOf('--disk');
const gameFiles = diskIndex >= 0 ? args.slice(diskIndex + 1) : [];

mkdirSync(outDir, { recursive: true });
const objects = sources.map((source) => {
  const obj = join(outDir, `${basename(source, '.s')}.o`);
  execFileSync(join(cc65Bin, 'ca65.exe'), ['-I', srcDir, '-o', obj, join(srcDir, source)], { stdio: 'inherit' });
  return obj;
});

const exe = join(outDir, 'RUNNES.EXE');
execFileSync(join(cc65Bin, 'ld65.exe'), ['-C', join(projectDir, 'runnes.cfg'), '-o', exe, ...objects], { stdio: 'inherit' });
const data = readFileSync(exe);
if (data.length !== 32768) {
  throw new Error(`RUNNES.EXE 大小应为 32768 字节，实际 ${data.length}`);
}
console.log(`built ${exe} (${data.length} bytes)`);

const { packFatFloppy } = await import(pathToFileURL(join(repoDir, 'packages', 'fcdev', 'src', 'packagers', 'fat-floppy.mjs')).href);
const files = [{ name: 'RUNNES.EXE', data }];
for (const file of gameFiles) {
  files.push({ name: basename(file).toUpperCase(), data: readFileSync(file) });
}
const image = join(outDir, 'RUNNES.img');
packFatFloppy({ files, outputFile: image, options: { label: 'RUNNES' } });
writeFileSync(join(outDir, 'files.txt'), files.map((file) => `${file.name} ${file.data.length}`).join('\n') + '\n');
console.log(`image ${image} (${files.map((file) => file.name).join(', ')})`);
