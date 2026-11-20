// ------------------------------------------------------------------------
// 名称：ines.mjs
// 说明：iNES 文件（NES 卡带）的解析器：读文件头，按映射器把 PRG 划成段（固定的和可换页的 bank）并读出三个中断向量。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：支持 iNES 与 NES 2.0 文件头（NES 2.0 的 PRG 大小只支持 16K 为单位的写法）；
//       内置映射器 0、3（同 NROM）、2、1（按“最后一个 16K bank 固定在 $C000”的默认模式）、7（32K 一个 bank）、4（MMC3，按默认模式），
//       其他映射器地址映射不能只看文件头，报 DISASM_MAPPER_UNSUPPORTED，由调用者用模拟器的地址解析查出布局后改用平铺解析器。
//       可换页的 bank 按初始得到的窗口放置（MMC3 的可换 bank 只放在 $8000，$A000 窗口里的代码要再调整布局）。
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
 * 可以直接按文件头划分 PRG 的映射器：名字与划分方式。
 * fixed：整块放在 CPU 地址空间末尾；last-fixed-16k：最后 16K 固定在 $C000，其余 16K 一个 bank 轮流占 $8000；
 * switch-32k：32K 一个 bank，都在 $8000；mmc3：8K 一个 bank，最后两个固定在 $C000、$E000，其余占 $8000。
 */
const MAPPERS = Object.freeze({
  0: { name: 'NROM', split: 'fixed' },
  1: { name: 'MMC1', split: 'last-fixed-16k' },
  2: { name: 'UxROM', split: 'last-fixed-16k' },
  3: { name: 'CNROM', split: 'fixed' },
  4: { name: 'MMC3', split: 'mmc3' },
  7: { name: 'AxROM', split: 'switch-32k' },
});

/** 内置支持的映射器号 */
export const SUPPORTED_MAPPERS = Object.freeze(Object.keys(MAPPERS).map(Number));

/** 8K：MMC3 的 bank 大小 */
const PRG_UNIT_8K = 0x2000;

/** 32K：AxROM 的 bank 大小 */
const PRG_UNIT_32K = 0x8000;

/**
 * 把 PRG 按映射器划成段。
 * @param {{name: string, split: string}} mapper 映射器
 * @param {number} prgSize PRG 的字节数
 * @param {number} prgStart PRG 在文件里的起始偏移
 * @returns {Array<{id: number, fileOffset: number, length: number, address: number, bank: number|null}>} 段
 * @throws {FcdevError} PRG 的大小不符合这种映射器的划分
 */
function splitPrg(mapper, prgSize, prgStart) {
  const segment = (index, offset, length, address, bank) => ({ id: index, fileOffset: prgStart + offset, length, address, bank });
  const wrongSize = (expected) => new FcdevError(`${mapper.name} 的 PRG 应为 ${expected}，实际 ${prgSize} 字节`, { code: 'DISASM_INPUT_INVALID' });
  if (mapper.split === 'fixed') {
    if (prgSize !== PRG_UNIT && prgSize !== 2 * PRG_UNIT) throw wrongSize('16K 或 32K');
    return [segment(0, 0, prgSize, 0x10000 - prgSize, null)];
  }
  if (mapper.split === 'last-fixed-16k') {
    if (prgSize === 0 || prgSize % PRG_UNIT !== 0) throw wrongSize('16K 的整数倍');
    if (prgSize === PRG_UNIT) return [segment(0, 0, PRG_UNIT, 0xc000, null)];
    const banks = prgSize / PRG_UNIT - 1;
    return [...Array.from({ length: banks }, (_, index) => segment(index, index * PRG_UNIT, PRG_UNIT, 0x8000, index)), segment(banks, banks * PRG_UNIT, PRG_UNIT, 0xc000, null)];
  }
  if (mapper.split === 'switch-32k') {
    if (prgSize === 0 || prgSize % PRG_UNIT_32K !== 0) throw wrongSize('32K 的整数倍');
    return Array.from({ length: prgSize / PRG_UNIT_32K }, (_, index) => segment(index, index * PRG_UNIT_32K, PRG_UNIT_32K, 0x8000, index));
  }
  if (prgSize < 4 * PRG_UNIT_8K || prgSize % PRG_UNIT_8K !== 0) throw wrongSize('8K 的整数倍，不少于 32K');
  const units = prgSize / PRG_UNIT_8K;
  return [
    ...Array.from({ length: units - 2 }, (_, index) => segment(index, index * PRG_UNIT_8K, PRG_UNIT_8K, 0x8000, index)),
    segment(units - 2, (units - 2) * PRG_UNIT_8K, PRG_UNIT_8K, 0xc000, null),
    segment(units - 1, (units - 1) * PRG_UNIT_8K, PRG_UNIT_8K, 0xe000, null),
  ];
}

/**
 * 解析 iNES 文件。
 * @param {Buffer} buffer 文件内容
 * @returns {import('./index.mjs').ParsedImage & {header: InesHeader}} 解析结果：按映射器划分的段；向量取 PRG 最后一个 bank 的末尾
 * @throws {FcdevError} 不是 iNES 文件、映射器不支持、PRG 大小不符合映射器、文件比文件头声明的短
 */
export function parseInes(buffer) {
  const header = parseInesHeader(buffer);
  const mapper = Object.hasOwn(MAPPERS, header.mapper) ? MAPPERS[header.mapper] : undefined;
  if (mapper === undefined) {
    throw new FcdevError(`暂不支持映射器 ${header.mapper}`, {
      code: 'DISASM_MAPPER_UNSUPPORTED',
      hint: `内置支持的映射器：${SUPPORTED_MAPPERS.join('、')}；其他映射器在模拟器里用 emu_inspect 的 resolve 查出各窗口对应的 PRG 偏移，再用平铺格式给出每个 bank 的段（文件偏移要加上 16 字节文件头）`,
    });
  }
  const prgStart = HEADER_SIZE + (header.trainer ? TRAINER_SIZE : 0);
  const prgEnd = prgStart + header.prgSize;
  if (buffer.length < prgEnd) throw new FcdevError('文件比 iNES 文件头声明的短', { code: 'DISASM_INPUT_INVALID' });
  const segments = splitPrg(mapper, header.prgSize, prgStart);

  const layout = createLayout(segments, { imageSize: buffer.length });
  const unmapped = [{ offset: 0, length: HEADER_SIZE, description: 'iNES 文件头' }];
  if (header.trainer) unmapped.push({ offset: HEADER_SIZE, length: TRAINER_SIZE, description: 'trainer' });
  if (buffer.length > prgEnd) unmapped.push({ offset: prgEnd, length: buffer.length - prgEnd, description: header.chrSize > 0 ? 'CHR 及其后的数据' : 'PRG 之后的数据' });

  return {
    format: 'ines',
    header,
    layout,
    vectors: { nmi: buffer.readUInt16LE(prgEnd - 6), reset: buffer.readUInt16LE(prgEnd - 4), irq: buffer.readUInt16LE(prgEnd - 2) },
    vectorSegmentId: segments.at(-1).id,
    unmapped,
    description: `iNES ${mapper.name}（映射器 ${header.mapper}），PRG ${header.prgSize / 1024}K`,
  };
}
