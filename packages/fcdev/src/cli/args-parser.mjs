// ------------------------------------------------------------------------
// 名称：args-parser.mjs
// 说明：解析命令行参数：命令、位置参数与通用选项。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：debug 命令把命令名之后的全部参数原样交给 ember-debug，不再解析选项。
// ------------------------------------------------------------------------
import { FcdevError } from '../shared/index.mjs';

/** 不解析选项、参数原样传递的命令 */
const RAW_COMMANDS = new Set(['debug']);

/** 带一个字符串值的选项：选项名到 options 里的字段 */
const VALUE_OPTIONS = {
  '--format': 'format',
  '--base': 'base',
  '--cdl': 'cdl',
  '--trace': 'trace',
  '--symbols': 'symbols',
  '--out': 'out',
};

/**
 * @typedef {object} ParsedArguments
 * @property {string|null} command 命令名；没有给命令时为 null
 * @property {string[]} positionals 位置参数
 * @property {{target?: string, port?: number, debug: boolean, verbose: boolean, json: boolean, help: boolean, version: boolean, format?: string, base?: string, cdl?: string, trace?: string, symbols?: string, out?: string, entries: string[]}} options 选项
 */

/**
 * 解析 process.argv 去掉前两项之后的参数。
 * @param {string[]} argv 命令行参数
 * @returns {ParsedArguments} 解析结果
 * @throws {FcdevError} 出现未知选项，或选项缺少值
 */
export function parseArguments(argv) {
  const options = { debug: false, verbose: false, json: false, help: false, version: false, entries: [] };
  const positionals = [];
  let command = null;
  for (let index = 0; index < argv.length; index += 1) {
    const token = argv[index];
    if (command !== null && RAW_COMMANDS.has(command)) {
      positionals.push(token);
    } else if (token === '--help' || token === '-h') {
      options.help = true;
    } else if (token === '--version' || token === '-v') {
      options.version = true;
    } else if (token === '--verbose') {
      options.verbose = true;
    } else if (token === '--json') {
      options.json = true;
    } else if (token === '--debug') {
      options.debug = true;
    } else if (token === '--entry' || token.startsWith('--entry=')) {
      const value = token === '--entry' ? argv[(index += 1)] : token.slice('--entry='.length);
      if (!value) throw new FcdevError('--entry 缺少地址', { code: 'ARGUMENT_INVALID' });
      options.entries.push(value);
    } else if (VALUE_OPTIONS[token.split('=')[0]]) {
      const name = token.split('=')[0];
      const value = token.includes('=') ? token.slice(name.length + 1) : argv[(index += 1)];
      if (!value) throw new FcdevError(`${name} 缺少值`, { code: 'ARGUMENT_INVALID' });
      options[VALUE_OPTIONS[name]] = value;
    } else if (token === '--port' || token.startsWith('--port=')) {
      const value = token === '--port' ? argv[(index += 1)] : token.slice('--port='.length);
      const port = Number(value);
      if (!Number.isInteger(port) || port < 1 || port > 65535) {
        throw new FcdevError(`--port 需要 1 到 65535 之间的整数，实际为“${value ?? ''}”`, { code: 'ARGUMENT_INVALID' });
      }
      options.port = port;
    } else if (token === '--target' || token.startsWith('--target=')) {
      const value = token === '--target' ? argv[(index += 1)] : token.slice('--target='.length);
      if (!value) throw new FcdevError('--target 缺少目标名', { code: 'ARGUMENT_INVALID' });
      options.target = value;
    } else if (token.startsWith('-')) {
      throw new FcdevError(`未知的选项：${token}`, { code: 'ARGUMENT_INVALID', hint: '用 fcdev --help 查看可用的选项' });
    } else if (command === null) {
      command = token;
    } else {
      positionals.push(token);
    }
  }
  return { command, positionals, options };
}
