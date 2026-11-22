// ------------------------------------------------------------------------
// 名称：program-builder.ts
// 说明：构建一个程序：逐个编译与汇编源文件，链接，再交给打包器；支持增量构建。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：目标文件比源文件和依赖文件新就不重编；目标、选项、扩展或工具链版本
//       变化时全部重编（指纹存在 stamp.json）。目标声明了源文件编码时，编译的是转码后的
//       副本，工具输出里的副本路径换回原文件。
// ------------------------------------------------------------------------

import { createHash } from 'node:crypto';
import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ExtensionResources } from '../../contracts/resources/extension-resources';
import type { ProcessResult, ToolName, ToolRunner, Toolchain } from '../../contracts/toolchain/toolchain';
import type { BuildPlan, BuildReporter, BuildResult } from '../../shared/build/build-plan';
import {
  buildAssemblerArguments,
  buildCompilerArguments,
  buildLinkerArguments,
  expandBankedLinkConfig,
  getSourceLanguage,
  parseDependencyFile,
  type CompileSettings,
  type SourceLanguage
} from '../../shared/build/build-text';
import { convertToGb2312 } from '../../shared/build/source-charset';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { packageProgram, type PackagerOptions } from '../../shared/packaging/program-packager';
import { resolveTargetPath } from '../../shared/targets/target-settings';

/** 构建一个程序要用到的能力。 */
export interface BuilderDependencies {
  /** 文件系统。 */
  readonly fileSystem: FileSystem;
  /** 工具运行。 */
  readonly toolRunner: ToolRunner;
  /** 工具链。 */
  readonly toolchain: Toolchain;
  /** 扩展资源位置，取扩展版本用。 */
  readonly resources: ExtensionResources;
}

/** 不输出任何信息的汇报器。 */
const SILENT_REPORTER: BuildReporter = { log: () => undefined, write: () => undefined };

/** 构建过程共用的上下文。 */
interface CompileContext {
  readonly dependencies: BuilderDependencies;
  readonly settings: CompileSettings;
  readonly reporter: BuildReporter;
  readonly sourceCharset: BuildPlan['target']['sourceCharset'];
}

/**
 * 计算增量构建的指纹。
 * @param plan 构建计划。
 * @param extensionVersion 扩展版本。
 * @param toolchainVersion 工具链版本。
 * @returns 指纹。
 */
function computeFingerprint(plan: BuildPlan, extensionVersion: string, toolchainVersion: string): string {
  const content = {
    extensionVersion,
    toolchainVersion,
    target: plan.target,
    targetOptions: plan.targetOptions,
    optimize: plan.config.optimize,
    define: plan.config.define,
    include: plan.config.include,
    sources: plan.sources.map((source) => path.relative(plan.sourceBase, source))
  };
  return createHash('sha256').update(JSON.stringify(content)).digest('hex');
}

/**
 * 读取上次构建记录的指纹。
 * @param fileSystem 文件系统。
 * @param stampFile 指纹文件。
 * @returns 指纹；没有记录或文件损坏时为 undefined。
 */
function readStamp(fileSystem: FileSystem, stampFile: string): string | undefined {
  if (fileSystem.kindOf(stampFile) !== 'file') return undefined;
  try {
    const stamp = JSON.parse(fileSystem.readText(stampFile)) as { fingerprint?: unknown };
    return typeof stamp.fingerprint === 'string' ? stamp.fingerprint : undefined;
  } catch {
    return undefined;
  }
}

/**
 * 判断目标文件是否比源文件和依赖文件都新。
 * @param fileSystem 文件系统。
 * @param objectFile 目标文件。
 * @param sourceFile 源文件。
 * @param dependencyFile 依赖文件。
 * @returns 不需要重编时为 true。
 */
function isUpToDate(fileSystem: FileSystem, objectFile: string, sourceFile: string, dependencyFile: string): boolean {
  const objectTime = fileSystem.modifiedTime(objectFile);
  if (objectTime === undefined || fileSystem.kindOf(dependencyFile) !== 'file') return false;
  const inputs = [sourceFile, ...parseDependencyFile(fileSystem.readText(dependencyFile))];
  return inputs.every((input) => {
    const time = fileSystem.modifiedTime(input);
    return time !== undefined && time <= objectTime;
  });
}

/**
 * 根据链接器的输出给出常见问题的提示。
 * @param output 工具输出。
 * @returns 提示；没有可提示的内容时为空。
 */
function describeToolFailure(output: string): string {
  if (/Unresolved external[^\w]*_?main\b/i.test(output)) return '没有找到 main：C 程序要定义 main，汇编程序要 .export _main';
  if (/Duplicate external identifier[^\w]*_?main\b/i.test(output)) return '目录里有多个 main；若要单独构建一个程序，请指定文件名';
  if (/Missing memory area assignment for segment .BANK\d+/.test(output)) return '库页个数不够：在 fcdev.json 的 targetOptions.banks 里加大库页个数';
  return '';
}

