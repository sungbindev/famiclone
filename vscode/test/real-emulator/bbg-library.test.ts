// ------------------------------------------------------------------------
// 名称：bbg-library.test.ts
// 说明：测试步步高汇编运行库 bbg.lib 的各个模块：每个模块一个测试程序，
//       在 Ember 的 bbg1（BBGDOS 3.0 系统盘）里运行，核对函数的实际效果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-24
// 备注：整机经 Ember 的调试服务启动（见 support/real-emulator.ts）；
//       环境不齐（Ember、cc65、步步高 BIOS 与系统盘）时整组跳过；
//       测试程序在 test/fixtures/bbg/，检查编号与程序里的 CHECK_ 宏一一对应。
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { afterEach, beforeEach, describe, it } from 'vitest';
import { createTempDir, type TempDir } from '../support/build-fixtures';
import { CAN_RUN_REAL_BBG, KEY, startBbgProgram, type Sb2000Session } from '../support/real-emulator';

/** 磁盘操作比较慢，测试程序最多运行的帧数。 */
const MAX_FRAMES = 60_000;

/**
 * 运行一个测试程序，要求它走到结束且全部检查通过。
 * @param temp 临时目录。
 * @param source fixtures/bbg 下的测试程序文件名。
 * @param expectedCount 应该执行的检查次数。
 */
async function runToDone(temp: TempDir, source: string, expectedCount: number): Promise<void> {
  const session = await startBbgProgram(temp, [source]);
  try {
    assert.equal(await session.runToStopPolled(MAX_FRAMES), 'done');
    await session.assertAllPassed(expectedCount);
  } finally {
    session.close();
  }
}

/**
 * 运行到下一个 sync 停靠点并截图，然后让程序继续。
 * @param session 会话。
 * @returns 截图。
 */
async function shotAtSync(session: Sb2000Session): Promise<Buffer> {
  assert.equal(await session.runToStopPolled(MAX_FRAMES), 'sync');
  const shot = await session.shot();
  await session.resume();
  return shot;
}

describe.skipIf(!CAN_RUN_REAL_BBG)('步步高汇编运行库（在 Ember 里运行）', { timeout: 300_000 }, () => {
  let temp: TempDir;

  beforeEach(() => {
    temp = createTempDir();
  });
  afterEach(() => temp.cleanup());

  it('BIOS 调用封装', async () => {
    await runToDone(temp, 'bios.s', 9);
  });

  it('文件调用封装', async () => {
    await runToDone(temp, 'file.s', 77);
  });

  it('文字输出：库函数显示的内容与直接显示等价的字符串一致', async () => {
    const session = await startBbgProgram(temp, ['text.s']);
    try {
      for (let group = 1; group <= 6; group += 1) {
        const byLibrary = await shotAtSync(session);
        const byString = await shotAtSync(session);
        assert.ok(byLibrary.equals(byString), `第 ${group} 组：库函数与直接显示的画面应相同`);
      }
      assert.equal(await session.runToStopPolled(MAX_FRAMES), 'done');
      await session.assertAllPassed(5);
    } finally {
      session.close();
    }
  });

  it('运行程序封装：运行子程序、取退出码，路径版本同样可用', async () => {
    await runToDone(temp, 'run.s', 8);
  });

  it('DOS 调用封装：显示、按键与输入一行', async () => {
    const session = await startBbgProgram(temp, ['dos.s']);
    try {
      const before = await shotAtSync(session); // 显示之前
      const after = await shotAtSync(session); // 显示 A 与 HELLO 之后
      assert.ok(!before.equals(after), '显示字符与字符串后画面应变化');
      assert.equal(await session.runToStopPolled(MAX_FRAMES), 'sync'); // 停靠点 3：等待按键 A（不回显）
      await session.resume();
      await session.tap(KEY.a!);
      assert.equal(await session.runToStopPolled(MAX_FRAMES), 'sync'); // 停靠点 4：等待按键 B（回显）
      await session.resume();
      await session.tap(KEY.b!);
      assert.equal(await session.runToStopPolled(MAX_FRAMES), 'sync'); // 停靠点 5：输入 hi 回车
      await session.resume();
      await session.tap('KeyH');
      await session.tap('KeyI');
      await session.tap(KEY.enter!);
      assert.equal(await session.runToStopPolled(MAX_FRAMES), 'sync'); // 停靠点 6：先按 Z 进缓冲，再按 Q
      await session.tap('KeyZ');
      await session.resume();
      await session.tap('KeyQ');
      const cleared = await shotAtSync(session); // 清屏之后
      assert.ok(!cleared.equals(after), '清屏后画面应与显示 HELLO 时不同');
      assert.equal(await session.runToStopPolled(MAX_FRAMES), 'done');
      await session.assertAllPassed(18);
    } finally {
      session.close();
    }
  });
});
