// ------------------------------------------------------------------------
// 名称：args.mjs
// 说明：解析 create-fcdev 的命令行参数：目录、--target、--template、--no-install。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import { CreateError } from './errors.mjs';

/**
 * @typedef {object} CreateArguments
 * @property {string|undefined} directory 目录，缺省为当前目录
 * @property {string|undefined} target 目标名
 * @property {string|undefined} language 语言
 * @property {boolean} install 是否安装依赖并生成 IDE 配置
 * @property {boolean} help 是否显示帮助
 * @property {boolean} version 是否显示版本
 */

/**
 * 解析命令行参数。
 * @param {string[]} argv 去掉 node 与脚本路径后的参数
 * @returns {CreateArguments} 解析结果
 * @throws {CreateError} 未知的选项、选项缺少值或位置参数多于一个
 */
export function parseCreateArguments(argv) {
  const result = { directory: undefined, target: undefined, language: undefined, install: true, help: false, version: false };
  const readValue = (token, name, index) => {
    const value = token.includes('=') ? token.slice(token.indexOf('=') + 1) : argv[index + 1];
    if (!value || value.startsWith('-')) throw new CreateError(`${name} 缺少值`, { code: 'ARGUMENT_INVALID' });
    return value;
  };
  for (let index = 0; index < argv.length; index += 1) {
    const token = argv[index];
    if (token === '--help' || token === '-h') {
      result.help = true;
    } else if (token === '--version' || token === '-v') {
      result.version = true;
    } else if (token === '--no-install') {
      result.install = false;
    } else if (token === '--target' || token.startsWith('--target=')) {
      result.target = readValue(token, '--target', index);
      if (!token.includes('=')) index += 1;
    } else if (token === '--template' || token.startsWith('--template=')) {
      result.language = readValue(token, '--template', index);
      if (!token.includes('=')) index += 1;
    } else if (token.startsWith('-')) {
      throw new CreateError(`未知的选项：${token}`, { code: 'ARGUMENT_INVALID', hint: '用 --help 查看可用的选项' });
    } else if (result.directory === undefined) {
      result.directory = token;
    } else {
      throw new CreateError('只能指定一个目录', { code: 'ARGUMENT_INVALID' });
    }
  }
  return result;
}
