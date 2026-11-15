// ------------------------------------------------------------------------
// 名称：emulator.test.mjs
// 说明：测试 Ember 相关功能：定位、端口检查、调试用户数据目录与环境变量、run 与 debug 命令的行为，以及真实 Ember 里运行 NES 程序、用 debug 命令连接并截图。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：debug 命令的用例需要调试 SDK 已构建（npm run build -w packages/ember-debug-sdk），没有时跳过；
//       真实 Ember 的用例会短暂打开一个窗口，需要环境变量 FCDEV_EMULATOR 指向 Ember 的可执行文件、cc65 与已构建的 SDK，缺一就跳过，设置 FCDEV_SKIP_GUI_TESTS=1 也会跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync, rmSync } from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath } from '../src/build/index.mjs';
import { runCli } from '../src/cli/index.mjs';
import { assertPortFree, debugEnvironment, debugUserDataDirectory, keyCodeOf, launchEmulator, locateEmulator, resolveDebugCli } from '../src/emulator/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** 在随机空闲端口上监听，返回服务与端口 */
function listenOnFreePort() {
  return new Promise((resolve) => {
    const server = net.createServer();
    server.listen(0, '127.0.0.1', () => resolve({ server, port: server.address().port }));
  });
}

/** 取一个当前空闲的端口（取到后立即释放） */
async function findFreePort() {
  const { server, port } = await listenOnFreePort();
  await new Promise((resolve) => server.close(resolve));
  return port;
}

/** 调试 SDK 是否已构建 */
function isSdkBuilt() {
  try {
    resolveDebugCli();
    return true;
  } catch {
    return false;
  }
}

const SKIP_WITHOUT_SDK = isSdkBuilt() ? false : '调试 SDK 没有构建（先运行 npm run build -w packages/ember-debug-sdk）';

/** 运行命令行，返回退出码与输出 */
async function runFcdev(argv, { cwd, env }) {
  const stdout = [];
  const stderr = [];
  const code = await runCli(argv, { cwd, env, stdout: { write: (text) => stdout.push(text) }, stderr: { write: (text) => stderr.push(text) } });
  return { code, stdout: stdout.join(''), stderr: stderr.join('') };
}

describe('端口检查', () => {
  it('被占用的端口报错', async () => {
    const { server, port } = await listenOnFreePort();
    try {
      await assert.rejects(assertPortFree(port), (error) => error.code === 'EMULATOR_PORT_BUSY' && error.hint.includes('--port'));
    } finally {
      server.close();
    }
  });

  it('空闲的端口通过', async () => {
    await assertPortFree(await findFreePort());
  });
});

describe('Ember 定位', () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('配置项优先于环境变量', () => {
    const configured = writeTestFile(temp.dir, 'a/ember.exe', '');
    const fromEnv = writeTestFile(temp.dir, 'b/ember.exe', '');
    assert.deepEqual(locateEmulator({ configured, env: { FCDEV_EMULATOR: fromEnv } }), { executable: configured, source: 'config' });
  });

  it('只有环境变量时取环境变量', () => {
    const fromEnv = writeTestFile(temp.dir, 'b/ember.exe', '');
    assert.deepEqual(locateEmulator({ env: { FCDEV_EMULATOR: fromEnv } }), { executable: fromEnv, source: 'env' });
  });

  it('指定的路径不存在时报错', () => {
    assert.throws(() => locateEmulator({ configured: path.join(temp.dir, 'none.exe'), env: {} }), (error) => error.code === 'EMULATOR_NOT_FOUND' && error.message.includes('Ember'));
  });

  it('都没有指定时报错并提示两种指定方式', () => {
    assert.throws(() => locateEmulator({ env: {} }), (error) => error.code === 'EMULATOR_NOT_FOUND' && error.hint.includes('FCDEV_EMULATOR') && error.hint.includes('emulator'));
  });
});

describe('调试会话的位置', () => {
  it('调试用户数据目录按端口区分，放在临时目录的 fcdev-ember 下', () => {
    const base = path.resolve('/tmp-base');
    assert.equal(debugUserDataDirectory(5699, base), path.join(base, 'fcdev-ember', '5699'));
    assert.notEqual(debugUserDataDirectory(5688, base), debugUserDataDirectory(5699, base));
  });

  it('没有显式指定时，按 FCDEV_DEBUG_PORT 推出用户数据目录，没有端口时用默认端口 5699', () => {
    assert.equal(debugEnvironment({ FCDEV_DEBUG_PORT: '5688' }).EMBER_USER_DATA, debugUserDataDirectory(5688));
    assert.equal(debugEnvironment({}).EMBER_USER_DATA, debugUserDataDirectory(5699));
  });

  it('用户显式指定了调试服务的位置时保持原样', () => {
    for (const name of ['EMBER_DEBUG_URL', 'EMBER_DEBUG_TOKEN', 'EMBER_DEBUG_ENDPOINT', 'EMBER_USER_DATA']) {
      const env = { [name]: 'x', FCDEV_DEBUG_PORT: '5688' };
      assert.deepEqual(debugEnvironment(env), env);
    }
  });
});

