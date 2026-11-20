// ------------------------------------------------------------------------
// 名称：auto-symbols.mjs
// 说明：自动命名：给被调用、被跳转、被引用的地址生成符号，让生成的源码里没有裸地址；已有符号的地址不动，名字用 sub_、loc_、data_、var_ 加地址。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：规则：调用目标是 sub_（过程），分支与跳转目标是 loc_（标签），布局内被读写的数据地址是 data_，布局外（RAM、寄存器）被读写的地址是 var_；
//       目标落在指令中间时，符号放在那条指令的开头，源码里写成“符号加偏移”；入口、地址表的目标也命名；中断向量的入口叫 reset、nmi、irq。
//       布局外的地址如果符号集里有同值的常量就不再生成。自动符号标记 auto，重命名后标记清除；进度报告把还带标记的算作未完成。
// ------------------------------------------------------------------------
import { isAssemblable } from '../cpu/index.mjs';
import { cloneProject, getSegment } from '../model/index.mjs';
import { createAddressSpace } from './address-space.mjs';
import { scanCode } from './code-scan.mjs';

/** 向量入口的名字，取自 entriesFromVectors 写的说明 */
const VECTOR_NAMES = { 复位向量: 'reset', 'NMI 向量': 'nmi', 'IRQ/BRK 向量': 'irq' };

/** 同一个地址有多种用途时，名字取优先级最高的 */
const PRIORITY = { sub: 3, loc: 2, data: 1 };

/** 各用途对应的符号种类 */
const KINDS = { sub: 'procedure', loc: 'label', data: 'data' };

/**
 * 十六进制写出地址。
 * @param {number} address 地址
 * @returns {string} 四位大写十六进制
 */
function hex4(address) {
  return address.toString(16).toUpperCase().padStart(4, '0');
}

/**
 * 为工程补上自动符号。
 * @param {import('../model/project.mjs').Project} project 工程，不会被修改
 * @param {Uint8Array} bytes 映像的全部字节
 * @param {{externalNames?: ReadonlyMap<number, string>}} [options] 符号集里已有的名字（数值到名字），这些布局外的地址不再生成符号
 * @returns {{project: import('../model/project.mjs').Project, added: number}} 补上符号的新工程与新增的符号数
 */
export function ensureAutoSymbols(project, bytes, { externalNames = new Map() } = {}) {
  const next = cloneProject(project);
  const space = createAddressSpace(next.layout);
  const scan = scanCode(next, bytes);

  /** 布局内的位置（段号:地址）到用途与名字；布局外的地址到 var */
  const wanted = new Map();
  const externals = new Set();

  const want = (segmentId, address, use, forcedName) => {
    const key = `${segmentId}:${address}`;
    const current = wanted.get(key);
    if (current === undefined || PRIORITY[use] > PRIORITY[current.use]) wanted.set(key, { segmentId, address, use, name: forcedName ?? current?.name });
    else if (forcedName !== undefined && current.name === undefined) current.name = forcedName;
  };

  /** 把一个 CPU 地址落实到符号的位置：指令中间的地址落到那条指令的开头 */
  const place = (address, fromSegmentId, use) => {
    const resolved = space.resolve(address, fromSegmentId);
    if (resolved.status === 'ambiguous') return true;
    if (resolved.status !== 'ok') return false;
    const { segment } = resolved;
    const offset = segment.fileOffset + (address - segment.address);
    const inside = scan.containing(offset);
    if (inside !== null && inside.offset !== offset) want(segment.id, inside.address, use === 'data' ? 'loc' : use);
    else want(segment.id, address, scan.byOffset.has(offset) && use === 'data' ? 'loc' : use);
    return true;
  };

  for (const item of scan.list) {
    const { instruction } = item;
    if (!isAssemblable(instruction)) continue;
    if (instruction.flow === 'call') place(instruction.target, item.segmentId, 'sub');
    else if (instruction.flow === 'branch' || instruction.flow === 'jump') place(instruction.target, item.segmentId, 'loc');
    else if (instruction.reference !== null) {
      if (!place(instruction.reference, item.segmentId, 'data')) externals.add(instruction.reference);
    }
  }
  for (const region of next.regions.regions) {
    if (region.type !== 'words') continue;
    const segment = next.layout.segments.find((item) => region.offset >= item.fileOffset && region.offset < item.fileOffset + item.length);
    const adjust = region.params.adjust ?? 0;
    for (let offset = region.offset; offset + 1 < region.offset + region.length; offset += 2) {
      place(((bytes[offset] | (bytes[offset + 1] << 8)) + adjust) & 0xffff, segment.id, 'loc');
    }
  }
  for (const entry of next.entries) want(entry.segmentId, entry.address, 'sub', VECTOR_NAMES[entry.description]);

  const taken = new Set(next.symbols.map((symbol) => symbol.name));
  const located = new Set(next.symbols.map((symbol) => `${symbol.segmentId}:${symbol.address}`));
  const created = [];
  const add = (symbol) => {
    taken.add(symbol.name);
    located.add(`${symbol.segmentId}:${symbol.address}`);
    created.push(symbol);
  };
  const freeName = (base) => {
    let name = base;
    for (let counter = 2; taken.has(name); counter += 1) name = `${base}_${counter}`;
    return name;
  };

  for (const item of [...wanted.values()].sort((a, b) => a.segmentId - b.segmentId || a.address - b.address)) {
    if (located.has(`${item.segmentId}:${item.address}`)) continue;
    const segment = getSegment(next.layout, item.segmentId);
    const bank = segment.bank === null ? '' : `b${segment.bank}_`;
    add({ name: freeName(item.name ?? `${item.use}_${bank}${hex4(item.address)}`), address: item.address, segmentId: item.segmentId, kind: KINDS[item.use], description: '', module: null, auto: true });
  }
  for (const address of [...externals].sort((a, b) => a - b)) {
    if (externalNames.has(address) || located.has(`null:${address}`)) continue;
    add({ name: freeName(`var_${hex4(address)}`), address, segmentId: null, kind: 'variable', description: '', module: null, auto: true });
  }
  next.symbols.push(...created.map((symbol) => Object.freeze(symbol)));
  return { project: next, added: created.length };
}
