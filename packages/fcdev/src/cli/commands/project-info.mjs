// ------------------------------------------------------------------------
// 名称：project-info.mjs
// 说明：汇总当前项目的目标、工具链与头文件目录，供 info 与 sync 命令使用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：从终端当前目录向上查找 fcdev.json；没有配置时必须用 --target 指定目标。
//       预定义宏里带一组编辑器兼容宏：cc65 的头文件大量使用 __fastcall__ 这类关键字，
//       C/C++ 扩展与 clangd 不认识，定义成空宏才不会报语法错误；只用于智能感知，不影响构建。
// ------------------------------------------------------------------------
import path from 'node:path';
import { loadProjectConfig } from '../../config/index.mjs';
import { FcdevError } from '../../shared/index.mjs';
import { loadTarget, resolveTargetPath } from '../../targets/index.mjs';
import { getToolchainVersion, locateToolchain } from '../../toolchain/index.mjs';

/** 只供编辑器使用的兼容宏：cc65 的调用约定关键字定义成空 */
const EDITOR_COMPATIBILITY_DEFINES = { __fastcall__: '', __cdecl__: '' };

/**
 * @typedef {object} ProjectInfo
 * @property {string} root 项目根目录
 * @property {string|null} configFile 配置文件路径
 * @property {{name: string, title: string}} target 目标
 * @property {{home: string, source: string, version: string}} toolchain 工具链
 * @property {string[]} includeDirs C 头文件目录
 * @property {string[]} asmIncludeDirs 汇编引入目录
 * @property {Record<string, string|number>} defines 预定义宏（含 cc65 自带的目标宏与编辑器兼容宏）
 * @property {string} outDir 输出目录
 */

/**
 * 汇总项目信息。
 * @param {{cwd: string, targetOverride?: string, env?: NodeJS.ProcessEnv}} input 终端目录、目标覆盖与环境变量
 * @returns {Promise<ProjectInfo>} 项目信息
 * @throws {FcdevError} 没有指定目标，或找不到工具链
 */
export async function describeProject({ cwd, targetOverride, env }) {
  const config = loadProjectConfig(cwd, cwd);
  const targetName = targetOverride ?? config.target;
  if (!targetName) {
    throw new FcdevError('没有指定目标', { code: 'TARGET_MISSING', hint: '在 fcdev.json 里写 "target"，或用 --target <名> 指定' });
  }
  const target = loadTarget(targetName);
  const toolchain = locateToolchain({ configured: config.toolchain, env });
  const resolveAll = (values) => values.map((value) => resolveTargetPath(target, value, toolchain.home));
  return {
    root: config.root,
    configFile: config.file,
    target: { name: target.name, title: target.title },
    toolchain: { home: toolchain.home, source: toolchain.source, version: await getToolchainVersion(toolchain) },
    includeDirs: [...resolveAll(target.include), ...config.include, path.join(toolchain.home, 'include')],
    asmIncludeDirs: [...resolveAll(target.asminclude), ...config.include, path.join(toolchain.home, 'asminc')],
    defines: { ...EDITOR_COMPATIBILITY_DEFINES, __CC65__: 1, [`__${target.cc65Target.toUpperCase()}__`]: 1, ...target.defines, ...config.define },
    outDir: config.outDir,
  };
}
