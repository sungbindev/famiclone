/*
 * ------------------------------------------------------------------------
 * 名称：woff2.mjs
 * 说明：WOFF2 字体解码：把 WOFF2 文件还原成 TrueType（sfnt）字体字节，供 DirectWrite 从内存加载。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：按 W3C WOFF2 规范实现：表目录、Brotli 解压（Node 自带 zlib）、glyf/loca 变换、hmtx 变换；
 *       输出的 glyf 用最朴素的编码（每点一个标志、坐标都用 16 位），loca 一律用长格式，字体内容与原字体等价；
 *       不支持字体集合（TTC）与 CFF 轮廓之外的特殊情况。
 * ------------------------------------------------------------------------
 */

import { brotliDecompressSync } from 'node:zlib';

/** WOFF2 规范里的已知表标签，下标就是目录项标志里的编号。 */
const KNOWN_TAGS = [
  'cmap', 'head', 'hhea', 'hmtx', 'maxp', 'name', 'OS/2', 'post', 'cvt ', 'fpgm', 'glyf', 'loca', 'prep', 'CFF ', 'VORG', 'EBDT', 'EBLC', 'gasp', 'hdmx', 'kern',
  'LTSH', 'PCLT', 'VDMX', 'vhea', 'vmtx', 'BASE', 'GDEF', 'GPOS', 'GSUB', 'EBSC', 'JSTF', 'MATH', 'CBDT', 'CBLC', 'COLR', 'CPAL', 'SVG ', 'sbix', 'acnt', 'avar',
  'bdat', 'bloc', 'bsln', 'cvar', 'fdsc', 'feat', 'fmtx', 'fvar', 'gvar', 'hsty', 'just', 'lcar', 'mort', 'morx', 'opbd', 'prop', 'trak', 'Zapf', 'Silf', 'Glat',
  'Gloc', 'Feat', 'Sill'
];

const WOFF2_SIGNATURE = 0x774f4632;
const TTC_FLAVOR = 0x74746366;
const HEADER_SIZE = 48;
const GLYF_HEADER_SIZE = 36;
const CHECKSUM_MAGIC = 0xb1b0afba;

/** 顺序读取的游标。 */
class Cursor {
  /**
   * @param {Uint8Array} bytes 数据。
   * @param {number} [position] 起始位置。
   */
  constructor(bytes, position = 0) {
    this.bytes = bytes;
    this.position = position;
  }

  u8() {
    if (this.position >= this.bytes.length) throw new Error('WOFF2 数据不完整');
    return this.bytes[this.position++];
  }

  u16() {
    return (this.u8() << 8) | this.u8();
  }

  i16() {
    const value = this.u16();
    return value >= 0x8000 ? value - 0x10000 : value;
  }

  /** UIntBase128：最多 5 字节，高位为 1 表示后面还有。 */
  base128() {
    let value = 0;
    for (let i = 0; i < 5; i += 1) {
      const byte = this.u8();
      if (i === 0 && byte === 0x80) throw new Error('WOFF2 的 UIntBase128 不能有前导零');
      value = value * 128 + (byte & 0x7f);
      if ((byte & 0x80) === 0) return value;
    }
    throw new Error('WOFF2 的 UIntBase128 超过 5 字节');
  }

  /** 255UInt16。 */
  u255() {
    const code = this.u8();
    if (code === 253) return this.u16();
    if (code === 255) return 253 + this.u8();
    if (code === 254) return 506 + this.u8();
    return code;
  }

  /**
   * 取出接下来 length 个字节（不复制）。
   * @param {number} length 字节数。
   */
  take(length) {
    if (this.position + length > this.bytes.length) throw new Error('WOFF2 数据不完整');
    const slice = this.bytes.subarray(this.position, this.position + length);
    this.position += length;
    return slice;
  }
}

/**
 * 按 WOFF2 的三元组规则给值加符号：标志的最低位为 1 是正数，否则是负数。
 * @param {number} bit 标志。
 * @param {number} value 绝对值。
 * @returns {number} 带符号的值。
 */
function withSign(bit, value) {
  return (bit & 1) !== 0 ? value : -value;
}

/**
 * 按 WOFF2 的三元组规则还原一个点相对上一点的位移。
 * @param {number} flag 标志的低 7 位。
 * @param {Cursor} stream 坐标流。
 * @returns {{dx: number, dy: number}} 位移。
 */
