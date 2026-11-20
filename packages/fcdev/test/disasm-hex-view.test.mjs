// ------------------------------------------------------------------------
// 名称：disasm-hex-view.test.mjs
// 说明：测试十六进制视图：按 CPU 地址对齐的行、范围头尾补位、字符视图、符号标注、换页段，以及参数错误。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { describe, it } from 'node:test';
import { buildHexRows, createLayout, renderHexView } from '../src/disasm/index.mjs';

/** 16 字节的文件头加 0x40 字节的程序：内容是 0x00-0x3F，其中放一段文字 */
function makeImage() {
  const bytes = new Uint8Array(0x10 + 0x40);
  for (let index = 0; index < 0x40; index += 1) bytes[0x10 + index] = index;
  bytes.set(Buffer.from('Hi!'), 0x10 + 0x12);
  const layout = createLayout([{ id: 0, fileOffset: 0x10, length: 0x40, address: 0xc000 }], { imageSize: bytes.length });
  return { bytes, layout };
}

describe('十六进制视图', () => {
  it('整段：每行 16 字节，地址、字节、字符视图', () => {
    const { bytes, layout } = makeImage();
    const lines = renderHexView({ bytes, layout, segmentId: 0 }).split('\n');
    assert.equal(lines[0], '; 段 0，$C000-$C03F，文件偏移 $10');
    assert.equal(lines.length, 1 + 4);
    assert.equal(lines[1], 'C000  00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  |................|');
    assert.equal(lines[2], 'C010  10 11 48 69 21 15 16 17  18 19 1A 1B 1C 1D 1E 1F  |..Hi!...........|');
  });

  it('范围的头尾不满一行时按地址对齐并补空位', () => {
    const { bytes, layout } = makeImage();
    const { rows, start, end } = buildHexRows({ bytes, layout, segmentId: 0, address: 0xc00e, length: 0x14 });
    assert.deepEqual([start, end], [0xc00e, 0xc022]);
    assert.deepEqual(rows.map((row) => row.address), [0xc000, 0xc010, 0xc020]);
    assert.deepEqual(rows[0].bytes.slice(0, 15), [...Array(14).fill(null), 0x0e]);
    assert.equal(rows[0].text, `${' '.repeat(14)}..`);
    assert.equal(rows[2].bytes[2], null);
    const text = renderHexView({ bytes, layout, segmentId: 0, address: 0xc00e, length: 0x14 }).split('\n');
    assert.equal(text[1], `C000  ${' '.repeat(43)}0E 0F  |${' '.repeat(14)}..|`);
  });

  it('长度超出段尾时截到段尾', () => {
    const { bytes, layout } = makeImage();
    const { end, rows } = buildHexRows({ bytes, layout, segmentId: 0, address: 0xc030, length: 0x100 });
    assert.equal(end, 0xc040);
    assert.equal(rows.length, 1);
  });

  it('可调每行字节数', () => {
    const { bytes, layout } = makeImage();
    const { rows } = buildHexRows({ bytes, layout, segmentId: 0, bytesPerRow: 8, length: 16 });
    assert.equal(rows.length, 2);
    assert.equal(rows[0].bytes.length, 8);
  });

  it('符号写在所在行的末尾', () => {
    const { bytes, layout } = makeImage();
    const labels = new Map([[0xc012, 'greeting'], [0xc015, 'after'], [0xc030, 'outside']]);
    const lines = renderHexView({ bytes, layout, segmentId: 0, address: 0xc000, length: 0x20, labels }).split('\n');
    assert.ok(lines[2].endsWith('  ; $C012 greeting，$C015 after'));
    assert.equal(lines.some((line) => line.includes('outside')), false);
  });

  it('换页的段：说明里写 bank，地址按段的 CPU 地址，字节取自各自的文件位置', () => {
    const bytes = Uint8Array.from([0xaa, 0xaa, 0xaa, 0xaa, 0xbb, 0xbb, 0xbb, 0xbb]);
    const layout = createLayout([
      { id: 0, fileOffset: 0, length: 4, address: 0x8000, bank: 0 },
      { id: 1, fileOffset: 4, length: 4, address: 0x8000, bank: 1 },
    ], { imageSize: 8 });
    const lines = renderHexView({ bytes, layout, segmentId: 1 }).split('\n');
    assert.equal(lines[0], '; 段 1，bank 1，$8000-$8003，文件偏移 $4');
    assert.ok(lines[1].startsWith('8000  BB BB BB BB'));
  });

  it('段不存在、起点不在段内、参数不合法时报错', () => {
    const { bytes, layout } = makeImage();
    const rejected = (options, text) => assert.throws(() => buildHexRows({ bytes, layout, segmentId: 0, ...options }), (error) => error.code === 'DISASM_RANGE_INVALID' && error.message.includes(text));
    rejected({ segmentId: 7 }, '没有段 7');
    rejected({ address: 0xbfff }, '不在段 0');
    rejected({ address: 0xc040 }, '不在段 0');
    rejected({ length: 0 }, '字节数');
    rejected({ bytesPerRow: 0 }, '每行字节数');
    rejected({ bytesPerRow: 65 }, '每行字节数');
  });
});
