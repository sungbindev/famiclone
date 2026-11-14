/*
 * ------------------------------------------------------------------------
 * 名称：endpoint-file.test.ts
 * 说明：端点发现的测试：各系统的默认用户数据目录、端点文件路径的优先级、文件缺失与过期的提示、地址与令牌的取值顺序。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { join } from 'node:path';

import { describe, expect, it } from 'vitest';

import { defaultUserDataDirectory, endpointFilePath, readEndpoint, resolveTarget, type EndpointDeps } from '../src/infra/endpoint-file.js';

const ENDPOINT = { url: 'ws://127.0.0.1:5688/', port: 5688, token: 'file-token', pid: 4242, coreVersion: '0.1.0', appVersion: '0.1.0', startedAt: '2026-11-14T08:00:00Z' };

/** 假环境：files 里的路径有内容，alive 里的进程在运行。 */
function deps(options: { platform?: NodeJS.Platform; env?: NodeJS.ProcessEnv; files?: Record<string, string>; alive?: number[] } = {}): EndpointDeps {
  return {
    env: options.env ?? {},
    platform: options.platform ?? 'win32',
    homeDirectory: join('home', 'user'),
    readFile: (path) => {
      const content = options.files?.[path];
      if (content === undefined) throw Object.assign(new Error('不存在'), { code: 'ENOENT' });
      return content;
    },
    isProcessAlive: (pid) => (options.alive ?? [4242]).includes(pid)
  };
}

const ROAMING = join('C:', 'Roaming');
const DEFAULT_FILE = join(ROAMING, 'Ember', 'run', 'debug-endpoint.json');

describe('defaultUserDataDirectory', () => {
  it('各系统上 Ember 的用户数据目录', () => {
    expect(defaultUserDataDirectory(deps({ platform: 'win32', env: { APPDATA: ROAMING } }))).toBe(join(ROAMING, 'Ember'));
    expect(defaultUserDataDirectory(deps({ platform: 'win32' }))).toBe(join('home', 'user', 'AppData', 'Roaming', 'Ember'));
    expect(defaultUserDataDirectory(deps({ platform: 'darwin' }))).toBe(join('home', 'user', 'Library', 'Application Support', 'Ember'));
    expect(defaultUserDataDirectory(deps({ platform: 'linux' }))).toBe(join('home', 'user', '.config', 'Ember'));
    expect(defaultUserDataDirectory(deps({ platform: 'linux', env: { XDG_CONFIG_HOME: '/xdg' } }))).toBe(join('/xdg', 'Ember'));
  });
});

describe('endpointFilePath', () => {
  it('显式的文件路径优先，其次环境变量，再次用户数据目录', () => {
    expect(endpointFilePath({ endpointFile: 'a.json' }, deps({ env: { EMBER_DEBUG_ENDPOINT: 'b.json' } }))).toBe('a.json');
    expect(endpointFilePath({}, deps({ env: { EMBER_DEBUG_ENDPOINT: 'b.json' } }))).toBe('b.json');
    expect(endpointFilePath({ userDataDirectory: 'data' }, deps())).toBe(join('data', 'run', 'debug-endpoint.json'));
    expect(endpointFilePath({}, deps({ env: { EMBER_USER_DATA: 'envdata' } }))).toBe(join('envdata', 'run', 'debug-endpoint.json'));
    expect(endpointFilePath({}, deps({ env: { APPDATA: 'C:' } }))).toBe(join('C:', 'Ember', 'run', 'debug-endpoint.json'));
  });
});

describe('readEndpoint', () => {
  it('读出端点文件', () => {
    expect(readEndpoint({ endpointFile: 'e.json' }, deps({ files: { 'e.json': JSON.stringify(ENDPOINT) } }))).toEqual(ENDPOINT);
  });

  it('文件不存在时提示 Ember 没有运行或没有开启调试服务', () => {
    expect(() => readEndpoint({ endpointFile: 'e.json' }, deps())).toThrow(/找不到端点文件 e\.json.*没有开启调试服务/);
  });

  it('内容不对或写文件的进程已退出时报错', () => {
    expect(() => readEndpoint({ endpointFile: 'e.json' }, deps({ files: { 'e.json': '{}' } }))).toThrow('内容不对');
    expect(() => readEndpoint({ endpointFile: 'e.json' }, deps({ files: { 'e.json': 'xx' } }))).toThrow('无法读取');
    expect(() => readEndpoint({ endpointFile: 'e.json' }, deps({ files: { 'e.json': JSON.stringify(ENDPOINT) }, alive: [] }))).toThrow('已过期');
  });
});

describe('resolveTarget', () => {
  const files = { [DEFAULT_FILE]: JSON.stringify(ENDPOINT) };
  const env = { APPDATA: ROAMING };

  it('地址与令牌都给出（参数或环境变量）时不读文件', () => {
    expect(resolveTarget({ url: 'ws://a/', token: 'x' }, deps())).toMatchObject({ url: 'ws://a/', token: 'x' });
    expect(resolveTarget({}, deps({ env: { EMBER_DEBUG_URL: 'ws://e/', EMBER_DEBUG_TOKEN: 'y' } }))).toMatchObject({ url: 'ws://e/', token: 'y' });
  });

  it('参数优先于环境变量，缺的部分从端点文件补', () => {
    const fromFile = resolveTarget({ userDataDirectory: join(ROAMING, 'Ember') }, deps({ files }));
    expect(fromFile).toMatchObject({ url: ENDPOINT.url, token: 'file-token' });
    expect(resolveTarget({ token: 'mine', userDataDirectory: join(ROAMING, 'Ember') }, deps({ files }))).toMatchObject({ url: ENDPOINT.url, token: 'mine' });
    expect(resolveTarget({ url: 'ws://other/' }, deps({ env, files }))).toMatchObject({ url: 'ws://other/', token: 'file-token' });
  });

  it('找不到端点文件又没有给全地址与令牌时抛出 endpoint 错误', () => {
    expect(() => resolveTarget({ url: 'ws://a/' }, deps())).toThrow(expect.objectContaining({ kind: 'endpoint' }));
  });
});
