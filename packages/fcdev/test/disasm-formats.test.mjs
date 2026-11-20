// ------------------------------------------------------------------------
// 名称：disasm-formats.test.mjs
// 说明：测试映像格式解析器：平铺二进制的加载地址与自定义段，iNES 的文件头、NROM 布局与中断向量，以及统一入口。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { describe, it } from 'node:test';
import { FORMAT_NAMES, addressToOffset, parseFlat, parseImage, parseInes, parseInesHeader } from '../src/disasm/index.mjs';

/** 构造 iNES 文件：向量三个字放在 PRG 末尾，CHR 填 0xcc */
function makeInes({ prgUnits = 1, chrUnits = 1, mapper = 0, vectors = [0xc000, 0xc000, 0xc000], trainer = false, code = [], nes2 = false } = {}) {
  const prg = Buffer.alloc(prgUnits * 0x4000);
  Buffer.from(code).copy(prg, 0);
  vectors.forEach((value, index) => prg.writeUInt16LE(value, prg.length - 6 + index * 2));
  const header = Buffer.alloc(16);
  header.write('NES\x1a', 0, 'latin1');
  header[4] = prgUnits & 0xff;
  header[5] = chrUnits & 0xff;
  header[6] = ((mapper & 0x0f) << 4) | (trainer ? 0x04 : 0);
  header[7] = (mapper & 0xf0) | (nes2 ? 0x08 : 0);
  if (nes2) {
    header[8] = (mapper >> 8) & 0x0f;
    header[9] = ((chrUnits >> 8) << 4) | (prgUnits >> 8);
  }
  return Buffer.concat([header, trainer ? Buffer.alloc(512, 0xee) : Buffer.alloc(0), prg, Buffer.alloc(chrUnits * 0x2000, 0xcc)]);
}

describe('平铺二进制', () => {
  it('给出加载地址：整个文件一个段，没有向量，没有未划入的字节', () => {
    const parsed = parseFlat(Buffer.alloc(0x100), { address: 0x8000 });
    assert.equal(parsed.format, 'flat');
    assert.deepEqual(parsed.layout.segments, [{ id: 0, fileOffset: 0, length: 0x100, address: 0x8000, bank: null, writable: false }]);
    assert.equal(parsed.vectors, null);
    assert.deepEqual(parsed.unmapped, []);
    assert.match(parsed.description, /\$8000/);
  });

  it('给出段列表：用换页的布局，没有覆盖的字节记入 unmapped', () => {
    const parsed = parseFlat(Buffer.alloc(0x30), {
      segments: [
        { id: 0, fileOffset: 0, length: 0x10, address: 0x8000, bank: 0 },
        { id: 1, fileOffset: 0x20, length: 0x10, address: 0x8000, bank: 1 },
      ],
    });
    assert.equal(parsed.layout.segments.length, 2);
    assert.deepEqual(parsed.unmapped, [{ offset: 0x10, length: 0x10, description: '没有划入任何段' }]);
  });

  it('文件为空、没有给出地址与段、超出 $FFFF 时报错', () => {
    assert.throws(() => parseFlat(Buffer.alloc(0), { address: 0 }), (error) => error.code === 'DISASM_INPUT_INVALID');
    assert.throws(() => parseFlat(Buffer.alloc(4), {}), (error) => error.code === 'DISASM_INPUT_INVALID' && error.hint.includes('加载地址'));
    assert.throws(() => parseFlat(Buffer.alloc(0x20), { address: 0xfff0 }), (error) => error.code === 'DISASM_LAYOUT_INVALID');
  });
});