function readTriplet(flag, stream) {
  const sign = withSign;
  if (flag < 10) {
    return { dx: 0, dy: sign(flag, ((flag & 14) << 7) + stream.u8()) };
  }
  if (flag < 20) {
    return { dx: sign(flag, (((flag - 10) & 14) << 7) + stream.u8()), dy: 0 };
  }
  if (flag < 84) {
    const b0 = stream.u8();
    const index = flag - 20;
    return { dx: sign(flag, 1 + (index & 0x30) + (b0 >> 4)), dy: sign(flag >> 1, 1 + ((index & 0x0c) << 2) + (b0 & 0x0f)) };
  }
  if (flag < 120) {
    const index = flag - 84;
    const b0 = stream.u8();
    const b1 = stream.u8();
    return { dx: sign(flag, 1 + (Math.floor(index / 12) << 8) + b0), dy: sign(flag >> 1, 1 + ((Math.floor((index % 12) / 4)) << 8) + b1) };
  }
  if (flag < 124) {
    const b0 = stream.u8();
    const b1 = stream.u8();
    const b2 = stream.u8();
    return { dx: sign(flag, (b0 << 4) + (b1 >> 4)), dy: sign(flag >> 1, ((b1 & 0x0f) << 8) + b2) };
  }
  const b0 = stream.u8();
  const b1 = stream.u8();
  const b2 = stream.u8();
  const b3 = stream.u8();
  return { dx: sign(flag, (b0 << 8) + b1), dy: sign(flag >> 1, (b2 << 8) + b3) };
}

/**
 * 还原 glyf 与 loca 表。
 * @param {Uint8Array} data 变换后的 glyf 表数据。
 * @returns {{glyf: Buffer, loca: Buffer, numGlyphs: number, xMins: number[]}} 还原后的两张表、字形数与每个字形的 xMin（空字形为 0）。
 */
function reconstructGlyf(data) {
  const header = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const numGlyphs = header.getUint16(4);
  const sizes = [];
  for (let i = 0; i < 7; i += 1) sizes.push(header.getUint32(8 + i * 4));
  const [contourSize, pointsSize, flagSize, glyphSize, compositeSize, bboxSize, instructionSize] = sizes;
  let offset = GLYF_HEADER_SIZE;
  const next = (size) => {
    const stream = new Cursor(data.subarray(offset, offset + size));
    offset += size;
    return stream;
  };
  const contourStream = next(contourSize);
  const pointsStream = next(pointsSize);
  const flagStream = next(flagSize);
  const glyphStream = next(glyphSize);
  const compositeStream = next(compositeSize);
  const bboxAll = next(bboxSize);
  const instructionStream = next(instructionSize);
  const bitmapLength = ((numGlyphs + 31) >> 5) << 2;
  const bitmap = bboxAll.take(bitmapLength);
  const bboxStream = bboxAll;

  const glyphs = [];
  const xMins = [];
  const offsets = [0];
  let total = 0;
  for (let glyph = 0; glyph < numGlyphs; glyph += 1) {
    const contours = contourStream.i16();
    const hasBbox = (bitmap[glyph >> 3] & (0x80 >> (glyph & 7))) !== 0;
    let body = Buffer.alloc(0);
    let xMin = 0;
    if (contours > 0) {
      const endPoints = [];
      let pointCount = 0;
      for (let c = 0; c < contours; c += 1) {
        pointCount += pointsStream.u255();
        endPoints.push(pointCount - 1);
      }
      const onCurve = [];
      const xs = [];
      const ys = [];
      const flags = flagStream.take(pointCount);
      let x = 0;
      let y = 0;
      for (let p = 0; p < pointCount; p += 1) {
        const { dx, dy } = readTriplet(flags[p] & 0x7f, glyphStream);
        x += dx;
        y += dy;
        xs.push(x);
        ys.push(y);
        onCurve.push((flags[p] & 0x80) === 0);
      }
      const instructionLength = glyphStream.u255();
      const instructions = instructionStream.take(instructionLength);
      let box;
      if (hasBbox) box = [bboxStream.i16(), bboxStream.i16(), bboxStream.i16(), bboxStream.i16()];
      else box = [Math.min(...xs), Math.min(...ys), Math.max(...xs), Math.max(...ys)];
      xMin = box[0];
      body = Buffer.alloc(10 + 2 * contours + 2 + instructionLength + pointCount * 5);
      let at = 0;
      body.writeInt16BE(contours, at);
      at += 2;
      for (const value of box) {
        body.writeInt16BE(value, at);
        at += 2;
      }
      for (const end of endPoints) {
        body.writeUInt16BE(end, at);
        at += 2;
      }
      body.writeUInt16BE(instructionLength, at);
      at += 2;
      Buffer.from(instructions).copy(body, at);
      at += instructionLength;
      for (let p = 0; p < pointCount; p += 1) body[at++] = onCurve[p] ? 1 : 0;
      let previous = 0;
      for (let p = 0; p < pointCount; p += 1) {
        body.writeInt16BE(xs[p] - previous, at);
        previous = xs[p];
        at += 2;
      }
      previous = 0;
      for (let p = 0; p < pointCount; p += 1) {
        body.writeInt16BE(ys[p] - previous, at);
        previous = ys[p];
        at += 2;
      }
    } else if (contours < 0) {
      const start = compositeStream.position;
      let more = true;
      let hasInstructions = false;
      while (more) {
        const flags = compositeStream.u16();
        compositeStream.u16();
        compositeStream.take((flags & 0x0001) !== 0 ? 4 : 2);
        if ((flags & 0x0008) !== 0) compositeStream.take(2);
        else if ((flags & 0x0040) !== 0) compositeStream.take(4);
        else if ((flags & 0x0080) !== 0) compositeStream.take(8);
        more = (flags & 0x0020) !== 0;
        if ((flags & 0x0100) !== 0) hasInstructions = true;
      }
      const components = compositeStream.bytes.subarray(start, compositeStream.position);
      let instructionLength = 0;
      let instructions = new Uint8Array(0);
      if (hasInstructions) {
        instructionLength = glyphStream.u255();
        instructions = instructionStream.take(instructionLength);
      }
      if (!hasBbox) throw new Error('WOFF2 复合字形缺少外框');
      const box = [bboxStream.i16(), bboxStream.i16(), bboxStream.i16(), bboxStream.i16()];
      xMin = box[0];
      body = Buffer.alloc(10 + components.length + (hasInstructions ? 2 + instructionLength : 0));
      body.writeInt16BE(-1, 0);
      box.forEach((value, index) => body.writeInt16BE(value, 2 + index * 2));
      Buffer.from(components).copy(body, 10);
      if (hasInstructions) {
        body.writeUInt16BE(instructionLength, 10 + components.length);
        Buffer.from(instructions).copy(body, 12 + components.length);
      }
    }
    const padded = Buffer.alloc((body.length + 3) & ~3);
    body.copy(padded);
    glyphs.push(padded);
    xMins.push(xMin);
    total += padded.length;
    offsets.push(total);
  }
  const loca = Buffer.alloc(offsets.length * 4);
  offsets.forEach((value, index) => loca.writeUInt32BE(value, index * 4));
  return { glyf: Buffer.concat(glyphs), loca, numGlyphs, xMins };
}