/**
 * 运行一个工具；失败时透传输出并抛错。
 * @param dependencies 构建能力。
 * @param tool 工具名。
 * @param args 参数。
 * @param reporter 汇报器。
 * @param failureMessage 失败时的说明。
 * @returns 运行结果。
 * @throws FcdevError 工具以非零退出码结束。
 */
async function runChecked(
  dependencies: BuilderDependencies,
  tool: ToolName,
  args: readonly string[],
  reporter: BuildReporter,
  failureMessage: string
): Promise<ProcessResult> {
  const result = await dependencies.toolRunner.run(dependencies.toolchain, tool, args);
  const text = `${result.stdout}${result.stderr}`;
  if (text !== '') reporter.write(text.endsWith('\n') ? text : `${text}\n`);
  if (result.code !== 0) throw new FcdevError(failureMessage, { code: 'TOOL_FAILED', hint: describeToolFailure(text) });
  return result;
}

/**
 * 为一个源文件准备转码后的副本。
 * @param fileSystem 文件系统。
 * @param input 源文件、中间文件的公共前缀（不含扩展名）与语言。
 * @returns 副本路径，以及把工具输出里的副本路径换回原文件的函数。
 * @throws FcdevError 有字符不在 GB2312 里。
 */
function prepareGb2312Source(
  fileSystem: FileSystem,
  input: { readonly source: string; readonly base: string; readonly language: SourceLanguage }
): { file: string; restoreText: (text: string) => string } {
  const { source, base, language } = input;
  const file = `${base}.gb2312${path.extname(source)}`;
  const converted = convertToGb2312(fileSystem.readBytes(source), source);
  if (language === 'c') {
    // C 副本开头加 #line，让错误信息与调试信息仍指向原文件
    const original = source.replaceAll('\\', '\\\\').replaceAll('"', '\\"');
    fileSystem.writeBytes(file, Buffer.concat([Buffer.from(`#line 1 "${original}"\n`, 'utf8'), converted]));
    return { file, restoreText: (text) => text };
  }
  fileSystem.writeBytes(file, converted);
  return { file, restoreText: (text) => text.replaceAll(file, source) };
}

/**
 * 编译或汇编一个源文件，得到目标文件。
 * @param context 构建上下文。
 * @param source 源文件。
 * @param objectFile 目标文件。
 */
async function compileSource(context: CompileContext, source: string, objectFile: string): Promise<void> {
  const { dependencies, settings, sourceCharset } = context;
  const base = objectFile.slice(0, -'.o'.length);
  const language = getSourceLanguage(source) ?? 'asm';
  const prepared = sourceCharset === null
    ? { file: source, restoreText: (text: string) => text }
    : prepareGb2312Source(dependencies.fileSystem, { source, base, language });
  const reporter: BuildReporter = { ...context.reporter, write: (text) => context.reporter.write(prepared.restoreText(text)) };
  if (language === 'c') {
    const asmFile = `${base}.s`;
    await runChecked(
      dependencies,
      'cc65',
      buildCompilerArguments(settings, { source: prepared.file, output: asmFile, dependencyFile: `${base}.d` }),
      reporter,
      `编译失败：${source}`
    );
    await runChecked(
      dependencies,
      'ca65',
      buildAssemblerArguments(settings, { source: asmFile, output: objectFile, dependencyFile: `${base}.asm.d` }),
      reporter,
      `汇编失败：${asmFile}`
    );
  } else {
    await runChecked(
      dependencies,
      'ca65',
      buildAssemblerArguments(settings, { source: prepared.file, output: objectFile, dependencyFile: `${base}.d` }),
      reporter,
      `汇编失败：${source}`
    );
  }
}

/**
 * 确定链接用的配置文件与打包选项：多页程序展开模板并写到中间文件目录，
 * 普通程序直接用目标的链接配置。
 * @param plan 构建计划。
 * @param dependencies 构建能力。
 * @returns 链接配置的路径与打包选项。
 */
