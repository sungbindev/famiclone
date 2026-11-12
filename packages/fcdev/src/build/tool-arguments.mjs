// ------------------------------------------------------------------------
// 名称：tool-arguments.mjs
// 说明：生成 cc65、ca65、ld65 的命令行参数：只拼参数，不运行，便于测试。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：一律带 -g 输出调试信息；依赖文件用 --create-dep 生成，供增量构建判断。
// ------------------------------------------------------------------------

/**
 * 预定义宏转成 -D 参数。
 * @param {Record<string, string|number>} defines 宏表
 * @returns {string[]} 参数列表
 */
function defineArguments(defines) {
  return Object.entries(defines).flatMap(([name, value]) => ['-D', `${name}=${value}`]);
}

/**
 * @typedef {object} CompileSettings
 * @property {string} cc65Target 目标名
 * @property {string} cpu CPU 型号
 * @property {boolean} optimize 是否优化
 * @property {string[]} includeDirs C 头文件目录
 * @property {string[]} asmIncludeDirs 汇编引入目录
 * @property {Record<string, string|number>} defines 预定义宏
 */

/**
 * cc65 编译一个 C 文件的参数。
 * @param {CompileSettings} settings 编译设置
 * @param {{source: string, output: string, dependencyFile: string}} files 源文件、输出的汇编文件与依赖文件
 * @returns {string[]} 参数列表
 */
export function buildCompilerArguments(settings, files) {
  return [
    '-t', settings.cc65Target,
    '--cpu', settings.cpu,
    '-g',
    ...(settings.optimize ? ['-Oirs'] : []),
    ...settings.includeDirs.flatMap((dir) => ['-I', dir]),
    ...defineArguments(settings.defines),
    '--create-dep', files.dependencyFile,
    '-o', files.output,
    files.source,
  ];
}

/**
 * ca65 汇编一个文件的参数。
 * @param {CompileSettings} settings 编译设置
 * @param {{source: string, output: string, dependencyFile: string}} files 源文件、输出的目标文件与依赖文件
 * @returns {string[]} 参数列表
 */
export function buildAssemblerArguments(settings, files) {
  return [
    '-t', settings.cc65Target,
    '--cpu', settings.cpu,
    '-g',
    ...settings.asmIncludeDirs.flatMap((dir) => ['-I', dir, '--bin-include-dir', dir]),
    ...defineArguments(settings.defines),
    '--create-dep', files.dependencyFile,
    '-o', files.output,
    files.source,
  ];
}

/**
 * ld65 链接的参数。
 * @param {{linkConfig: string, binaryFile: string, mapFile: string, debugFile: string, objects: string[], libraries: string[], forceImports?: string[]}} input 链接配置、输出文件、目标文件、库与强制引入的符号
 * @returns {string[]} 参数列表
 */
export function buildLinkerArguments(input) {
  return [
    '-C', input.linkConfig,
    '-m', input.mapFile,
    '--dbgfile', input.debugFile,
    '-o', input.binaryFile,
    ...(input.forceImports ?? []).flatMap((symbol) => ['-u', symbol]),
    ...input.objects,
    ...input.libraries,
  ];
}