/**
 * 还原 hmtx 表。
 * @param {Uint8Array} data 变换后的 hmtx 表数据。
 * @param {number} numHMetrics hhea 里的度量数。
 * @param {number} numGlyphs 字形数。
 * @param {number[]} xMins 每个字形的 xMin。
 * @returns {Buffer} 还原后的 hmtx 表。
 */
function reconstructHmtx(data, numHMetrics, numGlyphs, xMins) {
  const stream = new Cursor(data);
  const flags = stream.u8();
  const advances = [];
  for (let i = 0; i < numHMetrics; i += 1) advances.push(stream.u16());
  const lsb = [];
  for (let i = 0; i < numHMetrics; i += 1) lsb.push((flags & 1) === 0 ? stream.i16() : xMins[i]);
  for (let i = numHMetrics; i < numGlyphs; i += 1) lsb.push((flags & 2) === 0 ? stream.i16() : xMins[i]);
  const result = Buffer.alloc(numHMetrics * 4 + (numGlyphs - numHMetrics) * 2);
  let at = 0;
  for (let i = 0; i < numGlyphs; i += 1) {
    if (i < numHMetrics) {
      result.writeUInt16BE(advances[i], at);
      at += 2;
    }
    result.writeInt16BE(lsb[i], at);
    at += 2;
  }
  return result;
}

/**
 * 表数据的校验和。
 * @param {Buffer} table 表数据。
 * @returns {number} 校验和（无符号 32 位）。
 */
function checksum(table) {
  let sum = 0;
  for (let i = 0; i < table.length; i += 4) {
    const word = (table[i] << 24) | ((table[i + 1] ?? 0) << 16) | ((table[i + 2] ?? 0) << 8) | (table[i + 3] ?? 0);
    sum = (sum + (word >>> 0)) >>> 0;
  }
  return sum;
}

/**
 * 把各张表拼成 sfnt 字体。
 * @param {number} flavor 字体类型标记。
 * @param {Map<string, Buffer>} tables 各表。
 * @returns {Buffer} 字体字节。
 */