describe('学习机的按键', () => {
  it('字母与数字对应物理键位，字母不分大小写', () => {
    assert.equal(keyCodeOf('a'), 'KeyA');
    assert.equal(keyCodeOf('Z'), 'KeyZ');
    assert.equal(keyCodeOf('7'), 'Digit7');
  });

  it('其他字符无法输入，并提示改程序名', () => {
    assert.throws(() => keyCodeOf('-'), (error) => error.code === 'DOS_KEY_UNSUPPORTED' && error.hint.includes('name'));
  });
});

describe('run 命令', () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('学习机目标没有配置固件时在构建与启动之前报错，并提示设置 firmware', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: 'hello' }));
    writeTestFile(temp.dir, 'src/hello.c', '#include <sb2000.h>\nint main(void) { return 0; }\n');
    const { code, stderr } = await runFcdev(['run', 'src'], { cwd: temp.dir, env: { ...process.env, FCDEV_EMULATOR: process.execPath } });
    assert.equal(code, 1);
    assert.match(stderr, /需要固件/);
    assert.match(stderr, /firmware/);
  });

  it('找不到 Ember 时报错，在构建之前就报', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'hello.c', 'int main(void) { for (;;) { } }\n');
    const env = { ...process.env };
    delete env.FCDEV_EMULATOR;
    const { code, stderr } = await runFcdev(['run', 'hello.c'], { cwd: temp.dir, env });
    assert.equal(code, 1);
    assert.match(stderr, /找不到 Ember/);
  });
});

describe('debug 命令（需要调试 SDK）', { skip: SKIP_WITHOUT_SDK }, () => {
  it('参数交给 ember-debug：--version 输出版本号，退出码 0', async () => {
    const { code, stdout } = await runFcdev(['debug', '--version'], { cwd: process.cwd(), env: process.env });
    assert.equal(code, 0);
    assert.match(stdout, /\d+\.\d+\.\d+/);
  });

  it('没有运行的 Ember 时退出码 1，并说明在 FCDEV_DEBUG_PORT 对应的用户数据目录里找不到端点文件', async () => {
    const env = { ...process.env, FCDEV_DEBUG_PORT: '1' };
    for (const name of ['EMBER_DEBUG_URL', 'EMBER_DEBUG_TOKEN', 'EMBER_DEBUG_ENDPOINT', 'EMBER_USER_DATA']) delete env[name];
    const { code, stderr } = await runFcdev(['debug', 'call', 'runtime.status'], { cwd: process.cwd(), env });
    assert.equal(code, 1);
    assert.ok(stderr.includes(debugUserDataDirectory(1)), `错误里应出现 ${debugUserDataDirectory(1)}：${stderr}`);
  });
});

/** 找真实的 Ember；没有时返回 null */
function findRealEmber() {
  if (process.env.FCDEV_SKIP_GUI_TESTS === '1') return null;
  try {
    return locateEmulator();
  } catch {
    return null;
  }
}

const realEmber = findRealEmber();

describe('真实 Ember', { skip: realEmber && !SKIP_WITHOUT_TOOLCHAIN && !SKIP_WITHOUT_SDK ? false : '没有 Ember（FCDEV_EMULATOR）、cc65 工具链或调试 SDK，或设置了 FCDEV_SKIP_GUI_TESTS' }, () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('NES 程序在调试模式下载入后停在开机状态，运行 60 帧后能截图，debug 命令能连上同一个 Ember', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'hello.c', '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "TEST"); nes_wait_vblank(); nes_screen_on(); for (;;) { } }\n');
    const { result } = await buildPath({ pathArg: 'hello.c', cwd: temp.dir });
    const port = await findFreePort();

    const { pid, client } = await launchEmulator({ executable: realEmber.executable, romFile: result.outputFile, debug: true, port });
    try {
      const before = await client.request('runtime.status');
      assert.deepEqual([before.power, before.mode], ['on', 'paused']);
      const run = await client.request('runtime.run', { frames: 60, wait: true }, { timeoutMs: 60_000 });
      assert.equal(run.frames, 60);
      assert.equal(run.stopped, 'completed');
      const shot = await client.request('output.screenshot', {});
      assert.equal(Buffer.from(shot.data).subarray(1, 4).toString('latin1'), 'PNG');

      const env = { ...process.env, FCDEV_DEBUG_PORT: String(port) };
      const status = await runFcdev(['debug', 'call', 'runtime.status', '--compact'], { cwd: temp.dir, env });
      assert.equal(status.code, 0, status.stderr);
      assert.equal(JSON.parse(status.stdout).power, 'on');
      const saved = await runFcdev(['debug', 'screenshot', 'cli-shot.png'], { cwd: temp.dir, env });
      assert.equal(saved.code, 0, saved.stderr);
      assert.equal(readFileSync(path.join(temp.dir, 'cli-shot.png')).subarray(1, 4).toString('latin1'), 'PNG');
    } finally {
      client.close();
      process.kill(pid);
      // Ember 退出需要一点时间，目录还被占用时放弃清理也不影响测试结果
      try {
        rmSync(debugUserDataDirectory(port), { recursive: true, force: true, maxRetries: 10, retryDelay: 300 });
      } catch {
        // 临时目录留在系统临时目录里，下次启动会覆盖
      }
    }
  });
});
