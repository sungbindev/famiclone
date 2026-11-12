// ------------------------------------------------------------------------
// 名称：sync-command.mjs
// 说明：sync 命令：生成 IDE 的 C/C++ 配置，让智能提示找到 cc65 与目标的头文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：写 .vscode/c_cpp_properties.json 里名为 fcdev 的配置（保留其他配置）与项目根的 compile_flags.txt；
//       compile_flags.txt 的第一行是标记 -DFCDEV_GENERATED，没有标记的同名文件视为用户自己的，不覆盖。
// ------------------------------------------------------------------------
import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { FcdevError, readJsonFile } from '../../shared/index.mjs';
import { describeProject } from './project-info.mjs';

/** compile_flags.txt 里标记“由 fcdev 生成”的第一行 */
const GENERATED_MARKER = '-DFCDEV_GENERATED';

/** c_cpp_properties.json 里 fcdev 管理的配置名 */
const CONFIGURATION_NAME = 'fcdev';

/**
 * 把预定义宏转成 NAME=VALUE 形式。
 * @param {Record<string, string|number>} defines 宏表
 * @returns {string[]} 宏列表
 */
function formatDefines(defines) {
  return Object.entries(defines).map(([name, value]) => `${name}=${value}`);
}

/**
 * 写入或更新 .vscode/c_cpp_properties.json 里 fcdev 的配置。
 * @param {string} root 项目根目录
 * @param {import('./project-info.mjs').ProjectInfo} info 项目信息
 * @returns {string} 写入的文件
 * @throws {FcdevError} 已有文件不是合法的配置
 */
function writeCppProperties(root, info) {
  const file = path.join(root, '.vscode', 'c_cpp_properties.json');
  const document = existsSync(file) ? readJsonFile(file) : { version: 4, configurations: [] };
  if (!Array.isArray(document.configurations)) {
    throw new FcdevError(`${file} 缺少 configurations 数组`, { code: 'SYNC_FILE_INVALID' });
  }
  const configuration = {
    name: CONFIGURATION_NAME,
    includePath: info.includeDirs,
    defines: formatDefines(info.defines),
    cStandard: 'c89',
    intelliSenseMode: 'clang-x86',
  };
  const index = document.configurations.findIndex((item) => item.name === CONFIGURATION_NAME);
  if (index >= 0) document.configurations[index] = configuration;
  else document.configurations.push(configuration);
  mkdirSync(path.dirname(file), { recursive: true });
  writeFileSync(file, `${JSON.stringify(document, null, 2)}\n`);
  return file;
}

/**
 * 写入 compile_flags.txt。
 * @param {string} root 项目根目录
 * @param {import('./project-info.mjs').ProjectInfo} info 项目信息
 * @returns {string|null} 写入的文件；已有用户自己的同名文件时为 null
 */
function writeCompileFlags(root, info) {
  const file = path.join(root, 'compile_flags.txt');
  if (existsSync(file) && readFileSync(file, 'utf8').split(/\r?\n/)[0] !== GENERATED_MARKER) return null;
  const lines = [GENERATED_MARKER, ...info.includeDirs.map((dir) => `-I${dir}`), ...formatDefines(info.defines).map((item) => `-D${item}`)];
  writeFileSync(file, `${lines.join('\n')}\n`);
  return file;
}

/**
 * 执行 sync 命令。
 * @param {{options: {target?: string}, context: import('../index.mjs').CliContext}} input 选项与运行环境
 * @returns {Promise<number>} 退出码
 */
export async function runSyncCommand({ options, context }) {
  const info = await describeProject({ cwd: context.cwd, targetOverride: options.target, env: context.env });
  const written = [writeCppProperties(info.root, info)];
  const flags = writeCompileFlags(info.root, info);
  if (flags) written.push(flags);
  else context.stderr.write('compile_flags.txt 已存在且不是 fcdev 生成的，没有覆盖\n');
  context.stdout.write(`已生成：\n${written.map((file) => `  ${path.relative(context.cwd, file)}`).join('\n')}\n`);
  return 0;
}