function assembleSfnt(flavor, tables) {
  const tags = [...tables.keys()].toSorted();
  const count = tags.length;
  let entrySelector = 0;
  while (1 << (entrySelector + 1) <= count) entrySelector += 1;
  const searchRange = (1 << entrySelector) * 16;
  const directorySize = 12 + count * 16;
  let offset = directorySize;
  const placed = tags.map((tag) => {
    const table = tables.get(tag);
    const entry = { tag, table, offset };
    offset += (table.length + 3) & ~3;
    return entry;
  });
  const font = Buffer.alloc(offset);
  font.writeUInt32BE(flavor, 0);
  font.writeUInt16BE(count, 4);
  font.writeUInt16BE(searchRange, 6);
  font.writeUInt16BE(entrySelector, 8);
  font.writeUInt16BE(count * 16 - searchRange, 10);
  placed.forEach(({ tag, table, offset: at }, index) => {
    const entryAt = 12 + index * 16;
    font.write(tag, entryAt, 4, 'latin1');
    font.writeUInt32BE(checksum(table), entryAt + 4);
    font.writeUInt32BE(at, entryAt + 8);
    font.writeUInt32BE(table.length, entryAt + 12);
    table.copy(font, at);
  });
  // head 表的整体校验和调整值
  const head = placed.find((entry) => entry.tag === 'head');
  if (head) {
    font.writeUInt32BE(0, head.offset + 8);
    font.writeUInt32BE((CHECKSUM_MAGIC - checksum(font)) >>> 0, head.offset + 8);
    const index = placed.indexOf(head);
    font.writeUInt32BE(checksum(font.subarray(head.offset, head.offset + head.table.length)), 12 + index * 16 + 4);
  }
  return font;
}

/**
 * 把 WOFF2 文件解码成 TrueType 字体。
 * @param {Uint8Array} input WOFF2 文件内容。
 * @returns {Buffer} TrueType 字体字节。
 */
export function decodeWoff2(input) {
  const view = new DataView(input.buffer, input.byteOffset, input.byteLength);
  if (input.length < HEADER_SIZE || view.getUint32(0) !== WOFF2_SIGNATURE) throw new Error('不是 WOFF2 文件');
  const flavor = view.getUint32(4);
  if (flavor === TTC_FLAVOR) throw new Error('不支持字体集合（TTC）');
  const numTables = view.getUint16(12);
  const compressedSize = view.getUint32(20);
  const directory = new Cursor(input, HEADER_SIZE);
  const entries = [];
  for (let i = 0; i < numTables; i += 1) {
    const flags = directory.u8();
    const tag = (flags & 0x3f) === 0x3f ? Buffer.from(directory.take(4)).toString('latin1') : KNOWN_TAGS[flags & 0x3f];
    const version = flags >> 6;
    const originalLength = directory.base128();
    const isGlyphTable = tag === 'glyf' || tag === 'loca';
    const transformed = isGlyphTable ? version === 0 : version !== 0;
    const storedLength = transformed ? directory.base128() : originalLength;
    entries.push({ tag, transformed, originalLength, storedLength });
  }
  const stream = brotliDecompressSync(Buffer.from(input.subarray(directory.position, directory.position + compressedSize)));
  const stored = new Map();
  let at = 0;
  for (const entry of entries) {
    stored.set(entry.tag, stream.subarray(at, at + entry.storedLength));
    at += entry.storedLength;
  }

  const tables = new Map();
  let glyphs = null;
  if (stored.has('glyf')) {
    const glyfEntry = entries.find((entry) => entry.tag === 'glyf');
    if (glyfEntry.transformed) {
      glyphs = reconstructGlyf(stored.get('glyf'));
      tables.set('glyf', glyphs.glyf);
      tables.set('loca', glyphs.loca);
    }
  }
  for (const entry of entries) {
    if (tables.has(entry.tag)) continue;
    const data = Buffer.from(stored.get(entry.tag));
    if (entry.tag === 'hmtx' && entry.transformed) {
      if (!glyphs) throw new Error('hmtx 变换需要先还原 glyf');
      const hhea = stored.get('hhea');
      const numHMetrics = (hhea[34] << 8) | hhea[35];
      tables.set('hmtx', reconstructHmtx(data, numHMetrics, glyphs.numGlyphs, glyphs.xMins));
    } else {
      tables.set(entry.tag, data);
    }
  }
  // loca 一律用长格式
  if (glyphs && tables.has('head')) {
    const head = Buffer.from(tables.get('head'));
    head.writeInt16BE(1, 50);
    tables.set('head', head);
  }
  return assembleSfnt(flavor, tables);
}
