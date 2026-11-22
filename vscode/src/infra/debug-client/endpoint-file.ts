// ------------------------------------------------------------------------
// 名称：endpoint-file.ts
// 说明：找到正在运行的 Ember 调试服务：读端点文件（用户数据目录下的 run/debug-endpoint.json），或用
//       环境变量与显式参数给出地址与令牌。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-14
// 备注：只在 Node 环境使用；文件系统、环境变量、进程检查都经 EndpointDeps 注入，便于测试。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import { homedir } from 'node:os';
import { join } from 'node:path';

import type { Endpoint, EndpointResolver, Target, TargetOptions } from '../../contracts/debug-client/endpoint';
import { EmberConnectionError } from '../../shared/debug-client/errors';

/** 外部环境，测试时替换。 */
export interface EndpointDeps {
  readonly env: NodeJS.ProcessEnv;
  readonly platform: NodeJS.Platform;
  readonly homeDirectory: string;
  readFile(path: string): string;
  isProcessAlive(pid: number): boolean;
}

/** 用户数据目录里的应用目录名（Electron 用 productName）。 */
const APP_DIRECTORY_NAME = 'Ember';

/** 默认的外部环境。 */
export const SYSTEM_DEPS: EndpointDeps = {
  env: process.env,
  platform: process.platform,
  homeDirectory: homedir(),
  readFile: (path) => readFileSync(path, 'utf8'),
  isProcessAlive: (pid) => {
    try {
      process.kill(pid, 0);
      return true;
    } catch (error) {
      // 没有权限发信号说明进程在。
      return (error as NodeJS.ErrnoException).code === 'EPERM';
    }
  }
};

/** 当前系统上 Ember 的默认用户数据目录。 */
export function defaultUserDataDirectory(deps: EndpointDeps = SYSTEM_DEPS): string {
  if (deps.platform === 'win32') return join(deps.env.APPDATA ?? join(deps.homeDirectory, 'AppData', 'Roaming'), APP_DIRECTORY_NAME);
  if (deps.platform === 'darwin') return join(deps.homeDirectory, 'Library', 'Application Support', APP_DIRECTORY_NAME);
  return join(deps.env.XDG_CONFIG_HOME ?? join(deps.homeDirectory, '.config'), APP_DIRECTORY_NAME);
}

/** 端点文件的路径：显式参数、环境变量、用户数据目录，依次取第一个有的。 */
export function endpointFilePath(options: TargetOptions = {}, deps: EndpointDeps = SYSTEM_DEPS): string {
  const file = options.endpointFile ?? deps.env.EMBER_DEBUG_ENDPOINT;
  if (file !== undefined && file !== '') return file;
  const userData = options.userDataDirectory ?? deps.env.EMBER_USER_DATA;
  return join(userData !== undefined && userData !== '' ? userData : defaultUserDataDirectory(deps), 'run', 'debug-endpoint.json');
}

/** 端点文件的必填字段是否齐全。 */
function isEndpoint(value: unknown): value is Endpoint {
  if (value === null || typeof value !== 'object') return false;
  const record = value as Record<string, unknown>;
  return typeof record.url === 'string' && typeof record.token === 'string' && typeof record.pid === 'number';
}

/**
 * 读端点文件，并确认写它的 Ember 进程还在。
 * @throws EmberConnectionError(endpoint) 文件不存在、内容不对或进程已退出。
 */
export function readEndpoint(options: TargetOptions = {}, deps: EndpointDeps = SYSTEM_DEPS): Endpoint {
  const path = endpointFilePath(options, deps);
  let parsed: unknown;
  try {
    parsed = JSON.parse(deps.readFile(path));
  } catch (error) {
    if ((error as NodeJS.ErrnoException).code === 'ENOENT') {
      throw new EmberConnectionError('endpoint', `找不到端点文件 ${path}：Ember 没有运行，或没有开启调试服务（首选项的“调试服务”分组，或启动参数 --debug-port）；也可以直接给出地址与令牌（emu_connect 的 url 与 token，或环境变量 EMBER_DEBUG_URL 与 EMBER_DEBUG_TOKEN）`);
    }
    throw new EmberConnectionError('endpoint', `无法读取端点文件 ${path}：${error instanceof Error ? error.message : String(error)}`);
  }
  if (!isEndpoint(parsed)) throw new EmberConnectionError('endpoint', `端点文件 ${path} 的内容不对`);
  if (!deps.isProcessAlive(parsed.pid)) throw new EmberConnectionError('endpoint', `端点文件 ${path} 已过期：写它的 Ember 进程（${parsed.pid}）已退出`);
  return parsed;
}

/**
 * 确定连接目标：显式参数优先，其次环境变量，缺的部分从端点文件补。
 * @throws EmberConnectionError(endpoint) 找不到完整的地址与令牌。
 */
export function resolveTarget(options: TargetOptions = {}, deps: EndpointDeps = SYSTEM_DEPS): Target {
  const url = options.url ?? deps.env.EMBER_DEBUG_URL;
  const token = options.token ?? deps.env.EMBER_DEBUG_TOKEN;
  if (url !== undefined && url !== '' && token !== undefined && token !== '') return { url, token, source: '参数或环境变量' };
  const endpoint = readEndpoint(options, deps);
  return { url: url !== undefined && url !== '' ? url : endpoint.url, token: token !== undefined && token !== '' ? token : endpoint.token, source: endpointFilePath(options, deps) };
}

/** 用本机的端点文件与环境变量查找 Ember 的调试服务。 */
export class FileEndpointResolver implements EndpointResolver {
  /** @param deps 外部环境，默认用本机的；测试时替换。 */
  constructor(private readonly deps: EndpointDeps = SYSTEM_DEPS) {}

  /** @inheritdoc */
  endpointFilePath(options: TargetOptions = {}): string {
    return endpointFilePath(options, this.deps);
  }

  /** @inheritdoc */
  readEndpoint(options: TargetOptions = {}): Endpoint {
    return readEndpoint(options, this.deps);
  }

  /** @inheritdoc */
  resolveTarget(options: TargetOptions = {}): Target {
    return resolveTarget(options, this.deps);
  }
}
