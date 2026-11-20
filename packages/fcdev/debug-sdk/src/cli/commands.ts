/*
 * ------------------------------------------------------------------------
 * 名称：commands.ts
 * 说明：终端客户端（fcdev debug）的命令：endpoint、methods、describe、call、screenshot、watch，以及命令行的解析与分发。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：对外部环境（连接、输出、文件）的依赖都经 CliContext 传入，便于测试；退出码 0 成功、1 失败、2 用法错误。
 * ------------------------------------------------------------------------
 */

import { parseArgs } from 'node:util';

import type { RequestMap } from '../../generated/api.js';
import type { Endpoint, TargetOptions } from '../infra/endpoint-file.js';
import type { EmberDebugClient } from '../logic/debug-client.js';
import { describeMethod, findMethod, listMethods, typeSummary, type OpenRpcDocument } from '../logic/method-catalog.js';
import { EmberConnectionError, EmberRequestError } from '../shared/errors.js';
import { parseCallParams, printable, UsageError } from './arguments.js';

/** 终端客户端依赖的外部环境。 */
export interface CliContext {
  readonly document: OpenRpcDocument;
  /** 版本号，给 --version 用。 */
  readonly version: string;
  /** 输出一行到标准输出。 */
  out(text: string): void;
  /** 输出一行到标准错误。 */
  err(text: string): void;
  /** 读端点文件（给 endpoint 命令用）。 */
  readEndpoint(target: TargetOptions): Endpoint;
  /** 找到并连接 Ember；请求超时由每次请求自己指定。 */
  connect(target: TargetOptions): Promise<EmberDebugClient>;
  writeFile(path: string, bytes: Uint8Array): void;
  now(): Date;
}

/** 命令行选项。 */
interface Flags {
  readonly target: TargetOptions;
  readonly compact: boolean;
  readonly showToken: boolean;
  readonly out?: string;
  readonly count?: number;
  readonly timeoutMs?: number;
  readonly format?: string;
}

const HELP = `fcdev debug：Ember 调试服务的终端客户端

用法：fcdev debug <命令> [参数] [选项]

命令：
  endpoint                    显示正在运行的 Ember 的调试服务地址（令牌默认只显示前 4 位）
  methods [关键字]            列出全部请求（可按名称或简介过滤）
  describe <请求或事件>       显示参数、结果、错误码与示例
  call <请求> [参数]          发送一个请求；参数写成 key=value（值按 JSON 解析，不是 JSON 就当字符串；0x8000 与 $8000 是十六进制数，要当字符串就加引号），或一个 JSON 对象
  screenshot [文件]           截取当前画面并保存为 PNG（默认文件名带时间）；--format rgba 保存未压缩的像素
  watch [事件...]             订阅事件并逐行打印（默认订阅全部）；--count 与 --timeout 限制条数与时长

选项：
  --url <地址>                调试服务的地址（环境变量 EMBER_DEBUG_URL）
  --token <令牌>              令牌（环境变量 EMBER_DEBUG_TOKEN）
  --endpoint-file <路径>      端点文件的路径（环境变量 EMBER_DEBUG_ENDPOINT）
  --user-data-dir <目录>      Ember 的用户数据目录（环境变量 EMBER_USER_DATA）
  --timeout <毫秒>            call 等响应的最长时间，默认 60000；watch 的监听时长
  --out <文件>                call 的结果里有二进制数据时保存到这个文件
  --count <条数>              watch 收到这么多条事件后结束
  --format <png|rgba>         screenshot 的图像格式
  --compact                   JSON 输出不缩进
  --show-token                endpoint 显示完整令牌
  -h, --help                  显示帮助
  -v, --version               显示版本

没有给 --url 与 --token 时，自动读端点文件找到正在运行的 Ember；退出码：0 成功，1 失败，2 用法错误。

示例：
  fcdev debug call runtime.status
  fcdev debug call cartridges.load path=D:/roms/game.nes
  fcdev debug call runtime.run frames=300 wait=true
  fcdev debug screenshot shot.png
  fcdev debug watch runtime.* --timeout 5000`;

/** 打印 JSON。 */
function json(value: unknown, flags: Flags): string {
  return JSON.stringify(value, null, flags.compact ? undefined : 2);
}

