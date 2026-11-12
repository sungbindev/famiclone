// ------------------------------------------------------------------------
// 名称：build-libs.mjs
// 说明：编译各目标的运行库与启动代码：按 targets/<名>/lib-build.json 编译 C 与汇编源文件，生成 lib/ 下的 .o 与 .lib。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用法 node tools/build-libs.mjs [目标名…]；需要 cc65 工具链（FCDEV_CC65_HOME 或工具链包）。发布前由 prepack 调用，测试前由 pretest 调用。
//       lib-build.json 字段：libraries（库路径到源文件列表）、objects（目标文件路径到源文件）、binIncludeDirs（.incbin 的搜索目录）。
//       源文件列表里以“/”结尾的项是目录，递归收录其中全部 .c 与 .s 文件；.c 先用 cc65 编成汇编再汇编。库里每个源文件是一个目标文件，链接时只带用到的。
// ------------------------------------------------------------------------
import { existsSync, mkdirSync, readdirSync, readFileSync, rmSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { FcdevError } from '../packages/fcdev/src/shared/index.mjs';
import { listTargetNames, loadTarget, resolveTargetPath } from '../packages/fcdev/src/targets/index.mjs';
import { locateToolchain, runTool } from '../packages/fcdev/src/toolchain/index.mjs';

/**
 * 展开库的源文件列表：以“/”结尾的项按目录递归收录 .c 与 .s 文件。
 * @param {string} targetDir 目标目录
 * @param {string[]} entries lib-build.json 里的源文件项（相对目标目录）
 * @returns {string[]} 源文件的相对路径，目录内按路径排序
 */
function expandSources(targetDir, entries) {
  const sources = [];
  for (const entry of entries) {
    if (!entry.endsWith('/')) {
      sources.push(entry);
      continue;
    }
    const found = readdirSync(path.join(targetDir, entry), { recursive: true, withFileTypes: true })
      .filter((item) => item.isFile() && /\.(c|s)$/.test(item.name))
      .map((item) => path.relative(targetDir, path.join(item.parentPath, item.name)).split(path.sep).join('/'))
      .sort();
    sources.push(...found);
  }
  return sources;
}

/**
 * 编译一个 C 源文件：cc65 编成汇编，再汇编成目标文件。
 * @param {import('../packages/fcdev/src/toolchain/index.mjs').Toolchain} toolchain 工具链
 * @param {import('../packages/fcdev/src/targets/index.mjs').Target} target 目标
 * @param {{binIncludeDirs?: string[]}} spec lib-build.json 的内容
 * @param {string} source 源文件
 * @param {string} output 目标文件
 * @returns {Promise<void>}
 * @throws {FcdevError} 编译或汇编失败
 */
async function compileC(toolchain, target, spec, source, output) {
  const asmFile = output.replace(/\.o$/, '.s');
  const args = [
    '-t', target.cc65Target,
    '--cpu', target.cpu,
    '-Oirs',
    ...target.include.flatMap((value) => ['-I', resolveTargetPath(target, value, toolchain.home)]),
    ...Object.entries(target.defines).flatMap(([name, value]) => ['-D', `${name}=${value}`]),
    '-o', asmFile,
    source,
  ];
  const result = await runTool(toolchain, 'cc65', args);
  if (result.code !== 0) {
    throw new FcdevError(`编译 ${source} 失败`, { code: 'LIB_BUILD_FAILED', hint: `${result.stdout}${result.stderr}`.trim() });
  }
  try {
    await assemble(toolchain, target, spec, asmFile, output);
  } finally {
    rmSync(asmFile, { force: true });
  }
}

/**
 * 汇编一个源文件。
 * @param {import('../packages/fcdev/src/toolchain/index.mjs').Toolchain} toolchain 工具链
 * @param {import('../packages/fcdev/src/targets/index.mjs').Target} target 目标
 * @param {{binIncludeDirs?: string[]}} spec lib-build.json 的内容
 * @param {string} source 源文件
 * @param {string} output 目标文件
 * @returns {Promise<void>}
 * @throws {FcdevError} 汇编失败
 */
async function assemble(toolchain, target, spec, source, output) {
  const args = [
    '-t', target.cc65Target,
    '--cpu', target.cpu,
    '-I', path.join(target.dir, 'asminc'),
    ...(spec.binIncludeDirs ?? []).flatMap((dir) => ['--bin-include-dir', path.join(target.dir, dir)]),
    '-o', output,
    source,
  ];
  const result = await runTool(toolchain, 'ca65', args);
  if (result.code !== 0) {
    throw new FcdevError(`汇编 ${source} 失败`, { code: 'LIB_BUILD_FAILED', hint: `${result.stdout}${result.stderr}`.trim() });
  }
}

/**
 * 编译一个目标的运行库。
 * @param {import('../packages/fcdev/src/toolchain/index.mjs').Toolchain} toolchain 工具链
 * @param {import('../packages/fcdev/src/targets/index.mjs').Target} target 目标
 * @returns {Promise<string[]>} 生成的文件（相对目标目录）；目标没有 lib-build.json 时为空
 */
export async function buildTargetLibraries(toolchain, target) {
  const specFile = path.join(target.dir, 'lib-build.json');
  if (!existsSync(specFile)) return [];
  const spec = JSON.parse(readFileSync(specFile, 'utf8'));
  const produced = [];
  mkdirSync(path.join(target.dir, 'lib'), { recursive: true });

  for (const [objectPath, sourcePath] of Object.entries(spec.objects ?? {})) {
    await assemble(toolchain, target, spec, path.join(target.dir, sourcePath), path.join(target.dir, objectPath));
    produced.push(objectPath);
  }

  for (const [libraryPath, entries] of Object.entries(spec.libraries ?? {})) {
    const libraryFile = path.join(target.dir, libraryPath);
    rmSync(libraryFile, { force: true });
    const objectFiles = [];
    for (const sourcePath of expandSources(target.dir, entries)) {
      const objectFile = path.join(target.dir, 'lib', `${path.basename(sourcePath, path.extname(sourcePath))}.o`);
      if (objectFiles.includes(objectFile)) {
        throw new FcdevError(`库 ${libraryPath} 里有同名的源文件：${sourcePath}`, { code: 'LIB_BUILD_FAILED' });
      }
      const sourceFile = path.join(target.dir, sourcePath);
      await (sourcePath.endsWith('.c') ? compileC : assemble)(toolchain, target, spec, sourceFile, objectFile);
      objectFiles.push(objectFile);
    }
    const result = await runTool(toolchain, 'ar65', ['a', libraryFile, ...objectFiles]);
    if (result.code !== 0) {
      throw new FcdevError(`生成库 ${libraryPath} 失败`, { code: 'LIB_BUILD_FAILED', hint: `${result.stdout}${result.stderr}`.trim() });
    }
    for (const objectFile of objectFiles) rmSync(objectFile, { force: true });
    produced.push(libraryPath);
  }
  return produced;
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try {
    const toolchain = locateToolchain();
    const names = process.argv.length > 2 ? process.argv.slice(2) : listTargetNames();
    for (const name of names) {
      const produced = await buildTargetLibraries(toolchain, loadTarget(name));
      console.log(`${name}：${produced.length > 0 ? produced.join('、') : '没有需要编译的库'}`);
    }
  } catch (error) {
    console.error(error.message);
    if (error.hint) console.error(error.hint);
    process.exit(1);
  }
}
