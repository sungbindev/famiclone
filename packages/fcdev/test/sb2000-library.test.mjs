// ------------------------------------------------------------------------
// 名称：sb2000-library.test.mjs
// 说明：测试 SB2000 运行库的各个模块：每个模块一个测试程序，在模拟器里运行，核对函数的实际效果。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：需要模拟器、cc65 与环境变量 FCDEV_TEST_FIRMWARE（SB2000 的 BIOS），没有时跳过；测试程序在 test/fixtures/sb2000/。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { listFat12, readFat12File } from '../src/packagers/index.mjs';
import { createTempDir } from './helpers.mjs';
import { imageFile, SCAN, SKIP_WITHOUT_SB2000_RUNTIME, startSb2000Program } from './sb2000-runner.mjs';

/** 磁盘操作比较慢，文件类测试程序最多运行的帧数 */
const DISK_MAX_FRAMES = 80000;

/**
 * 运行一个测试程序，要求它走到结束且全部检查通过。
 * @param {{dir: string}} temp 临时目录
 * @param {object} options startSb2000Program 的选项
 * @param {number} expectedCount 应该执行的检查次数
 * @param {(session: import('./sb2000-runner.mjs').Sb2000Session) => Promise<void>} [verify] 结束后对会话的额外核对
 * @returns {Promise<void>}
 */
async function runToDone(temp, options, expectedCount, verify) {
  const session = await startSb2000Program(temp, options);
  try {
    assert.equal(await session.runToStop(DISK_MAX_FRAMES), 'done');
    await session.assertAllPassed(expectedCount);
    if (verify) await verify(session);
  } finally {
    await session.close();
  }
}

/** 测试用的大文件：第 i 个字节是 i*7+1 */
function makeBigFile() {
  const big = Buffer.alloc(10000);
  for (let index = 0; index < big.length; index += 1) big[index] = (index * 7 + 1) & 0xff;
  return big;
}

