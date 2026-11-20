// ------------------------------------------------------------------------
// 名称：ines.mjs
// 说明：iNES 文件（NES 卡带）的解析器：读文件头，NROM 的 PRG 生成布局并读出三个中断向量。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：支持 iNES 与 NES 2.0 文件头（NES 2.0 的 PRG 大小只支持 16K 为单位的写法）；映射器不是 0 时地址映射不能只看文件头，
//       报 DISASM_MAPPER_UNSUPPORTED，由调用者改用平铺解析器并给出布局。
//       文件头、trainer、CHR 不在 CPU 地址空间里，不进布局，记入 unmapped。
// ------------------------------------------------------------------------
import { FcdevError } from '../../shared/index.mjs';
import { createLayout } from '../model/index.mjs';

/** 文件头长度 */
const HEADER_SIZE = 16;

/** trainer 块长度 */
const TRAINER_SIZE = 512;

/** PRG 的单位：16K */
const PRG_UNIT = 0x4000;

/** CHR 的单位：8K */
const CHR_UNIT = 0x2000;

/**
 * @typedef {object} InesHeader
 * @property {boolean} nes2 是否 NES 2.0 文件头
 * @property {number} mapper 映射器号
 * @property {number} prgSize PRG 的字节数
 * @property {number} chrSize CHR 的字节数
 * @property {boolean} trainer 是否有 512 字节的 trainer
 * @property {boolean} battery 是否有电池供电的存储
 * @property {boolean} verticalMirroring 名称表是否垂直镜像
 * @property {boolean} fourScreen 是否四屏
 */

/**
 * 解析文件头。
 * @param {Buffer} buffer 文件内容
 * @returns {InesHeader} 文件头
 * @throws {FcdevError} 不是 iNES 文件，或使用了不支持的 NES 2.0 大小写法
 */
export function parseInesHeader(buffer) {
  if (buffer.length < HEADER_SIZE || buffer.subarray(0, 4).toString('latin1') !== 'NES\x1a') {
    throw new FcdevError('不是 iNES 文件（缺少 NES 文件头）', { code: 'DISASM_INPUT_INVALID' });
  }
  const nes2 = (buffer[7] & 0x0c) === 0x08;
  let prgUnits = buffer[4];
  let chrUnits = buffer[5];
  if (nes2) {
    if ((buffer[9] & 0x0f) === 0x0f || (buffer[9] & 0xf0) === 0xf0) {
      throw new FcdevError('不支持 NES 2.0 的指数写法的 ROM 大小', { code: 'DISASM_INPUT_INVALID' });
    }
    prgUnits |= (buffer[9] & 0x0f) << 8;
    chrUnits |= (buffer[9] >> 4) << 8;
  }
  return {
    nes2,
    mapper: (buffer[6] >> 4) | (buffer[7] & 0xf0) | (nes2 ? (buffer[8] & 0x0f) << 8 : 0),
    prgSize: prgUnits * PRG_UNIT,
    chrSize: chrUnits * CHR_UNIT,
    trainer: (buffer[6] & 0x04) !== 0,
    battery: (buffer[6] & 0x02) !== 0,
    verticalMirroring: (buffer[6] & 0x01) !== 0,
    fourScreen: (buffer[6] & 0x08) !== 0,
  };
}

/**
 * 解析 iNES 文件。
 * @param {Buffer} buffer 文件内容
 * @returns {import('./index.mjs').ParsedImage & {header: InesHeader}} 解析结果：PRG 一个段，16K 映在 $C000，32K 映在 $8000
 * @throws {FcdevError} 不是 iNES 文件、映射器不是 0、PRG 不是 16K 或 32K、文件比文件头声明的短
 */
export function parseInes(buffer) {
  const header = parseInesHeader(buffer);
  if (header.mapper !== 0) {
    throw new FcdevError(`暂不支持映射器 ${header.mapper}`, {
      code: 'DISASM_MAPPER_UNSUPPORTED',
      hint: '目前只支持 NROM（映射器 0）；其他映射器请改用平铺格式，并给出每个 bank 的段（文件偏移、长度、地址、bank）',
    });
  }
  if (header.prgSize !== PRG_UNIT && header.prgSize !== 2 * PRG_UNIT) {
    throw new FcdevError(`NROM 的 PRG 应为 16K 或 32K，实际 ${header.prgSize} 字节`, { code: 'DISASM_INPUT_INVALID' });
  }
  const prgStart = HEADER_SIZE + (header.trainer ? TRAINER_SIZE : 0);
  const prgEnd = prgStart + header.prgSize;
  if (buffer.length < prgEnd) throw new FcdevError('文件比 iNES 文件头声明的短', { code: 'DISASM_INPUT_INVALID' });

  const layout = createLayout([{ id: 0, fileOffset: prgStart, length: header.prgSize, address: 0x10000 - header.prgSize }], { imageSize: buffer.length });
  const unmapped = [{ offset: 0, length: HEADER_SIZE, description: 'iNES 文件头' }];
  if (header.trainer) unmapped.push({ offset: HEADER_SIZE, length: TRAINER_SIZE, description: 'trainer' });
  if (buffer.length > prgEnd) unmapped.push({ offset: prgEnd, length: buffer.length - prgEnd, description: header.chrSize > 0 ? 'CHR 及其后的数据' : 'PRG 之后的数据' });

  return {
    format: 'ines',
    header,
    layout,
    vectors: { nmi: buffer.readUInt16LE(prgEnd - 6), reset: buffer.readUInt16LE(prgEnd - 4), irq: buffer.readUInt16LE(prgEnd - 2) },
    unmapped,
    description: `iNES NROM，PRG ${header.prgSize / 1024}K`,
  };
}
