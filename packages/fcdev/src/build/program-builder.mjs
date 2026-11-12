// ------------------------------------------------------------------------
// 名称：program-builder.mjs
// 说明：构建一个程序：逐个编译与汇编源文件，链接，再交给打包器；支持增量构建。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：某个目标文件比源文件和依赖文件新就不重编；目标、选项、fcdev 或工具链版本变化时全部重编（指纹存在 stamp.json）。
// ------------------------------------------------------------------------
import { createHash } from 'node:crypto';
import { readFileSync, rmSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { ensureDirectory, FcdevError, getModifiedTime, readJsonFile } from '../shared/index.mjs';
import { getPackager } from '../packagers/index.mjs';
import { resolveTargetPath } from '../targets/index.mjs';
import { getToolchainVersion, runTool } from '../toolchain/index.mjs';
import { resolveLinkSetup } from './bank-link-config.mjs';
import { parseDependencyFile } from './dependency-file.mjs';
import { getSourceLanguage } from './source-collector.mjs';
import { buildAssemblerArguments, buildCompilerArguments, buildLinkerArguments } from './tool-arguments.mjs';

/** 主包自身的 package.json，用于取版本号 */
const PACKAGE_JSON_FILE = fileURLToPath(new URL('../../package.json', import.meta.url));

/**
 * @typedef {object} Reporter
 * @property {(message: string) => void} log 进度信息
 * @property {(text: string) => void} write 工具的原始输出，原样透传
 */

/**
 * @typedef {object} BuildResult
 * @property {string} programName 程序名
 * @property {string} outputFile 产物路径
 * @property {number} size 产物大小
 * @property {number} compiled 本次重新编译的源文件数
 * @property {number} reused 沿用上次结果的源文件数
 */

/** 不输出任何信息的汇报器 */
const SILENT_REPORTER = { log() {}, write() {} };

/**
 * 计算增量构建的指纹。
 * @param {import('./build-plan.mjs').BuildPlan} plan 构建计划
 * @param {string} toolchainVersion 工具链版本
 * @returns {string} 指纹
 */
function computeFingerprint(plan, toolchainVersion) {
  const fcdevVersion = readJsonFile(PACKAGE_JSON_FILE).version;
  const content = {
    fcdevVersion,
    toolchainVersion,
    target: plan.target,
    targetOptions: plan.targetOptions,
    optimize: plan.config.optimize,
    define: plan.config.define,
    include: plan.config.include,
    sources: plan.sources.map((source) => path.relative(plan.sourceBase, source)),
  };
  return createHash('sha256').update(JSON.stringify(content)).digest('hex');
}

/**
 * 读取上次构建记录的指纹。
 * @param {string} stampFile 指纹文件
 * @returns {string|null} 指纹；没有记录或文件损坏时为 null
 */
function readStamp(stampFile) {
  try {
    return JSON.parse(readFileSync(stampFile, 'utf8')).fingerprint ?? null;
  } catch {
    return null;
  }
}

/**
 * 判断目标文件是否比源文件和依赖文件都新。
 * @param {string} objectFile 目标文件
 * @param {string} sourceFile 源文件
 * @param {string} dependencyFile 依赖文件
 * @returns {boolean} 不需要重编时为 true
 */
function isUpToDate(objectFile, sourceFile, dependencyFile) {
  const objectTime = getModifiedTime(objectFile);
  const dependencyText = getModifiedTime(dependencyFile) === null ? null : readFileSync(dependencyFile, 'utf8');
  if (objectTime === null || dependencyText === null) return false;
  const inputs = [sourceFile, ...parseDependencyFile(dependencyText)];
  return inputs.every((input) => {
    const time = getModifiedTime(input);
    return time !== null && time <= objectTime;
  });
}

/**
 * 运行一个工具；失败时透传输出并抛错。
 * @param {import('../toolchain/index.mjs').Toolchain} toolchain 工具链
 * @param {string} tool 工具名
 * @param {string[]} args 参数
 * @param {Reporter} reporter 汇报器
 * @param {string} failureMessage 失败时的说明
 * @returns {Promise<import('../shared/index.mjs').ProcessResult>} 运行结果
 * @throws {FcdevError} 工具以非零退出码结束
 */
async function runChecked(toolchain, tool, args, reporter, failureMessage) {
  const result = await runTool(toolchain, tool, args);
  const text = `${result.stdout}${result.stderr}`;
  if (text !== '') reporter.write(text.endsWith('\n') ? text : `${text}\n`);
  if (result.code !== 0) throw new FcdevError(failureMessage, { code: 'TOOL_FAILED', hint: describeToolFailure(text) });
  return result;
}

/**
 * 根据链接器的输出给出常见问题的提示。
 * @param {string} output 工具输出
 * @returns {string} 提示；没有可提示的内容时为空
 */
function describeToolFailure(output) {
  if (/Unresolved external[^\w]*_?main\b/i.test(output)) return '没有找到 main：C 程序要定义 main，汇编程序要 .export _main';
  if (/Duplicate external identifier[^\w]*_?main\b/i.test(output)) return '目录里有多个 main；若要单独构建一个程序，请指定文件名';
  if (/Missing memory area assignment for segment .BANK\d+/.test(output)) return '库页个数不够：在 fcdev.json 的 targetOptions.banks 里加大库页个数';
  return '';
}

/**
 * 编译或汇编一个源文件，得到目标文件。
 * @param {object} context 构建上下文
 * @param {string} source 源文件
 * @param {string} objectFile 目标文件
 * @returns {Promise<void>}
 */
async function compileSource(context, source, objectFile) {
  const { toolchain, settings, reporter } = context;
  const base = objectFile.slice(0, -'.o'.length);
  if (getSourceLanguage(source) === 'c') {
    const asmFile = `${base}.s`;
    await runChecked(toolchain, 'cc65', buildCompilerArguments(settings, { source, output: asmFile, dependencyFile: `${base}.d` }), reporter, `编译失败：${source}`);
    await runChecked(toolchain, 'ca65', buildAssemblerArguments(settings, { source: asmFile, output: objectFile, dependencyFile: `${base}.asm.d` }), reporter, `汇编失败：${asmFile}`);
  } else {
    await runChecked(toolchain, 'ca65', buildAssemblerArguments(settings, { source, output: objectFile, dependencyFile: `${base}.d` }), reporter, `汇编失败：${source}`);
  }
}

/**
 * 构建一个程序。
 * @param {import('./build-plan.mjs').BuildPlan} plan 构建计划
 * @param {import('../toolchain/index.mjs').Toolchain} toolchain 工具链
 * @param {Reporter} [reporter] 汇报器
 * @returns {Promise<BuildResult>} 构建结果
 * @throws {FcdevError} 编译、链接或打包失败
 */
export async function buildProgram(plan, toolchain, reporter = SILENT_REPORTER) {
  const { target, config } = plan;
  const toTargetPaths = (values) => values.map((value) => resolveTargetPath(target, value, toolchain.home));
  const settings = {
    cc65Target: target.cc65Target,
    cpu: target.cpu,
    optimize: config.optimize,
    includeDirs: [...toTargetPaths(target.include), ...config.include, plan.sourceBase],
    asmIncludeDirs: [...toTargetPaths(target.asminclude), ...config.include, plan.sourceBase],
    defines: { ...target.defines, ...config.define },
  };

  ensureDirectory(plan.objectDir);
  ensureDirectory(path.dirname(plan.outputFile));

  const stampFile = path.join(plan.objectDir, 'stamp.json');
  const fingerprint = computeFingerprint(plan, await getToolchainVersion(toolchain));
  if (readStamp(stampFile) !== fingerprint) {
    // 指纹变了：删掉旧的中间文件再重编
    rmSync(plan.objectDir, { recursive: true, force: true });
    ensureDirectory(plan.objectDir);
  }

  const context = { toolchain, settings, reporter };
  const objects = [];
  let compiled = 0;
  let reused = 0;
  for (const source of plan.sources) {
    const relative = path.relative(plan.sourceBase, source);
    const objectName = `${relative.slice(0, relative.length - path.extname(relative).length).split(path.sep).join('__')}.o`;
    const objectFile = path.join(plan.objectDir, objectName);
    objects.push(objectFile);
    if (isUpToDate(objectFile, source, `${objectFile.slice(0, -2)}.d`)) {
      reused += 1;
      continue;
    }
    reporter.log(`编译 ${relative}`);
    await compileSource(context, source, objectFile);
    compiled += 1;
  }

  const binaryFile = path.join(plan.objectDir, `${plan.programName}.bin`);
  const optionObjects = Object.entries(plan.targetOptions)
    .filter(([key, value]) => value === true && target.options[key])
    .flatMap(([key]) => target.options[key].links);
  reporter.log('链接');
  const linkSetup = resolveLinkSetup(plan, toolchain.home);
  await runChecked(toolchain, 'ld65', buildLinkerArguments({
    linkConfig: linkSetup.linkConfig,
    binaryFile,
    mapFile: path.join(plan.objectDir, `${plan.programName}.map`),
    debugFile: path.join(plan.objectDir, `${plan.programName}.dbg`),
    objects: [...toTargetPaths(target.startup), ...objects, ...toTargetPaths(optionObjects)],
    libraries: toTargetPaths(target.libraries),
    forceImports: target.forceImports,
  }), reporter, `链接失败：${plan.programName}`);

  const packaged = getPackager(target.packager)({ binaryFile, outputFile: plan.outputFile, options: linkSetup.packagerOptions });
  writeFileSync(stampFile, JSON.stringify({ fingerprint }));
  return { programName: plan.programName, outputFile: packaged.outputFile, size: packaged.size, compiled, reused };
}
