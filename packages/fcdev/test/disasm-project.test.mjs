// ------------------------------------------------------------------------
// 名称：disasm-project.test.mjs
// 说明：测试反汇编工程：符号、注释、操作数提示、模块的检查，工程文件的存取与过期判断，批量编辑的整批回滚，符号集解析，自动命名，对工程的追踪。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { describe, it } from 'node:test';
import {
  EDIT_OPERATIONS, applyEdits, cloneProject, createAddressSpace, createLayout, createProject, ensureAutoSymbols, entryAtAddress, indexByValue, isImageStale, isVerificationStale,
  parseFlat, parseProject, parseSymbolSet, projectDigest, serializeProject, traceProject,
} from '../src/disasm/index.mjs';

/** 平铺映像的工程 */
function flatProject(bytes, { address = 0xc000, entries = [] } = {}) {
  const buffer = Buffer.from(bytes);
  const parsed = parseFlat(buffer, { address });
  const space = createAddressSpace(parsed.layout);
  const project = createProject({
    imagePath: 'a.bin', bytes: buffer, format: 'flat', formatOptions: { address }, layout: parsed.layout,
    entries: entries.map((entry) => entryAtAddress(space, entry, { source: 'manual' })),
  });
  return { project, bytes: buffer };
}

const PROGRAM = [0xa9, 0x01, 0x20, 0x08, 0xc0, 0xd0, 0xfb, 0x60, 0x60];

describe('符号集', () => {
  it('取出“名字 = 数值”，数值支持 $、0x、十进制，其他行忽略', () => {
    const text = '; 注释\nPPUCTRL = $2000\nPPUMASK=0x2001 ; 说明\nCOUNT = 10\n.import foo\nBIG = $12345\nname = other + 1\n  INDENT = $10\n.define X 5\n';
    assert.deepEqual(parseSymbolSet(text), [{ name: 'PPUCTRL', value: 0x2000 }, { name: 'PPUMASK', value: 0x2001 }, { name: 'COUNT', value: 10 }, { name: 'INDENT', value: 0x10 }]);
  });

  it('同一个数值有多个名字时用第一个', () => {
    assert.deepEqual([...indexByValue([{ name: 'A', value: 1 }, { name: 'B', value: 1 }, { name: 'C', value: 2 }])], [[1, 'A'], [2, 'C']]);
  });
});