function resolveLinkSetup(plan: BuildPlan, dependencies: BuilderDependencies): { linkConfig: string; packagerOptions: PackagerOptions } {
  const { target } = plan;
  const { fileSystem, toolchain } = dependencies;
  const bankCount = target.banked === null ? 0 : Number(plan.targetOptions[target.banked.option] ?? 0);
  if (target.banked === null || bankCount === 0) {
    return { linkConfig: resolveTargetPath(target, target.linkConfig, toolchain.home), packagerOptions: target.packagerOptions };
  }
  const template = fileSystem.readText(resolveTargetPath(target, target.banked.linkConfig, toolchain.home));
  const linkConfig = path.join(plan.objectDir, `${plan.programName}.cfg`);
  fileSystem.writeText(linkConfig, expandBankedLinkConfig(template, target.banked, bankCount));
  // 多页程序的大小由库页个数决定，不再检查单页的大小上限
  const packagerOptions = Object.fromEntries(Object.entries(target.packagerOptions).filter(([key]) => key !== 'maxSize'));
  return { linkConfig, packagerOptions };
}

/**
 * 读取工具链版本，用于增量构建的指纹。
 * @param dependencies 构建能力。
 * @returns cc65 的版本输出，如 “cc65 V2.19 - Git 357f64e”。
 */
async function getToolchainVersion(dependencies: BuilderDependencies): Promise<string> {
  const result = await dependencies.toolRunner.run(dependencies.toolchain, 'cc65', ['--version']);
  return (result.stderr || result.stdout).trim();
}

/**
 * 构建一个程序。
 * @param plan 构建计划。
 * @param dependencies 构建能力。
 * @param reporter 汇报器。
 * @returns 构建结果。
 * @throws FcdevError 编译、链接或打包失败。
 */
export async function buildProgram(plan: BuildPlan, dependencies: BuilderDependencies, reporter: BuildReporter = SILENT_REPORTER): Promise<BuildResult> {
  const { target, config } = plan;
  const { fileSystem, toolchain, resources } = dependencies;
  const toTargetPaths = (values: readonly string[]): string[] => values.map((value) => resolveTargetPath(target, value, toolchain.home));
  const settings: CompileSettings = {
    cc65Target: target.cc65Target,
    cpu: target.cpu,
    optimize: config.optimize,
    includeDirs: [...toTargetPaths(target.include), ...config.include, plan.sourceBase],
    asmIncludeDirs: [...toTargetPaths(target.asminclude), ...config.include, plan.sourceBase],
    defines: { ...target.defines, ...config.define }
  };

  fileSystem.ensureDirectory(plan.objectDir);
  fileSystem.ensureDirectory(path.dirname(plan.outputFile));

  const stampFile = path.join(plan.objectDir, 'stamp.json');
  const fingerprint = computeFingerprint(plan, resources.version, await getToolchainVersion(dependencies));
  if (readStamp(fileSystem, stampFile) !== fingerprint) {
    // 指纹变了：删掉旧的中间文件再重编
    fileSystem.removeDirectory(plan.objectDir);
    fileSystem.ensureDirectory(plan.objectDir);
  }

  const context: CompileContext = { dependencies, settings, reporter, sourceCharset: target.sourceCharset };
  const objects: string[] = [];
  let compiled = 0;
  let reused = 0;
  for (const source of plan.sources) {
    const relative = path.relative(plan.sourceBase, source);
    const objectName = `${relative.slice(0, relative.length - path.extname(relative).length).split(path.sep).join('__')}.o`;
    const objectFile = path.join(plan.objectDir, objectName);
    objects.push(objectFile);
    if (isUpToDate(fileSystem, objectFile, source, `${objectFile.slice(0, -2)}.d`)) {
      reused += 1;
      continue;
    }
    reporter.log(`编译 ${relative}`);
    await compileSource(context, source, objectFile);
    compiled += 1;
  }

  const binaryFile = path.join(plan.objectDir, `${plan.programName}.bin`);
  const optionObjects = Object.entries(plan.targetOptions)
    .filter(([key, value]) => value === true && target.options[key] !== undefined)
    .flatMap(([key]) => target.options[key]?.links ?? []);
  reporter.log('链接');
  const linkSetup = resolveLinkSetup(plan, dependencies);
  await runChecked(
    dependencies,
    'ld65',
    buildLinkerArguments({
      linkConfig: linkSetup.linkConfig,
      binaryFile,
      mapFile: path.join(plan.objectDir, `${plan.programName}.map`),
      debugFile: path.join(plan.objectDir, `${plan.programName}.dbg`),
      objects: [...toTargetPaths(target.startup), ...objects, ...toTargetPaths(optionObjects)],
      libraries: toTargetPaths(target.libraries),
      forceImports: target.forceImports
    }),
    reporter,
    `链接失败：${plan.programName}`
  );

  const packaged = packageProgram(target.packager, fileSystem.readBytes(binaryFile), linkSetup.packagerOptions);
  fileSystem.writeBytes(plan.outputFile, packaged);
  fileSystem.writeText(stampFile, JSON.stringify({ fingerprint }));
  return { programName: plan.programName, outputFile: plan.outputFile, size: packaged.length, compiled, reused };
}
