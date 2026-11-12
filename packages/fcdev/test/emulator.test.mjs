// ------------------------------------------------------------------------
// 名称：emulator.test.mjs
// 说明：测试模拟器相关功能：调试客户端的应答解析与超时、模拟器定位、端口检查、debug 命令的路径换算，以及真实模拟器里运行 NES 程序并截图。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：客户端用本机临时 TCP 服务模拟调试服务；真实模拟器的用例会短暂打开一个窗口，找不到模拟器或不在 Windows 上时跳过，设置 FCDEV_SKIP_GUI_TESTS=1 也会跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { resolveCommandPaths } from '../src/cli/commands/debug-command.mjs';
import { DebugClient, assertPortFree, launchEmulator, locateEmulator, parseReply } from '../src/emulator/index.mjs';
import { buildPath } from '../src/build/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** 在随机空闲端口上启动一个假的调试服务，handler 收到命令后返回应答文本 */
function startFakeServer(handler) {
  return new Promise((resolve) => {
    const server = net.createServer((socket) => {
      socket.setEncoding('utf8');
      let pending = '';
      socket.on('data', (chunk) => {
        pending += chunk;
        for (let end = pending.indexOf('\n'); end >= 0; end = pending.indexOf('\n')) {
          const command = pending.slice(0, end);
          pending = pending.slice(end + 1);
          const reply = handler(command);
          if (reply !== null) socket.write(`${reply}\n.\n`);
        }
      });
    });
    server.listen(0, '127.0.0.1', () => resolve({ server, port: server.address().port }));
  });
}

describe('调试服务应答解析', () => {
  it('首行以 OK 开头为成功，其余行放在 lines', () => {
    assert.deepEqual(parseReply('OK frames=3\nHEX 00 01\n'), { ok: true, head: 'OK frames=3', lines: ['HEX 00 01'] });
  });

  it('首行以 ERR 开头为失败', () => {
    assert.equal(parseReply('ERR usage: audio fast|real\n').ok, false);
  });

  it('空应答视为失败', () => {
    assert.deepEqual(parseReply(''), { ok: false, head: '', lines: [] });
  });
});

describe('调试客户端', () => {
  let fake;

  afterEach(() => fake?.server.close());

  it('发送命令并读到以“.”行结束的应答，连续发送多条', async () => {
    fake = await startFakeServer((command) => (command === 'status' ? 'OK running=1\nline2' : `ERR unknown ${command}`));
    const client = await DebugClient.connect({ port: fake.port });
    try {
      const first = await client.send('status');
      assert.deepEqual([first.ok, first.head, first.lines], [true, 'OK running=1', ['line2']]);
      const second = await client.send('nope');
      assert.equal(second.ok, false);
      assert.equal(second.head, 'ERR unknown nope');
    } finally {
      client.close();
    }
  });

  it('应答超时时报错并说明是哪条命令', async () => {
    fake = await startFakeServer(() => null);
    const client = await DebugClient.connect({ port: fake.port });
    try {
      await assert.rejects(client.send('run 1', { timeoutMs: 100 }), (error) => error.code === 'DEBUG_TIMEOUT' && error.message.includes('run 1'));
    } finally {
      client.close();
    }
  });

  it('上一条命令没有返回时不允许发送下一条', async () => {
    fake = await startFakeServer(() => null);
    const client = await DebugClient.connect({ port: fake.port });
    try {
      const pending = client.send('run 1', { timeoutMs: 200 });
      assert.throws(() => client.send('status'), (error) => error.code === 'DEBUG_BUSY');
      await assert.rejects(pending, (error) => error.code === 'DEBUG_TIMEOUT');
    } finally {
      client.close();
    }
  });

  it('连接不上时给出怎么启动模拟器的提示', async () => {
    const free = await startFakeServer(() => null);
    const { port } = free;
    await new Promise((resolve) => free.server.close(resolve));
    await assert.rejects(DebugClient.connect({ port }), (error) => error.code === 'DEBUG_CONNECT_FAILED' && error.hint.includes('fcdev run --debug'));
  });
});