/** 把令牌打码。 */
function maskToken(token: string): string {
  return `${token.slice(0, 4)}…（共 ${token.length} 个字符）`;
}

/** 解析毫秒或条数这类正整数选项。 */
function positiveInteger(text: string | undefined, name: string): number | undefined {
  if (text === undefined) return undefined;
  const value = Number(text);
  if (!Number.isInteger(value) || value < 0) throw new UsageError(`${name} 必须是非负整数：${text}`);
  return value;
}

/** 命令 endpoint。 */
function endpointCommand(flags: Flags, ctx: CliContext): void {
  const endpoint = ctx.readEndpoint(flags.target);
  ctx.out(json({ ...endpoint, token: flags.showToken ? endpoint.token : maskToken(endpoint.token) }, flags));
}

/** 命令 methods。 */
function methodsCommand(filter: string | undefined, ctx: CliContext): void {
  const methods = listMethods(ctx.document, filter);
  if (methods.length === 0) throw new UsageError(`没有名称或简介含有“${filter ?? ''}”的请求`);
  const width = Math.max(...methods.map((method) => method.name.length));
  for (const method of methods) {
    const mark = method['x-debugOnly'] === true ? '[调试]' : method['x-mainOnly'] === true ? '[主进程]' : '';
    ctx.out(`${method.name.padEnd(width)}  ${mark.padEnd(6)}  ${method.summary}`.trimEnd());
  }
}

/** 命令 describe：请求或事件。 */
function describeCommand(name: string | undefined, ctx: CliContext): void {
  if (name === undefined) throw new UsageError('describe 需要一个请求或事件的名称，用 methods 查看全部请求');
  const method = findMethod(ctx.document, name);
  if (method !== undefined) {
    ctx.out(describeMethod(method));
    return;
  }
  const event = ctx.document['x-events'].find((item) => item.name === name);
  if (event === undefined) throw new UsageError(`没有这个请求或事件：${name}，用 methods 查看全部请求`);
  const required = new Set(event.data.required ?? []);
  ctx.out([`${event.name}（事件，需要先订阅）`, '', event.description, '', '数据：', ...Object.entries(event.data.properties ?? {}).map(([key, child]) => `  ${key}${required.has(key) ? '' : '?'}: ${typeSummary(child)}${child.description === undefined ? '' : ` — ${child.description}`}`)].join('\n'));
}

/** 取出结果里的二进制 data。 */
function binaryData(result: unknown): Uint8Array | undefined {
  const data = (result as { data?: unknown } | null)?.data;
  return data instanceof Uint8Array ? data : undefined;
}

/** 命令 call。 */
async function callCommand(method: string | undefined, tokens: readonly string[], flags: Flags, ctx: CliContext): Promise<void> {
  if (method === undefined) throw new UsageError('call 需要一个请求名，用 methods 查看全部请求');
  const params = parseCallParams(tokens);
  const client = await ctx.connect(flags.target);
  try {
    const result = await client.request(method as keyof RequestMap, params as never, flags.timeoutMs === undefined ? undefined : { timeoutMs: flags.timeoutMs });
    const data = binaryData(result);
    if (data !== undefined && flags.out !== undefined) {
      ctx.writeFile(flags.out, data);
      ctx.err(`二进制数据（${data.byteLength} 字节）已保存到 ${flags.out}`);
    }
    ctx.out(json(printable(result), flags));
  } finally {
    client.close();
  }
}

/** 两位数字，不足补 0。 */
function twoDigits(value: number): string {
  return String(value).padStart(2, '0');
}

/** 默认的截图文件名：带本地时间。 */
function defaultScreenshotName(now: Date, format: string): string {
  const stamp = `${now.getFullYear()}${twoDigits(now.getMonth() + 1)}${twoDigits(now.getDate())}-${twoDigits(now.getHours())}${twoDigits(now.getMinutes())}${twoDigits(now.getSeconds())}`;
  return `ember-screenshot-${stamp}.${format === 'png' ? 'png' : 'rgba'}`;
}

