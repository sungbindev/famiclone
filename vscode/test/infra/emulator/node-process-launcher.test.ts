// ------------------------------------------------------------------------
// 名称：node-process-launcher.test.ts
// 说明：测试用 Node 启动与结束 Ember 的实现：端口检查、调试用户数据目录的位置与准备、令牌、启动不存
//       在的程序时报错、结束已退出的进程。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：不启动真实的 Ember；端口检查在本机随机端口上做。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { existsSync, mkdirSync, writeFileSync } from 'node:fs';
import * as net from 'node:net';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'vitest';
import { NodeEmulatorProcessLauncher } from '../../../src/infra/emulator/node-process-launcher';
import type { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { createTempDir, type TempDir } from '../../support/build-fixtures';

/** 在随机空闲端口上监听，返回服务与端口。 */
function listenOnFreePort(): Promise<{ server: net.Server; port: number }> {
  return new Promise((resolve) => {
    const server = net.createServer();
    server.listen(0, '127.0.0.1', () => resolve({ server, port: (server.address() as net.AddressInfo).port }));
  });
}

describe('启动与结束 Ember 的 Node 实现', () => {
  let temp: TempDir;
  let launcher: NodeEmulatorProcessLauncher;

  beforeEach(() => {
    temp = createTempDir();
    launcher = new NodeEmulatorProcessLauncher(temp.dir);
  });
  afterEach(() => temp.cleanup());

  it('被占用的端口报错并提示改设置；空闲的端口通过', async () => {
    const { server, port } = await listenOnFreePort();
    try {
      await assert.rejects(launcher.assertPortFree(port), (error: FcdevError) => error.code === 'EMULATOR_PORT_BUSY' && error.hint.includes('fcdev.emulator.debugPort'));
    } finally {
      server.close();
    }
    await launcher.assertPortFree(port);
  });

  it('调试用户数据目录按端口区分，放在临时目录的 fcdev-ember 下', () => {
    assert.equal(launcher.userDataDirectory(5699), path.join(temp.dir, 'fcdev-ember', '5699'));
    assert.notEqual(launcher.userDataDirectory(5688), launcher.userDataDirectory(5699));
  });

  it('准备用户数据目录：创建目录，删掉上一次会话遗留的端点文件', () => {
    const directory = launcher.userDataDirectory(5699);
    mkdirSync(path.join(directory, 'run'), { recursive: true });
    const endpoint = path.join(directory, 'run', 'debug-endpoint.json');
    writeFileSync(endpoint, '{}');
    assert.equal(launcher.prepareUserDataDirectory(5699), directory);
    assert.equal(existsSync(endpoint), false);
    assert.equal(existsSync(directory), true);
  });

  it('令牌是每次不同的 32 位十六进制文字', () => {
    const first = launcher.createToken();
    assert.match(first, /^[0-9a-f]{32}$/);
    assert.notEqual(first, launcher.createToken());
  });

  it('启动不存在的程序时 failure 以启动失败结束，confirmStarted 抛出', async () => {
    const child = launcher.start(path.join(temp.dir, 'none', 'Ember.exe'), [], false);
    await assert.rejects(launcher.confirmStarted(child), (error: FcdevError) => error.code === 'EMULATOR_START_FAILED');
  });

  it('结束一个已经退出的进程返回 false', () => {
    assert.equal(launcher.terminate(2_147_483_000), false);
  });
});
