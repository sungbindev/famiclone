// ------------------------------------------------------------------------
// 名称：sb97-rom.test.mjs
// 说明：sb97-rom.mjs 的单元测试：文件头解析、分页视图、向量与页判别；并用真实 BIOS 文件核对已确认的事实。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-17
// 备注：真实文件的用例只检查文件头与向量这类不会随分析变化的事实。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { test } from 'node:test';
import { ROM_PATH } from '../tools/sb97-scan.mjs';
import { PAGE_SIZE, classifyPage, createCpuView, loadRom, pageVectors, parseRom } from '../tools/sb97-rom.mjs';

/** 构造一个只含文件头与若干页（每页填同一个字节）的文件。 */
function buildFile({ header, pageFills }) {
  const bytes = new Uint8Array(16 + pageFills.length * PAGE_SIZE);
  bytes.set(header);
  pageFills.forEach((fill, index) => bytes.fill(fill, 16 + index * PAGE_SIZE, 16 + (index + 1) * PAGE_SIZE));
  return bytes;
}

const HEADER_NES2 = [0x4e, 0x45, 0x53, 0x1a, 0x02, 0x00, 0xe1, 0x68, 0x01, 0x00, 0x00, 0x07];

test('NES 2.0 文件头解析出扩展的 mapper 号、CHR-RAM 大小与镜像', () => {
  const rom = parseRom(buildFile({ header: HEADER_NES2, pageFills: [0x11, 0x22] }));
  assert.equal(rom.header.nes2, true);
  assert.equal(rom.header.mapper, 0x16e);
  assert.equal(rom.header.chrRamSize, 8192);
  assert.equal(rom.header.verticalMirroring, true);
  assert.equal(rom.pages.length, 2);
  assert.equal(rom.pages[1][0], 0x22);
});

test('缺少文件头标记或长度不够时报错', () => {
  assert.throws(() => parseRom(new Uint8Array(32)), /文件头标记/);
  const truncated = buildFile({ header: HEADER_NES2, pageFills: [0x11] });
  assert.throws(() => parseRom(truncated), /长度不够/);
});

test('CPU 视图：$C000 起是固定页，$8000 起是当前窗口页', () => {
  const rom = parseRom(buildFile({ header: HEADER_NES2, pageFills: [0xaa, 0xbb] }));
  const view = createCpuView(rom, 1);
  assert.equal(view.read(0xc000), 0xaa);
  assert.equal(view.read(0xffff), 0xaa);
  assert.equal(view.read(0x8000), 0xbb);
  assert.equal(view.read(0x2000), 0);
  assert.equal(view.isRom(0x7fff), false);
  assert.equal(view.isRom(0x8000), true);
});

test('页判别：全同一个字节的页是 empty', () => {
  assert.equal(classifyPage(new Uint8Array(PAGE_SIZE).fill(0xff)).kind, 'empty');
});

test('页判别：随机数据不是 code-like', () => {
  const page = new Uint8Array(PAGE_SIZE);
  let seed = 12345;
  for (let i = 0; i < page.length; i += 1) {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    page[i] = seed >> 16;
  }
  assert.equal(classifyPage(page).kind, 'other');
});

test('页判别：由 JSR、RTS 和 STA 组成的重复片段是 code-like', () => {
  const page = new Uint8Array(PAGE_SIZE);
  const chunk = [0xa9, 0x01, 0x20, 0x00, 0x90, 0x8d, 0x00, 0x20, 0x60, 0x4c, 0x00, 0xc0];
  for (let i = 0; i + chunk.length <= page.length; i += chunk.length) page.set(chunk, i);
  assert.equal(classifyPage(page).kind, 'code-like');
});

test('向量按页尾读出', () => {
  const page = new Uint8Array(PAGE_SIZE);
  page.set([0x34, 0x12, 0x00, 0xff, 0xa0, 0x00], 0x3ffa);
  assert.deepEqual(pageVectors(page), { nmi: 0x1234, reset: 0xff00, irq: 0x00a0 });
});

test('真实 BIOS：文件头是 NES 2.0、mapper 366、1MB、8KB CHR-RAM', () => {
  const rom = loadRom(ROM_PATH);
  assert.equal(rom.header.nes2, true);
  assert.equal(rom.header.mapper, 366);
  assert.equal(rom.header.prgPages, 64);
  assert.equal(rom.header.chrPages, 0);
  assert.equal(rom.header.chrRamSize, 8192);
  assert.equal(rom.header.verticalMirroring, true);
});

test('真实 BIOS：第 0 页是固定页，复位向量 $FF00，第 5 页的复位向量 $C000', () => {
  const rom = loadRom(ROM_PATH);
  assert.equal(pageVectors(rom.pages[0]).reset, 0xff00);
  assert.equal(pageVectors(rom.pages[5]).reset, 0xc000);
});
