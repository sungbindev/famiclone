// ------------------------------------------------------------------------
// 名称：disasm-layout.test.ts
// 说明：测试映像布局：段的检查、换页窗口、地址与文件偏移的换算、未覆盖的字节。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { describe, it } from 'vitest';
import type { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { addressToOffset, createLayout, findSegmentsAt, findUncovered, getSegment, locateOffset } from '../../../src/shared/disassembly/model/layout';

/** 断言抛出布局错误，且说明里含有给定文字 */
function assertInvalid(action: () => unknown, text: string) {
  assert.throws(action, (error: FcdevError) => error.code === 'DISASM_LAYOUT_INVALID' && error.message.includes(text));
}

describe('创建布局', () => {
  it('省略 bank 与 writable 时补成 null 与 false，结果不可修改', () => {
    const layout = createLayout([{ id: 0, fileOffset: 0, length: 0x4000, address: 0xc000 }], { imageSize: 0x4000 });
    assert.deepEqual(layout.segments[0], { id: 0, fileOffset: 0, length: 0x4000, address: 0xc000, bank: null, writable: false });
    assert.equal(Object.isFrozen(layout), true);
    assert.equal(Object.isFrozen(layout.segments[0]), true);
  });

  it('换页窗口：地址相同、bank 不同的段可以并存', () => {
    const layout = createLayout([
      { id: 0, fileOffset: 0, length: 0x4000, address: 0x8000, bank: 0 },
      { id: 1, fileOffset: 0x4000, length: 0x4000, address: 0x8000, bank: 1 },
      { id: 2, fileOffset: 0x8000, length: 0x4000, address: 0xc000 },
    ], { imageSize: 0xc000 });
    assert.equal(layout.segments.length, 3);
  });

  it('拒绝没有段、段号重复、字段不合法', () => {
    assertInvalid(() => createLayout([], { imageSize: 16 }), '至少要有一个段');
    assertInvalid(() => createLayout([{ id: 0, fileOffset: 0, length: 4, address: 0 }, { id: 0, fileOffset: 4, length: 4, address: 4 }], { imageSize: 16 }), '段号 0 重复');
    assertInvalid(() => createLayout([{ id: 0, fileOffset: 0, length: 0, address: 0 }], { imageSize: 16 }), '长度不能是 0');
    assertInvalid(() => createLayout([{ id: 0, fileOffset: -1, length: 4, address: 0 }], { imageSize: 16 }), 'fileOffset');
    assertInvalid(() => createLayout([{ id: 0, fileOffset: 0, length: 4, address: 0, bank: -1 }], { imageSize: 16 }), 'bank');
    assertInvalid(() => createLayout([{ id: 0, fileOffset: 0, length: 4, address: 0, writable: 1 as unknown as boolean }], { imageSize: 16 }), 'writable');
    assertInvalid(() => createLayout([{ id: 0, fileOffset: 0, length: 4, address: 0 }], { imageSize: 0 }), '映像大小');
  });

  it('拒绝超出文件、超出 $FFFF', () => {
    assertInvalid(() => createLayout([{ id: 0, fileOffset: 8, length: 16, address: 0 }], { imageSize: 16 }), '超出映像文件');
    assertInvalid(() => createLayout([{ id: 0, fileOffset: 0, length: 16, address: 0xfff8 }], { imageSize: 16 }), '超出 $FFFF');
  });

  it('拒绝文件范围重叠', () => {
    assertInvalid(() => createLayout([
      { id: 0, fileOffset: 0, length: 8, address: 0x8000, bank: 0 },
      { id: 1, fileOffset: 4, length: 8, address: 0x8000, bank: 1 },
    ], { imageSize: 16 }), '在文件里重叠');
  });

  it('拒绝同一 bank 或没有 bank 的地址重叠', () => {
    assertInvalid(() => createLayout([
      { id: 0, fileOffset: 0, length: 8, address: 0x8000, bank: 0 },
      { id: 1, fileOffset: 8, length: 8, address: 0x8004, bank: 0 },
    ], { imageSize: 16 }), '不是不同的 bank');
    assertInvalid(() => createLayout([
      { id: 0, fileOffset: 0, length: 8, address: 0x8000 },
      { id: 1, fileOffset: 8, length: 8, address: 0x8000, bank: 1 },
    ], { imageSize: 16 }), '不是不同的 bank');
  });
});

describe('查询', () => {
  const layout = createLayout([
    { id: 0, fileOffset: 0x10, length: 0x4000, address: 0x8000, bank: 0 },
    { id: 1, fileOffset: 0x4010, length: 0x4000, address: 0x8000, bank: 1 },
    { id: 2, fileOffset: 0x8010, length: 0x4000, address: 0xc000 },
  ], { imageSize: 0xc010 + 0x10 });

  it('按段号取段', () => {
    assert.equal(getSegment(layout, 1)!.bank, 1);
    assert.equal(getSegment(layout, 9), undefined);
  });

  it('按 CPU 地址找段：换页窗口返回每个 bank 的段，固定区只有一段，没有映射的地址为空', () => {
    assert.deepEqual(findSegmentsAt(layout, 0x9000).map((segment) => segment.id), [0, 1]);
    assert.deepEqual(findSegmentsAt(layout, 0xffff).map((segment) => segment.id), [2]);
    assert.deepEqual(findSegmentsAt(layout, 0x7fff), []);
  });

  it('地址与文件偏移互相换算', () => {
    assert.equal(addressToOffset(getSegment(layout, 1)!, 0x8000), 0x4010);
    assert.equal(addressToOffset(getSegment(layout, 2)!, 0xfffa), 0x8010 + 0x3ffa);
    assert.throws(() => addressToOffset(getSegment(layout, 0)!, 0xc000), RangeError);
    const located = locateOffset(layout, 0x4011)!;
    assert.equal(located.segment.id, 1);
    assert.equal(located.address, 0x8001);
    assert.equal(locateOffset(layout, 0x0f), null);
  });

  it('列出不属于任何段的字节范围', () => {
    assert.deepEqual(findUncovered(layout), [{ offset: 0, length: 0x10 }, { offset: 0xc010, length: 0x10 }]);
    const full = createLayout([{ id: 0, fileOffset: 0, length: 8, address: 0 }], { imageSize: 8 });
    assert.deepEqual(findUncovered(full), []);
  });
});