describe('工程文件', () => {
  it('存成 JSON 再读回，内容完全一致；字段顺序固定', () => {
    const { project } = flatProject(PROGRAM, { entries: [0xc000] });
    const edited = applyEdits(project, [
      { op: 'defineSymbol', name: 'start', address: 0xc000, kind: 'procedure', description: '入口' },
      { op: 'setRegion', address: 0xc000, length: 8, type: 'code' },
      { op: 'setComment', address: 0xc000, kind: 'block', text: '说明' },
      { op: 'setHint', address: 0xc000, form: 'decimal' },
      { op: 'assignModule', name: 'core', address: 0xc000, length: 4 },
    ]).project;
    const text = serializeProject(edited);
    assert.deepEqual(Object.keys(JSON.parse(text)), ['version', 'image', 'format', 'profile', 'layout', 'entries', 'regions', 'symbols', 'comments', 'hints', 'modules', 'symbolSets', 'unresolved', 'verification']);
    assert.equal(serializeProject(parseProject(text)), text);
    assert.ok(text.endsWith('\n'));
  });

  it('读回时逐项检查：版本、缺字段、符号重名、模块重叠、坏 JSON', () => {
    const { project } = flatProject(PROGRAM);
    const data = JSON.parse(serializeProject(project));
    assert.throws(() => parseProject('{'), (error) => error.code === 'DISASM_PROJECT_INVALID');
    assert.throws(() => parseProject(JSON.stringify({ ...data, version: 9 })), /不支持的工程文件版本/);
    assert.throws(() => parseProject(JSON.stringify({ ...data, symbols: undefined })), /缺少 symbols/);
    const twice = { ...data, symbols: [{ name: 'a', address: 0xc000 }, { name: 'a', address: 0xc001 }] };
    assert.throws(() => parseProject(JSON.stringify(twice)), (error) => error.code === 'DISASM_SYMBOL_INVALID');
    const overlap = { ...data, modules: [{ name: 'x', ranges: [{ address: 0xc000, length: 4 }] }, { name: 'y', ranges: [{ address: 0xc003, length: 2 }] }] };
    assert.throws(() => parseProject(JSON.stringify(overlap)), (error) => error.code === 'DISASM_MODULE_INVALID' && error.message.includes('重叠'));
  });

  it('映像变了工程就过期；回读结果在工程改动后过期', () => {
    const { project, bytes } = flatProject(PROGRAM);
    assert.equal(isImageStale(project, bytes), false);
    assert.equal(isImageStale(project, Buffer.from([...PROGRAM, 0])), true);
    assert.equal(isImageStale(project, Buffer.from(PROGRAM.map((value) => value ^ 1))), true);
    assert.equal(isVerificationStale(project), true);
    const verified = { ...project, verification: { ok: true, digest: projectDigest(project), at: 'x' } };
    assert.equal(isVerificationStale(verified), false);
    const changed = applyEdits(verified, [{ op: 'setComment', address: 0xc000, kind: 'inline', text: 'x' }]).project;
    assert.equal(isVerificationStale(changed), true);
    assert.equal(projectDigest(cloneProject(project)), projectDigest(project));
  });

  it('创建工程时布局的映像大小必须与字节数一致，符号集文件名必须是 .inc', () => {
    const buffer = Buffer.alloc(4);
    const { layout } = parseFlat(buffer, { address: 0x8000 });
    assert.throws(() => createProject({ imagePath: 'a', bytes: Buffer.alloc(5), format: 'flat', layout }), (error) => error.code === 'DISASM_PROJECT_INVALID');
    assert.throws(() => createProject({ imagePath: 'a', bytes: buffer, format: 'flat', layout, symbolSets: [{ name: 'x.txt', path: 'x' }] }), /应为 xxx\.inc/);
  });
});