/** 命令 screenshot。 */
async function screenshotCommand(file: string | undefined, flags: Flags, ctx: CliContext): Promise<void> {
  const format = flags.format ?? 'png';
  if (format !== 'png' && format !== 'rgba') throw new UsageError(`--format 只能是 png 或 rgba：${format}`);
  const client = await ctx.connect(flags.target);
  try {
    const result = await client.request('output.screenshot', { format });
    if (result.data === undefined) throw new EmberConnectionError('protocol', '截图的结果里没有图像数据');
    const path = file ?? defaultScreenshotName(ctx.now(), format);
    ctx.writeFile(path, result.data);
    ctx.out(json({ file: path, format: result.format, width: result.width, height: result.height, byteLength: result.byteLength }, flags));
  } finally {
    client.close();
  }
}

/** 命令 watch。 */
async function watchCommand(patterns: readonly string[], flags: Flags, ctx: CliContext): Promise<void> {
  const client = await ctx.connect(flags.target);
  try {
    let received = 0;
    const finished = new Promise<void>((resolve) => {
      client.onAny((event, data) => {
        ctx.out(JSON.stringify({ event, data: printable(data) }));
        received += 1;
        if (flags.count !== undefined && received >= flags.count) resolve();
      });
      void client.closed.then(() => resolve());
      if (flags.timeoutMs !== undefined && flags.timeoutMs > 0) setTimeout(resolve, flags.timeoutMs).unref();
    });
    await client.subscribe(patterns.length === 0 ? ['*'] : patterns);
    await finished;
  } finally {
    client.close();
  }
}

/** 错误转成一行说明。 */
function describeError(error: unknown): string {
  if (error instanceof EmberRequestError) return `错误 ${error.code}：${error.message}`;
  if (error instanceof Error) return error.message;
  return String(error);
}

/**
 * 解析命令行并执行命令。
 * @param argv 命令行参数（不含 node 与脚本路径）。
 * @returns 退出码。
 */
export async function runCli(argv: readonly string[], ctx: CliContext): Promise<number> {
  try {
    const { values, positionals } = parseArgs({
      args: [...argv],
      allowPositionals: true,
      options: {
        url: { type: 'string' },
        token: { type: 'string' },
        'endpoint-file': { type: 'string' },
        'user-data-dir': { type: 'string' },
        timeout: { type: 'string' },
        out: { type: 'string' },
        count: { type: 'string' },
        format: { type: 'string' },
        compact: { type: 'boolean', default: false },
        'show-token': { type: 'boolean', default: false },
        help: { type: 'boolean', short: 'h', default: false },
        version: { type: 'boolean', short: 'v', default: false }
      }
    });
    if (values.version) {
      ctx.out(ctx.version);
      return 0;
    }
    if (values.help || positionals.length === 0) {
      ctx.out(HELP);
      return values.help ? 0 : 2;
    }
    const flags: Flags = {
      target: { url: values.url, token: values.token, endpointFile: values['endpoint-file'], userDataDirectory: values['user-data-dir'] },
      compact: values.compact,
      showToken: values['show-token'],
      out: values.out,
      count: positiveInteger(values.count, '--count'),
      timeoutMs: positiveInteger(values.timeout, '--timeout'),
      format: values.format
    };
    const [command, ...rest] = positionals;
    switch (command) {
      case 'endpoint':
        endpointCommand(flags, ctx);
        break;
      case 'methods':
        methodsCommand(rest[0], ctx);
        break;
      case 'describe':
        describeCommand(rest[0], ctx);
        break;
      case 'call':
        await callCommand(rest[0], rest.slice(1), flags, ctx);
        break;
      case 'screenshot':
        await screenshotCommand(rest[0], flags, ctx);
        break;
      case 'watch':
        await watchCommand(rest, flags, ctx);
        break;
      default:
        throw new UsageError(`没有这个命令：${command ?? ''}，用 --help 查看全部命令`);
    }
    return 0;
  } catch (error) {
    if (error instanceof UsageError || (error instanceof TypeError && 'code' in error && String(error.code).startsWith('ERR_PARSE_ARGS'))) {
      ctx.err(describeError(error));
      return 2;
    }
    ctx.err(describeError(error));
    return 1;
  }
}
