// ------------------------------------------------------------------------
// 名称：sb2000-runner.mjs
// 说明：SB2000 运行库测试的公共工具：构建测试程序，放进软盘镜像，在模拟器里运行，读出测试程序记录的检查结果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：测试程序放在 test/fixtures/sb2000/，用 sb-test.h 记录检查结果；需要环境变量 FCDEV_TEST_FIRMWARE 指向 SB2000 的 BIOS，没有时由调用者跳过。
//       调试端口自动选空闲端口，只结束自己启动的模拟器，不影响其他在运行的实例。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { copyFileSync, existsSync, mkdirSync, readFileSync } from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { buildPath } from '../src/build/index.mjs';
import { launchEmulator, locateEmulator } from '../src/emulator/index.mjs';
import { getDiskPackager } from '../src/packagers/index.mjs';
import { SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** 测试程序目录 */
const FIXTURE_DIR = fileURLToPath(new URL('./fixtures/sb2000/', import.meta.url));

/** DirectInput 扫描码：字母、数字与回车 */
export const SCAN = {
  enter: 0x1c, space: 0x39,
  0: 0x0b, 1: 0x02, 2: 0x03, 3: 0x04, 4: 0x05, 5: 0x06, 6: 0x07, 7: 0x08, 8: 0x09, 9: 0x0a,
  q: 0x10, w: 0x11, e: 0x12, r: 0x13, t: 0x14, y: 0x15, u: 0x16, i: 0x17, o: 0x18, p: 0x19,
  a: 0x1e, s: 0x1f, d: 0x20, f: 0x21, g: 0x22, h: 0x23, j: 0x24, k: 0x25, l: 0x26,
  z: 0x2c, x: 0x2d, c: 0x2e, v: 0x2f, b: 0x30, n: 0x31, m: 0x32,
};

/** 测试程序名，必须是 8.3 名字 */
const PROGRAM_NAME = 'sbtest';

/**
 * 找模拟器与固件都可用的真实环境。
 * @returns {{firmware: string, emulator: {executable: string}}|null} 不可用时为 null
 */
export function findSb2000Setup() {
  const firmware = process.env.FCDEV_TEST_FIRMWARE;
  if (!firmware || !existsSync(firmware) || process.env.FCDEV_SKIP_GUI_TESTS === '1' || SKIP_WITHOUT_TOOLCHAIN) return null;
  try {
    return { firmware, emulator: locateEmulator() };
  } catch {
    return null;
  }
}

/** 测试跳过说明：环境齐全时为 false */
export const SKIP_WITHOUT_SB2000_RUNTIME = findSb2000Setup() ? false : '需要模拟器、cc65 与环境变量 FCDEV_TEST_FIRMWARE（SB2000 的 BIOS）';

/**
 * 找一个空闲端口。
 * @returns {Promise<number>} 端口
 */
function findFreePort() {
  return new Promise((resolve) => {
    const server = net.createServer().listen(0, '127.0.0.1', () => {
      const { port } = server.address();
      server.close(() => resolve(port));
    });
  });
}

/**
 * 从链接器的调试信息里取符号地址（映射文件只列被其他模块引用的导出）。
 * @param {string} debugText 调试信息文件内容
 * @param {string} symbol 符号名，含前导下划线
 * @returns {number} 地址
 */
function findSymbol(debugText, symbol) {
  const match = new RegExp(`^sym\\b.*name="${symbol}",.*?\\bval=0x([0-9A-Fa-f]+)`, 'm').exec(debugText);
  assert.ok(match, `调试信息里没有符号 ${symbol}`);
  return parseInt(match[1], 16) & 0xffff;
}

/** 一次测试运行：封装模拟器与测试程序的符号 */
export class Sb2000Session {
  /**
   * @param {object} parts 组成部分
   * @param {import('../src/emulator/index.mjs').DebugClient} parts.client 调试客户端
   * @param {Record<string, number>} parts.symbols 符号地址
   * @param {string} parts.imageFile 镜像路径
   */
  constructor({ client, symbols, imageFile }) {
    this.client = client;
    this.symbols = symbols;
    this.imageFile = imageFile;
  }

  /**
   * 发命令，要求应答 OK。
   * @param {string} command 命令
   * @param {{timeoutMs?: number}} [options] 超时
   * @returns {Promise<import('../src/emulator/index.mjs').DebugReply>} 应答
   */
  async command(command, options) {
    const reply = await this.client.send(command, options);
    assert.ok(reply.ok, `命令“${command}”失败：${reply.head}`);
    return reply;
  }

  /**
   * 按下并松开一个键，期间运行若干帧让 BIOS 读到。
   * @param {number} scancode 扫描码
   * @returns {Promise<void>}
   */
  async tap(scancode) {
    await this.client.send(`key ${scancode.toString(16)} 1`);
    await this.client.send('run 5');
    await this.client.send(`key ${scancode.toString(16)} 0`);
    await this.client.send('run 4');
  }

  /**
   * 依次敲一串字符。
   * @param {string} text 只含 SCAN 里有的字符
   * @returns {Promise<void>}
   */
  async type(text) {
    for (const char of text) await this.tap(SCAN[char]);
  }

  /**
   * 读 CPU 地址空间的一段内存。
   * @param {number} address 起始地址
   * @param {number} length 字节数
   * @returns {Promise<Buffer>} 内容
   */
  async read(address, length) {
    const reply = await this.command(`read cpu ${address.toString(16)} ${length}`);
    return Buffer.from(reply.lines.map((line) => line.replace(/^HEX /, '')).join(''), 'hex');
  }

  /**
   * 读测试程序里的一个全局变量。
   * @param {string} name 变量名，不含下划线
   * @param {number} length 字节数
   * @returns {Promise<Buffer>} 内容
   */
  readVariable(name, length) {
    return this.read(this.symbols[`_${name}`], length);
  }

  /**
   * 运行到测试程序的停靠点（sb_test_sync 或 sb_test_done）。
   * @param {number} [maxFrames] 最多运行的帧数
   * @returns {Promise<'sync'|'done'>} 停在哪个停靠点
   */
  async runToStop(maxFrames = 6000) {
    for (let done = 0; done < maxFrames;) {
      const reply = await this.client.send('run 300', { timeoutMs: 120_000 });
      done += Number(/frames=(\d+)/.exec(reply.head)?.[1] ?? 300);
      const stop = /stop=(?!none)\w+/.exec(reply.head);
      if (!stop) continue;
      // DOS 外壳也在 $8000 窗口里运行，可能写到同一地址；窗口里是本程序（文件头魔数 AA 55）时才算到达
      const header = await this.read(0x8000, 5);
      if (header[3] !== 0xaa || header[4] !== 0x55) continue;
      const [flag] = await this.readVariable('sb_test_stop', 1);
      if (flag === 0) continue; // 启动代码清零 BSS 时也写这个单元
      if (flag === 1) return 'done';
      if (flag === 2) return 'sync';
      assert.fail(`停在了意外的位置：${reply.head}`);
    }
    assert.fail(`运行 ${maxFrames} 帧后测试程序仍没有到达停靠点`);
  }

  /**
   * 读出测试程序记录的全部检查结果。
   * @returns {Promise<{ok: boolean, line: number, value: number}[]>} 检查结果
   */
  async readChecks() {
    const [count] = await this.readVariable('sb_test_count', 1);
    const max = 80;
    const ok = await this.readVariable('sb_test_ok', max);
    const lines = await this.readVariable('sb_test_line', max * 2);
    const values = await this.readVariable('sb_test_value', max * 2);
    return Array.from({ length: Math.min(count, max) }, (_, index) => ({
      ok: ok[index] === 1,
      line: lines.readUInt16LE(index * 2),
      value: values.readUInt16LE(index * 2),
    }));
  }

  /**
   * 断言全部检查通过；失败时列出源文件行号与实际值。
   * @param {number} expectedCount 应该执行的检查次数
   * @returns {Promise<void>}
   */
  async assertAllPassed(expectedCount) {
    const checks = await this.readChecks();
    const failed = checks.filter((check) => !check.ok).map((check) => `第 ${check.line} 行（实际值 ${check.value}，$${check.value.toString(16)}）`);
    assert.deepEqual(failed, [], `失败的检查：${failed.join('；')}`);
    assert.equal(checks.length, expectedCount, `应执行 ${expectedCount} 次检查，实际 ${checks.length} 次`);
  }

  /**
   * 把软盘镜像保存回文件并读出，用来核对程序对磁盘的修改。
   * @returns {Promise<Buffer>} 镜像内容
   */
  async saveDisk() {
    await this.command('savedisk');
    return readFileSync(this.imageFile);
  }

  /**
   * 截取当前画面。
   * @returns {Promise<Buffer>} PNG 文件内容
   */
  async shot() {
    this.shotCount = (this.shotCount ?? 0) + 1;
    const file = path.join(path.dirname(this.imageFile), `shot-${this.shotCount}.png`);
    await this.command(`shot ${file}`);
    return readFileSync(file);
  }

  /** 关闭模拟器 */
  async close() {
    await this.client.send('quit').catch(() => {});
    this.client.close();
  }
}

/**
 * 构建测试程序，放进软盘镜像并在模拟器里启动到 DOS 里运行它。
 * 返回时测试程序已经开始运行，调用 session.runToStop() 运行到停靠点。
 * @param {{dir: string}} temp 临时目录
 * @param {{sources: string[], extraFiles?: {name: string, data: Buffer}[], children?: {name: string, source: string}[], targetOptions?: Record<string, unknown>}} options sources 为 fixtures/sb2000 下的测试程序文件名，extraFiles 为一并放进镜像的文件，children 为另外构建、一并放进镜像的子程序（name 是 8.3 主名），targetOptions 写进 fcdev.json
 * @returns {Promise<Sb2000Session>} 会话
 */
export async function startSb2000Program(temp, { sources, extraFiles = [], children = [], targetOptions = {} }) {
  const setup = findSb2000Setup();
  assert.ok(setup, '没有可用的模拟器与固件');
  writeTestFile(temp.dir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: PROGRAM_NAME, firmware: setup.firmware, targetOptions }));
  mkdirSync(path.join(temp.dir, 'src'), { recursive: true });
  for (const name of ['sb-test.h', 'sb-test.c', ...sources]) {
    copyFileSync(path.join(FIXTURE_DIR, name), path.join(temp.dir, 'src', name));
  }
  const { plan, result } = await buildPath({ pathArg: 'src', cwd: temp.dir });

  const childFiles = [];
  for (const child of children) {
    const childDir = path.join(temp.dir, `child-${child.name}`);
    writeTestFile(childDir, 'fcdev.json', JSON.stringify({ target: 'sb2000', name: child.name }));
    mkdirSync(path.join(childDir, 'src'), { recursive: true });
    copyFileSync(path.join(FIXTURE_DIR, child.source), path.join(childDir, 'src', child.source));
    const built = await buildPath({ pathArg: 'src', cwd: childDir });
    childFiles.push({ name: path.basename(built.result.outputFile), data: readFileSync(built.result.outputFile) });
  }

  const imageFile = path.join(temp.dir, 'test.img');
  getDiskPackager('fat-floppy')({
    files: [{ name: path.basename(result.outputFile), data: readFileSync(result.outputFile) }, ...childFiles, ...extraFiles],
    outputFile: imageFile,
    options: { label: 'FCDEV' },
  });

  const debugText = readFileSync(path.join(plan.objectDir, `${PROGRAM_NAME}.dbg`), 'utf8');
  const symbols = {};
  for (const symbol of ['_sb_test_stop', '_sb_test_count', '_sb_test_ok', '_sb_test_line', '_sb_test_value']) {
    symbols[symbol] = findSymbol(debugText, symbol);
  }

  const port = await findFreePort();
  const { client } = await launchEmulator({ executable: setup.emulator.executable, romFile: setup.firmware, debug: true, port });
  const session = new Sb2000Session({ client, symbols, imageFile });
  try {
    await session.command(`disk ${imageFile}`);
    await session.command('reset');
    await client.send('run 1300', { timeoutMs: 120_000 });
    await session.tap(SCAN.enter);
    await client.send('run 60');
    await session.tap(SCAN.enter);
    await client.send('run 400');
    // 断点要在输入完程序名之后再设：设早了会打断按键所需的帧数
    await session.type(PROGRAM_NAME);
    await session.tap(SCAN.enter);
    await session.command(`bp w ${symbols._sb_test_stop.toString(16)}`);
  } catch (error) {
    await session.close();
    throw error;
  }
  return session;
}

/**
 * 在临时目录写入一个文本文件的镜像条目，便于给文件类测试提供数据。
 * @param {string} name 8.3 文件名
 * @param {string|Buffer} content 内容
 * @returns {{name: string, data: Buffer}} 镜像文件项
 */
export function imageFile(name, content) {
  return { name, data: Buffer.isBuffer(content) ? content : Buffer.from(content) };
}