describe('批量编辑', () => {
  it('符号：定义、重命名（清除自动标记，同步提示）、修改、删除；名字与地址不能重复', () => {
    const { project } = flatProject(PROGRAM);
    const named = applyEdits(project, [
      { op: 'defineSymbol', name: 'start', address: 0xc000, kind: 'procedure' },
      { op: 'defineSymbol', name: 'ram', address: 0x0010 },
      { op: 'setHint', address: 0xc000, form: 'symbol', symbol: 'start' },
    ]).project;
    assert.deepEqual(named.symbols.map((symbol) => [symbol.name, symbol.segmentId, symbol.kind]), [['start', 0, 'procedure'], ['ram', null, 'variable']]);
    const renamed = applyEdits(named, [{ op: 'renameSymbol', from: 'start', to: 'main' }, { op: 'updateSymbol', name: 'main', description: '主程序', module: null }]).project;
    assert.equal(renamed.symbols[0].name, 'main');
    assert.equal(renamed.symbols[0].description, '主程序');
    assert.equal(renamed.hints[0].symbol, 'main');
    assert.throws(() => applyEdits(named, [{ op: 'defineSymbol', name: 'start', address: 0xc001 }]), /已经存在/);
    assert.throws(() => applyEdits(named, [{ op: 'defineSymbol', name: 'other', address: 0xc000 }]), /已经有符号 start/);
    assert.throws(() => applyEdits(named, [{ op: 'removeSymbol', name: 'start' }]), /还被操作数提示引用着/);
    assert.equal(applyEdits(named, [{ op: 'removeSymbol', name: 'ram' }]).project.symbols.length, 1);
  });

  it('符号名必须是合法标识符，不能是助记符或寄存器名', () => {
    const { project } = flatProject(PROGRAM);
    for (const name of ['1abc', 'a-b', 'lda', 'LDA', 'x', '', 'a b']) {
      assert.throws(() => applyEdits(project, [{ op: 'defineSymbol', name, address: 0xc000 }]), (error) => error.code === 'DISASM_SYMBOL_INVALID', name);
    }
  });

  it('区域、注释、提示、模块、入口：用 CPU 地址定位；换页窗口里要给段号', () => {
    const bytes = Buffer.alloc(32, 0xea);
    const layout = createLayout([{ id: 0, fileOffset: 0, length: 16, address: 0x8000, bank: 0 }, { id: 1, fileOffset: 16, length: 16, address: 0x8000, bank: 1 }], { imageSize: 32 });
    const project = createProject({ imagePath: 'b.bin', bytes, format: 'flat', layout });
    assert.throws(() => applyEdits(project, [{ op: 'setComment', address: 0x8000, kind: 'block', text: 'x' }]), /多个 bank/);
    const edited = applyEdits(project, [
      { op: 'setRegion', segmentId: 1, address: 0x8004, length: 4, type: 'bytes', params: { perLine: 2 } },
      { op: 'setComment', segmentId: 1, address: 0x8000, kind: 'block', text: 'bank 1' },
      { op: 'addEntry', segmentId: 0, address: 0x8000, description: '入口' },
      { op: 'assignModule', name: 'b1', segmentId: 1, address: 0x8000, length: 16 },
    ]).project;
    assert.deepEqual(edited.regions.regions.map((region) => [region.offset, region.length, region.source]), [[20, 4, 'manual']]);
    assert.deepEqual(edited.comments, [{ segmentId: 1, address: 0x8000, kind: 'block', text: 'bank 1' }]);
    assert.deepEqual(edited.entries.map((entry) => [entry.segmentId, entry.source]), [[0, 'manual']]);
    assert.throws(() => applyEdits(edited, [{ op: 'addEntry', segmentId: 0, address: 0x8000 }]), /已经存在/);
    assert.throws(() => applyEdits(edited, [{ op: 'assignModule', name: 'dup', segmentId: 1, address: 0x8008, length: 4 }]), /重叠/);
    assert.equal(applyEdits(edited, [{ op: 'removeModule', name: 'b1' }]).project.modules.length, 0);
  });

  it('注释与提示：同一位置的旧项被替换；行尾注释不能换行；没有可删的项时报错', () => {
    const { project } = flatProject(PROGRAM);
    const edited = applyEdits(project, [
      { op: 'setComment', address: 0xc000, kind: 'inline', text: '第一版' },
      { op: 'setComment', address: 0xc000, kind: 'inline', text: '第二版' },
      { op: 'setHint', address: 0xc000, form: 'decimal' },
      { op: 'setHint', address: 0xc000, form: 'binary' },
    ]).project;
    assert.deepEqual(edited.comments.map((comment) => comment.text), ['第二版']);
    assert.deepEqual(edited.hints.map((hint) => hint.form), ['binary']);
    assert.throws(() => applyEdits(project, [{ op: 'setComment', address: 0xc000, kind: 'inline', text: 'a\nb' }]), /不能换行/);
    assert.throws(() => applyEdits(project, [{ op: 'removeComment', address: 0xc000, kind: 'block' }]), /没有块注释/);
    assert.throws(() => applyEdits(project, [{ op: 'removeHint', address: 0xc000 }]), /没有操作数提示/);
    assert.throws(() => applyEdits(project, [{ op: 'setHint', address: 0xc000, form: 'low' }]), /要指明符号/);
    assert.throws(() => applyEdits(project, [{ op: 'setHint', address: 0xc000, form: 'symbol', symbol: 'nothing' }]), /不存在/);
  });

  it('整批要么全部生效，要么都不生效：出错时指出项号，原工程不变', () => {
    const { project } = flatProject(PROGRAM);
    const before = serializeProject(project);
    assert.throws(
      () => applyEdits(project, [{ op: 'defineSymbol', name: 'ok', address: 0xc000 }, { op: 'defineSymbol', name: 'bad name', address: 0xc001 }]),
      (error) => error.code === 'DISASM_SYMBOL_INVALID' && error.message.startsWith('第 2 项（defineSymbol）：'),
    );
    assert.equal(serializeProject(project), before);
    assert.throws(() => applyEdits(project, [{ op: 'explode' }]), (error) => error.code === 'DISASM_EDIT_INVALID' && error.message.includes('第 1 项') && error.hint.includes('defineSymbol'));
    assert.deepEqual(EDIT_OPERATIONS.includes('setRegion') && EDIT_OPERATIONS.includes('assignModule'), true);
  });
});