describe('端口检查', () => {
  it('被占用的端口报错', async () => {
    const fake = await startFakeServer(() => null);
    try {
      await assert.rejects(assertPortFree(fake.port), (error) => error.code === 'EMULATOR_PORT_BUSY' && error.hint.includes('--port'));
    } finally {
      fake.server.close();
    }
  });

  it('空闲的端口通过', async () => {
    const fake = await startFakeServer(() => null);
    const { port } = fake;
    await new Promise((resolve) => fake.server.close(resolve));
    await assertPortFree(port);
  });
});

describe('模拟器定位', () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('配置项优先于环境变量', () => {
    const configured = writeTestFile(temp.dir, 'a/emu.exe', '');
    const fromEnv = writeTestFile(temp.dir, 'b/emu.exe', '');
    assert.deepEqual(locateEmulator({ configured, env: { FCDEV_EMULATOR: fromEnv } }), { executable: configured, source: 'config' });
  });

  it('指定的路径不存在时报错', () => {
    assert.throws(() => locateEmulator({ configured: path.join(temp.dir, 'none.exe'), env: {} }), (error) => error.code === 'EMULATOR_NOT_FOUND');
  });
});

describe('debug 命令的路径换算', () => {
  const cwd = path.resolve('/work/project');

  it('shot、trace 等命令的路径换成绝对路径', () => {
    assert.deepEqual(resolveCommandPaths(['shot', 'a.png'], cwd), ['shot', path.resolve(cwd, 'a.png')]);
    assert.deepEqual(resolveCommandPaths(['trace', 't.log', '100'], cwd), ['trace', path.resolve(cwd, 't.log'), '100']);
  });

  it('disk 的路径可以含空格', () => {
    assert.deepEqual(resolveCommandPaths(['disk', 'my', 'disk.img'], cwd), ['disk', path.resolve(cwd, 'my disk.img')]);
  });

  it('cdl save 的前缀换成绝对路径', () => {
    assert.deepEqual(resolveCommandPaths(['cdl', 'save', 'out/boot'], cwd), ['cdl', 'save', path.resolve(cwd, 'out/boot')]);
  });

  it('off 与不含路径的命令保持原样', () => {
    assert.deepEqual(resolveCommandPaths(['trace', 'off'], cwd), ['trace', 'off']);
    assert.deepEqual(resolveCommandPaths(['read', 'cpu', '8000', '16'], cwd), ['read', 'cpu', '8000', '16']);
  });
});

/** 找真实模拟器；没有时返回 null */
function findRealEmulator() {
  if (process.env.FCDEV_SKIP_GUI_TESTS === '1') return null;
  try {
    return locateEmulator();
  } catch {
    return null;
  }
}

const realEmulator = findRealEmulator();

describe('真实模拟器', { skip: realEmulator && !SKIP_WITHOUT_TOOLCHAIN ? false : '没有模拟器或 cc65 工具链，或设置了 FCDEV_SKIP_GUI_TESTS' }, () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('NES 程序在调试模式下运行 60 帧后能截图', async () => {
    writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'nes' }));
    writeTestFile(temp.dir, 'hello.c', '#include <fcnes.h>\nint main(void) { nes_screen_off(); nes_print(1, 1, "TEST"); nes_wait_vblank(); nes_screen_on(); for (;;) { } }\n');
    const { result } = await buildPath({ pathArg: 'hello.c', cwd: temp.dir });
    const probe = await startFakeServer(() => null);
    const { port } = probe;
    await new Promise((resolve) => probe.server.close(resolve));

    const { client } = await launchEmulator({ executable: realEmulator.executable, romFile: result.outputFile, debug: true, port });
    try {
      const run = await client.send('run 60', { timeoutMs: 60_000 });
      assert.ok(run.ok, run.head);
      assert.match(run.head, /frames=60/);
      const shot = path.join(temp.dir, 'shot.png');
      assert.ok((await client.send(`shot ${shot}`)).ok);
      assert.equal(readFileSync(shot).subarray(1, 4).toString('latin1'), 'PNG');
    } finally {
      await client.send('quit').catch(() => {});
      client.close();
    }
  });
});