describe('SB2000 运行库（在模拟器里运行）', { skip: SKIP_WITHOUT_SB2000_RUNTIME }, () => {
  let temp;

  beforeEach(() => {
    temp = createTempDir();
  });

  afterEach(() => temp.cleanup());

  it('系统基础服务', async () => {
    await runToDone(temp, { sources: ['system.c', 'system-hook.s'] }, 43);
  });

  it('内存服务', async () => {
    await runToDone(temp, { sources: ['memory.c'] }, 21);
  });

  it('文字输出', async () => {
    const session = await startSb2000Program(temp, { sources: ['text.c'] });
    try {
      const shots = [];
      for (;;) {
        if ((await session.runToStop()) === 'done') break;
        shots.push(await session.shot());
      }
      await session.assertAllPassed(17);
      assert.equal(shots.length, 5);
      assert.ok(!shots[1].equals(shots[0]), '显示 HELLO 后画面应变化');
      assert.ok(shots[2].equals(shots[1]), '同样的字符再画一次画面应不变');
      assert.ok(!shots[3].equals(shots[2]), '换一个字符画面应变化');
      assert.ok(!shots[4].equals(shots[3]), '显示汉字后画面应变化');
    } finally {
      await session.close();
    }
  });

  it('键盘、手柄与鼠标', async () => {
    const session = await startSb2000Program(temp, { sources: ['input.c'] });
    try {
      /** 每个停靠点之后脚本要注入的动作，顺序与 input.c 里的 sb_test_sync 一一对应 */
      const actions = [
        async () => { await session.tap(SCAN.a); await session.tap(SCAN.b); },
        async () => { await session.tap(SCAN.c); },
        async () => { await session.tap(SCAN.a); await session.tap(SCAN.b); await session.tap(SCAN.c); },
        async () => { await session.tap(0x3b); },
        async () => { await session.command('pad 0 01'); },
        async () => { await session.command('pad 0 00'); },
        async () => { await session.command('pad 0 02'); },
        async () => { await session.command('mouse move 10 5'); },
        async () => { await session.command('mouse btn 4'); },
        async () => { await session.command('mouse btn 0'); },
        async () => { await session.tap(0x01); },
      ];
      for (const action of actions) {
        assert.equal(await session.runToStop(), 'sync');
        await action();
      }
      assert.equal(await session.runToStop(), 'done');
      await session.assertAllPassed(25);
    } finally {
      await session.close();
    }
  });

  it('读文件', async () => {
    await runToDone(temp, {
      sources: ['file-read.c'],
      extraFiles: [imageFile('HELLO.TXT', 'Hello, SB2000!\n'), imageFile('BIG.BIN', makeBigFile())],
    }, 17);
  });

  it('写文件、改名、删除', async () => {
    const big = makeBigFile();
    await runToDone(temp, { sources: ['file-write.c'], extraFiles: [imageFile('BIG.BIN', big)] }, 34, async (session) => {
      // 程序留下的 KEEP.DAT 与改写过的 BIG.BIN 用独立的 FAT12 解析核对，被删除与改名的文件不应留在目录里
      const image = await session.saveDisk();
      const files = listFat12(image);
      const names = files.map((file) => file.name);
      const keep = files.find((file) => file.name === 'KEEP.DAT');
      assert.ok(keep, `镜像里应有 KEEP.DAT，实际有 ${names.join(' ')}`);
      const written = (index) => (5 + index * 3) & 0xff;
      const expectedKeep = Buffer.alloc(700);
      for (let index = 0; index < 700; index += 1) expectedKeep[index] = written(index);
      assert.deepEqual(readFat12File(image, keep), expectedKeep);
      const expectedBig = Buffer.from(big);
      for (let index = 0; index < 4100; index += 1) expectedBig[3000 + index] = written(index);
      assert.deepEqual(readFat12File(image, files.find((file) => file.name === 'BIG.BIN')), expectedBig);
      for (const gone of ['NEW.TXT', 'NEW2.TXT']) assert.ok(!names.includes(gone), `${gone} 应已不存在`);
    });
  });

  it('目录、日期时间与运行子程序', async () => {
    const big = makeBigFile();
    await runToDone(temp, {
      sources: ['dir.c'],
      extraFiles: [imageFile('HELLO.TXT', 'Hello, SB2000!\n'), imageFile('BIG.BIN', big)],
      children: [{ name: 'child', source: 'child.c' }],
    }, 26, async (session) => {
      const image = await session.saveDisk();
      const files = listFat12(image);
      const names = files.map((file) => file.name);
      assert.ok(names.includes('KEEPDIR'), `镜像里应有目录 KEEPDIR，实际有 ${names.join(' ')}`);
      assert.ok(!names.includes('SUB'), 'SUB 应已删除');
      assert.deepEqual(readFat12File(image, files.find((file) => file.name === 'BIG.BIN')), big);
    });
  });

  it('数学服务', async () => {
    await runToDone(temp, { sources: ['math.c'] }, 32);
  });

  it('声音与语音', async () => {
    await runToDone(temp, { sources: ['sound.c'] }, 13);
  });

  it('打印机与输入法', async () => {
    const session = await startSb2000Program(temp, { sources: ['peripheral.c'] });
    try {
      assert.equal(await session.runToStop(DISK_MAX_FRAMES), 'sync');
      await session.tap(SCAN.a);
      await session.tap(SCAN[1]);
      assert.equal(await session.runToStop(DISK_MAX_FRAMES), 'done');
      await session.assertAllPassed(26);
    } finally {
      await session.close();
    }
  });

  it('显示服务', async () => {
    await runToDone(temp, { sources: ['display.c'] }, 40);
  });

  it('通用 DMA', async () => {
    await runToDone(temp, { sources: ['dma.c'] }, 20);
  });

  it('图形端口', async () => {
    await runToDone(temp, { sources: ['ppu.c'] }, 12);
  });

  it('硬件定时器', async () => {
    await runToDone(temp, { sources: ['timer.c'] }, 12);
  });

  it('磁盘扇区服务', async () => {
    await runToDone(temp, { sources: ['disk.c'] }, 30, async (session) => {
      // 读回镜像，核对程序直接写下的扇区与格式化的磁道（逻辑扇区号 = (磁道 × 2 + 面) × 18 + 扇区 - 1）
      const image = await session.saveDisk();
      const sectorAt = (lba) => image.subarray(lba * 512, (lba + 1) * 512);
      const single = Buffer.alloc(512);
      for (let index = 0; index < 512; index += 1) single[index] = (index * 5 + 3) & 0xff;
      assert.deepEqual(sectorAt(2542), single);
      for (let n = 0; n < 20; n += 1) {
        const expected = Buffer.alloc(512);
        for (let index = 0; index < 512; index += 1) expected[index] = (n * 7 + index * 3 + 1) & 0xff;
        assert.deepEqual(sectorAt(2572 + n), expected, `成批写入的第 ${n} 个扇区`);
      }
      for (let lba = 2862; lba <= 2879; lba += 1) {
        assert.deepEqual(sectorAt(lba), Buffer.alloc(512, 0xf6), `格式化后的逻辑扇区 ${lba}`);
      }
    });
  });

  it('换页：9 个库页的大程序，经蹦床调用库页函数', async () => {
    await runToDone(temp, { sources: ['bank.c', 'bank-pages.c', 'bank-pages.h'], targetOptions: { banks: 9 } }, 24);
  });

  it('换页：库页个数取上限 113，程序 116 页', async () => {
    await runToDone(temp, { sources: ['bank-top.c'], targetOptions: { banks: 113 } }, 5);
  });
});