describe('追踪工程与自动命名', () => {
  it('追踪：写入 trace 区域，保留手工区域，留下未解析项；重复追踪结果不变', () => {
    const { project, bytes } = flatProject([0xea, 0x60, 0x6c, 0x00, 0x02, 0x60], { entries: [0xc000, 0xc002] });
    const marked = applyEdits(project, [{ op: 'setRegion', address: 0xc005, length: 1, type: 'bytes' }]).project;
    const first = traceProject(marked, bytes);
    assert.deepEqual(first.project.regions.regions.map((region) => [region.offset, region.length, region.type, region.source]), [[0, 5, 'code', 'trace'], [5, 1, 'bytes', 'manual']]);
    assert.deepEqual(first.project.unresolved, [{ kind: 'indirect-jump', segmentId: 0, address: 0xc002, offset: 2, pointer: 0x0200 }]);
    assert.deepEqual(first.summary, { instructions: 3, tables: 0, warnings: 0 });
    assert.equal(serializeProject(traceProject(first.project, bytes).project), serializeProject(first.project));
  });

  it('自动命名：调用 sub_、跳转与分支 loc_、数据 data_、布局外 var_；已有符号与符号集里的名字不再生成；重复运行不增加', () => {
    const { project, bytes } = flatProject([0x20, 0x0d, 0xc0, 0xad, 0x0e, 0xc0, 0xa5, 0x20, 0xd0, 0x00, 0x4c, 0x0f, 0xc0, 0x60, 0x60, 0x60], { entries: [0xc000] });
    const traced = traceProject(project, bytes).project;
    const { project: named, added } = ensureAutoSymbols(applyEdits(traced, [{ op: 'defineSymbol', name: 'mine', address: 0xc00d, kind: 'label' }]).project, bytes, { externalNames: new Map([[0x20, 'REG']]) });
    assert.deepEqual(named.symbols.map((symbol) => [symbol.name, symbol.address, symbol.kind, symbol.auto]), [
      ['mine', 0xc00d, 'label', false],
      ['sub_C000', 0xc000, 'procedure', true],
      ['loc_C00A', 0xc00a, 'label', true],
      ['data_C00E', 0xc00e, 'data', true],
      ['loc_C00F', 0xc00f, 'label', true],
    ]);
    assert.equal(added, 4);
    assert.equal(ensureAutoSymbols(named, bytes, { externalNames: new Map([[0x20, 'REG']]) }).added, 0);
  });

  it('目标落在指令中间时符号放在那条指令的开头；向量入口叫 reset、nmi、irq', () => {
    const { project, bytes } = flatProject([0xa9, 0x00, 0x4c, 0x01, 0xc0], { entries: [0xc000] });
    const withVector = { ...project, entries: [{ ...project.entries[0], source: 'vector', description: '复位向量' }] };
    const { project: named } = ensureAutoSymbols(traceProject(withVector, bytes).project, bytes);
    assert.deepEqual(named.symbols.map((symbol) => symbol.name), ['reset']);
  });

  it('换页布局里的自动符号名带 bank 号', () => {
    const bytes = Buffer.from([0x20, 0x03, 0x80, 0x60, 0xff, 0xff, 0xff, 0xff]);
    const layout = createLayout([{ id: 0, fileOffset: 0, length: 4, address: 0x8000, bank: 3 }, { id: 1, fileOffset: 4, length: 4, address: 0x8000, bank: 4 }], { imageSize: 8 });
    const project = createProject({ imagePath: 'c.bin', bytes, format: 'flat', layout, entries: [{ segmentId: 0, address: 0x8000, source: 'manual', description: '' }] });
    const { project: named } = ensureAutoSymbols(traceProject(project, bytes).project, bytes);
    assert.deepEqual(named.symbols.map((symbol) => symbol.name), ['sub_b3_8000', 'sub_b3_8003']);
  });
});