describe('iNES', () => {
  it('NROM-256 的 PRG 映在 $8000，NROM-128 映在 $C000，读出三个中断向量', () => {
    const big = parseInes(makeInes({ prgUnits: 2, vectors: [0x8100, 0x8000, 0x8200] }));
    assert.deepEqual(big.layout.segments, [{ id: 0, fileOffset: 16, length: 0x8000, address: 0x8000, bank: null, writable: false }]);
    assert.deepEqual(big.vectors, { nmi: 0x8100, reset: 0x8000, irq: 0x8200 });
    const small = parseInes(makeInes({ prgUnits: 1 }));
    assert.equal(small.layout.segments[0].address, 0xc000);
    assert.equal(small.format, 'ines');
  });

  it('向量在布局里的位置正是复位向量所在的字节', () => {
    const file = makeInes({ prgUnits: 1, vectors: [0x1111, 0xc123, 0x2222] });
    const { layout, vectors } = parseInes(file);
    assert.equal(file.readUInt16LE(addressToOffset(layout.segments[0], 0xfffc)), vectors.reset);
  });

  it('有 trainer 时 PRG 从 trainer 之后开始，文件头、trainer、CHR 记入 unmapped', () => {
    const file = makeInes({ trainer: true, code: [0xa9, 0x01] });
    const parsed = parseInes(file);
    assert.equal(parsed.layout.segments[0].fileOffset, 16 + 512);
    assert.deepEqual([...file.subarray(528, 530)], [0xa9, 0x01]);
    assert.deepEqual(parsed.unmapped.map((range) => range.description), ['iNES 文件头', 'trainer', 'CHR 及其后的数据']);
    assert.deepEqual(parsed.unmapped[2], { offset: 16 + 512 + 0x4000, length: 0x2000, description: 'CHR 及其后的数据' });
  });

  it('没有 CHR 时不产生 CHR 范围', () => {
    const parsed = parseInes(makeInes({ chrUnits: 0 }));
    assert.deepEqual(parsed.unmapped.map((range) => range.description), ['iNES 文件头']);
  });

  it('读文件头：映射器、大小、镜像、电池；NES 2.0 带出高位', () => {
    const header = parseInesHeader(makeInes({ prgUnits: 2, chrUnits: 4, mapper: 0x4b }));
    assert.deepEqual([header.nes2, header.mapper, header.prgSize, header.chrSize, header.trainer], [false, 0x4b, 0x8000, 0x8000, false]);
    const file = makeInes({ prgUnits: 1, chrUnits: 1, mapper: 0x123, nes2: true });
    file[6] |= 0x03;
    const modern = parseInesHeader(file);
    assert.deepEqual([modern.nes2, modern.mapper, modern.battery, modern.verticalMirroring], [true, 0x123, true, true]);
  });

  it('映射器不是 0、PRG 大小不是 16K 与 32K、文件头不对、文件被截断、指数写法都报错', () => {
    assert.throws(() => parseInes(makeInes({ mapper: 4 })), (error) => error.code === 'DISASM_MAPPER_UNSUPPORTED' && error.message.includes('4') && error.hint.includes('平铺'));
    assert.throws(() => parseInes(makeInes({ prgUnits: 4 })), (error) => error.code === 'DISASM_INPUT_INVALID' && error.message.includes('16K 或 32K'));
    assert.throws(() => parseInes(Buffer.from('MZ not nes file....')), (error) => error.code === 'DISASM_INPUT_INVALID');
    assert.throws(() => parseInes(makeInes({ chrUnits: 0 }).subarray(0, 8000)), (error) => error.code === 'DISASM_INPUT_INVALID' && error.message.includes('短'));
    const exponent = makeInes({ nes2: true });
    exponent[9] = 0x0f;
    assert.throws(() => parseInes(exponent), (error) => error.code === 'DISASM_INPUT_INVALID' && error.message.includes('指数'));
  });
});

describe('统一入口', () => {
  it('按格式名选择解析器', () => {
    assert.deepEqual(FORMAT_NAMES, ['flat', 'ines']);
    assert.equal(parseImage('flat', Buffer.alloc(4), { address: 0xf000 }).layout.segments[0].address, 0xf000);
    assert.equal(parseImage('ines', makeInes()).format, 'ines');
  });

  it('格式名未知时报错并列出可用的格式，原型上的名字也不算', () => {
    assert.throws(() => parseImage('elf', Buffer.alloc(4)), (error) => error.code === 'DISASM_FORMAT_UNKNOWN' && error.hint.includes('flat'));
    assert.throws(() => parseImage('toString', Buffer.alloc(4)), (error) => error.code === 'DISASM_FORMAT_UNKNOWN');
  });
});
