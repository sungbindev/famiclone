// ------------------------------------------------------------------------
// 名称：disasm-operations.test.mjs
// 说明：测试 Copilot 反汇编工具对应的操作：新建与打开、追踪、列表、清单、交叉引用、例程、调用图、搜索、十六进制、识别、批量编辑、符号集、布局、生成与回读。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：夹具是一个 22 字节的平铺小程序（$C000 起：调用 $C00B 处的延时例程、写 $2000、死循环，后面是字符串 HELLO）；回读需要 cc65 工具链，没有时跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { API_VERSION, OPERATION_NAMES, parseProject, runOperation } from '../src/disasm/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN, writeTestFile } from './helpers.mjs';

/** $C000 起：jsr $C00B；lda #5；sta $2000；jmp $C008；延时例程 ldx #$10、dex、bne、rts；字符串 HELLO */
const PROGRAM = Buffer.from([
  0x20, 0x0b, 0xc0, 0xa9, 0x05, 0x8d, 0x00, 0x20, 0x4c, 0x08, 0xc0,
  0xa2, 0x10, 0xca, 0xd0, 0xfd, 0x60,
  0x48, 0x45, 0x4c, 0x4c, 0x4f,
]);

describe('反汇编操作', () => {
  let temp;
  let projectDir;
  let imageFile;

  beforeEach(() => {
    temp = createTempDir();
    projectDir = path.join(temp.dir, 'project');
    imageFile = path.join(temp.dir, 'game.bin');
    writeFileSync(imageFile, PROGRAM);
  });
  afterEach(() => temp.cleanup());

  /** 运行操作，projectDir 默认取夹具的工程目录 */
  const run = (name, params = {}) => runOperation(name, { projectDir, ...params });

  /** 新建工程并追踪 */
  async function openAndTrace() {
    await run('open', { image: imageFile, address: '$C000', entries: ['$C000'] });
    return run('trace');
  }

  it('操作表有 17 个操作，接口版本为 1', () => {
    assert.equal(API_VERSION, 1);
    assert.deepEqual([...OPERATION_NAMES].sort(), ['apply', 'callgraph', 'detect', 'find', 'generate', 'hex', 'layout', 'list', 'open', 'reference', 'routine', 'status', 'symbols', 'trace', 'verify', 'view', 'xrefs']);
  });

  it('新建工程：布局、入口、进度；再次打开不用 image；已有工程时不能直接重建', async () => {
    const created = await run('open', { image: imageFile, address: 0xc000, entries: ['$C000'] });
    assert.equal(created.created, true);
    assert.deepEqual(created.segments.map((segment) => [segment.id, segment.address, segment.length]), [[0, 0xc000, 22]]);
    assert.equal(created.counts.entries, 1);
    assert.equal(created.status.verification.state, 'none');
    const reopened = await run('open');
    assert.equal(reopened.created, false);
    assert.equal(reopened.format, 'flat');
    await assert.rejects(run('open', { image: imageFile, address: 0xc000 }), (error) => error.code === 'DISASM_PROJECT_EXISTS');
    assert.equal((await run('open', { image: imageFile, address: 0xc000, overwrite: true })).counts.entries, 0);
  });

  it('参数不对时报错：未知操作、参数不是对象、缺 projectDir、平铺格式没有地址、映像摘要变了', async () => {
    await assert.rejects(runOperation('nothing', {}), (error) => error.code === 'DISASM_OPERATION_UNKNOWN' && error.hint.includes('trace'));
    await assert.rejects(runOperation('status', null), (error) => error.code === 'DISASM_PARAM_INVALID');
    await assert.rejects(runOperation('status', {}), (error) => error.code === 'DISASM_PARAM_INVALID' && error.message.includes('projectDir'));
    await assert.rejects(run('open', { image: imageFile }), (error) => error.code === 'ARGUMENT_MISSING');
    await run('open', { image: imageFile, address: 0xc000, entries: [0xc000] });
    writeFileSync(imageFile, Buffer.alloc(22, 0xea));
    await assert.rejects(run('status'), (error) => error.code === 'DISASM_IMAGE_MISMATCH');
  });

  it('追踪：指令数、例程数、新符号；再次追踪没有新增', async () => {
    const first = await openAndTrace();
    assert.equal(first.instructions, 8);
    assert.equal(first.added, 8);
    assert.equal(first.routines, 2);
    assert.ok(first.newSymbols >= 4);
    assert.deepEqual(first.unresolved, { indirectJumps: 0, crossBank: 0 });
    const second = await run('trace');
    assert.equal(second.added, 0);
    assert.equal(second.newSymbols, 0);
    assert.equal(second.entriesAdded, 0);
  });

  it('列表：入口、例程、没有识别的字节、自动命名的符号、没有说明的例程；分页', async () => {
    await openAndTrace();
    assert.deepEqual((await run('list', { category: 'routines' })).items.map((item) => [item.name, item.size, item.commented]), [['sub_C000', 11, false], ['sub_C00B', 6, false]]);
    assert.deepEqual((await run('list', { category: 'unknown' })).items.map((item) => [item.address, item.length]), [[0xc011, 5]]);
    assert.equal((await run('list', { category: 'entries' })).total, 1);
    assert.equal((await run('list', { category: 'uncommented-routines' })).total, 2);
    const page = await run('list', { category: 'auto-symbols', limit: 2 });
    assert.equal(page.items.length, 2);
    assert.equal(page.nextCursor, '2');
    assert.equal((await run('list', { category: 'auto-symbols', limit: 2, cursor: page.nextCursor })).items[0].name, (await run('list', { category: 'auto-symbols' })).items[2].name);
    await assert.rejects(run('list', { category: 'bad' }), (error) => error.code === 'DISASM_PARAM_INVALID');
  });

  it('清单：例程与地址范围，符号名代替地址，数据行标出区域类型', async () => {
    await openAndTrace();
    const routine = await run('view', { routine: 'sub_C00B' });
    assert.match(routine.text, /^sub_C00B:/m);
    assert.match(routine.text, /ldx #\$10/);
    assert.match(routine.text, /bne loc_C00D/);
    const range = await run('view', { address: '$C000', length: 22 });
    assert.match(range.text, /jsr sub_C00B/);
    assert.match(range.text, /sta var_2000/);
    assert.match(range.text, /48 45 4C 4C 4F\s+\.byte \$48,\$45,\$4C,\$4C,\$4F {2}; unknown {2}"HELLO"/);
    const cut = await run('view', { address: 0xc000, length: 22, maxLines: 3 });
    assert.equal(cut.truncated, true);
    assert.ok(cut.nextAddress > 0xc000);
    await assert.rejects(run('view', { address: 0x1234 }), (error) => error.code === 'DISASM_PARAM_INVALID');
    await assert.rejects(run('view', { routine: 'nope' }), (error) => error.code === 'DISASM_ROUTINE_NOT_FOUND');
  });

  it('交叉引用、例程上下文与调用图', async () => {
    await openAndTrace();
    const refs = await run('xrefs', { symbol: 'sub_C00B' });
    assert.deepEqual(refs.items.map((item) => [item.kind, item.address, item.routine]), [['call', 0xc000, 'sub_C000']]);
    assert.equal(refs.target.name, 'sub_C00B');
    assert.deepEqual((await run('xrefs', { address: '$2000', kind: 'write' })).items.map((item) => item.mnemonic), ['sta']);

    const delay = await run('routine', { routine: 'sub_C00B' });
    assert.deepEqual(delay.callers.map((caller) => [caller.kind, caller.from]), [['call', 'sub_C000']]);
    assert.deepEqual(delay.registers, { inputs: [], outputs: ['x'] });
    assert.deepEqual(delay.memory, []);
    const main = await run('routine', { address: '$C005' });
    assert.equal(main.name, 'sub_C000');
    assert.deepEqual(main.callees.map((callee) => [callee.name, callee.kinds]), [['sub_C00B', ['call']]]);
    assert.deepEqual(main.memory.map((item) => [item.name, item.read, item.write]), [['var_2000', 0, 1]]);
    assert.deepEqual(main.registers.outputs, ['a']);

    const graph = await run('callgraph', { routine: 'sub_C000', depth: 2 });
    assert.equal(graph.text, 'sub_C000 ($C000)\n  sub_C00B ($C00B)');
    assert.equal((await run('callgraph', { routine: 'sub_C00B', direction: 'callers' })).text, 'sub_C00B ($C00B)\n  sub_C000 ($C000)');
  });

  it('搜索：字节、文本、指令序列；十六进制视图带符号与区域', async () => {
    await openAndTrace();
    const bytes = await run('find', { kind: 'bytes', pattern: 'A9 ??' });
    assert.deepEqual(bytes.matches.map((match) => match.address), [0xc003]);
    assert.deepEqual((await run('find', { kind: 'text', text: 'HELLO' })).matches.map((match) => match.address), [0xc011]);
    assert.equal((await run('find', { kind: 'text', text: 'hello' })).matches.length, 0);
    const instructions = await run('find', { kind: 'instructions', pattern: 'lda #*; sta $2000' });
    assert.deepEqual(instructions.matches.map((match) => match.text), [['$C003  lda #$05', '$C005  sta $2000']]);
    assert.equal((await run('find', { kind: 'instructions', pattern: 'lda #*; rts' })).matches.length, 0);
    const paged = await run('find', { kind: 'bytes', pattern: '4C', limit: 1 });
    assert.equal(paged.matches.length, 1);
    assert.equal(paged.matches[0].address, 0xc008);
    assert.equal(paged.nextCursor, '19');
    const second = await run('find', { kind: 'bytes', pattern: '4C', limit: 1, cursor: paged.nextCursor });
    assert.deepEqual(second.matches.map((match) => match.address), [0xc013]);

    const view = await run('hex', { address: '$C000', length: 22 });
    assert.match(view.text, /\$C00B sub_C00B/);
    assert.deepEqual(view.regions.map((region) => [region.address, region.length, region.type]), [[0xc000, 17, 'code'], [0xc011, 5, 'unknown']]);
    await assert.rejects(run('find', { kind: 'bytes', pattern: 'GG' }), (error) => error.code === 'DISASM_PARAM_INVALID');
  });

  it('识别候选：没标注的字符串', async () => {
    await openAndTrace();
    const result = await run('detect', { types: ['string'] });
    assert.deepEqual(result.items.map((item) => [item.type, item.address, item.length]), [['string', 0xc011, 5]]);
  });

  it('批量编辑：改名、说明、注释；出错时整批不改', async () => {
    await openAndTrace();
    const done = await run('apply', {
      edits: [
        { op: 'renameSymbol', from: 'sub_C00B', to: 'delay' },
        { op: 'updateSymbol', name: 'delay', description: '延时' },
        { op: 'setComment', address: '$C00B', kind: 'block', text: '延时例程' },
        { op: 'setRegion', address: '$C011', length: 5, type: 'string', params: { encoding: 'ascii' } },
      ],
    });
    assert.equal(done.applied, 4);
    assert.equal(done.proceduresWithoutComment, 1);
    const text = (await run('view', { routine: 'delay' })).text;
    assert.match(text, /; 延时例程\ndelay:  ; 延时/);
    assert.equal((await run('list', { category: 'unknown' })).total, 0);

    const before = readFileSync(path.join(projectDir, 'disasm.json'), 'utf8');
    await assert.rejects(run('apply', { edits: [{ op: 'renameSymbol', from: 'delay', to: 'slow' }, { op: 'renameSymbol', from: 'missing', to: 'x' }] }), (error) => error.code === 'DISASM_EDIT_INVALID' && error.message.includes('第 2 项'));
    assert.equal(readFileSync(path.join(projectDir, 'disasm.json'), 'utf8'), before);
    await assert.rejects(run('apply', { edits: [] }), (error) => error.code === 'DISASM_PARAM_INVALID');
  });

  it('符号集：导入、搜索、与工程符号重名时拒绝；reference 查指令与符号', async () => {
    await openAndTrace();
    const regs = writeTestFile(temp.dir, 'regs.inc', 'PPUCTRL = $2000  ; 控制\nPPUMASK = $2001\n');
    const imported = await run('symbols', { action: 'import', file: regs });
    assert.deepEqual([imported.imported, imported.constants], ['regs.inc', 2]);
    assert.equal(parseProject(readFileSync(path.join(projectDir, 'disasm.json'), 'utf8')).symbolSets[0].name, 'regs.inc');
    await assert.rejects(run('symbols', { action: 'import', file: regs }), (error) => error.code === 'DISASM_SYMBOL_SET_DUPLICATE');
    assert.deepEqual((await run('symbols', { action: 'search', query: 'ppum' })).items.map((item) => [item.source, item.name, item.address]), [['regs.inc', 'PPUMASK', 0x2001]]);
    assert.equal((await run('symbols', { scope: 'project', kind: 'procedure' })).total, 2);
    const clash = writeTestFile(temp.dir, 'clash.inc', 'sub_C000 = $10\n');
    await assert.rejects(run('symbols', { action: 'import', file: clash }), (error) => error.code === 'DISASM_SYMBOL_DUPLICATE');

    const lda = await run('reference', { query: 'LDA' });
    assert.ok(lda.instruction.forms.some((form) => form.opcode === '$A9' && form.mode === 'immediate' && form.syntax === 'lda #$nn'));
    assert.equal(lda.symbols.total, 0);
    const found = await run('reference', { query: '$2001' });
    assert.deepEqual(found.symbols.items.map((item) => item.name), ['PPUMASK']);
  });

  it('布局：查看；换成两个段后符号与入口仍有效，区域清空；新布局无效时报错', async () => {
    await openAndTrace();
    const view = await run('layout');
    assert.equal(view.changed, false);
    assert.deepEqual(view.uncovered, []);
    const changed = await run('layout', {
      segments: [{ id: 0, fileOffset: 0, length: 11, address: 0xc000 }, { id: 1, fileOffset: 11, length: 11, address: 0xc00b }],
    });
    assert.equal(changed.changed, true);
    assert.deepEqual(changed.dropped, { entries: 0, symbols: 0, comments: 0, hints: 0, modules: 0, regions: 0 });
    assert.equal(changed.segments.length, 2);
    assert.equal((await run('status')).status.symbols.total, (await run('list', { category: 'auto-symbols' })).total);
    assert.equal((await run('list', { category: 'routines' })).items.find((item) => item.name === 'sub_C00B').segmentId, 1);
    await run('trace');
    await assert.rejects(run('layout', { segments: [{ id: 0, fileOffset: 0, length: 30, address: 0xc000 }] }), (error) => error.code === 'DISASM_LAYOUT_INVALID');
  });

  it('追踪时可以带入新入口', async () => {
    await run('open', { image: imageFile, address: 0xc000, entries: ['$C000'] });
    const result = await run('trace', { entries: ['$C00B', { address: '$C000' }] });
    assert.equal(result.entriesAdded, 1);
    assert.equal((await run('list', { category: 'entries' })).total, 2);
  });

  it('生成源码工程', async () => {
    await openAndTrace();
    const generated = await run('generate');
    assert.deepEqual(generated.modules, ['main']);
    assert.ok(generated.files.includes('src/main.s'));
    assert.match(readFileSync(path.join(projectDir, 'src', 'main.s'), 'utf8'), /jsr sub_C00B/);
  });

  describe('回读（需要 cc65）', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
    it('回读一致，结果记进工程；之后编辑使回读结果过期', async () => {
      await openAndTrace();
      const result = await run('verify');
      assert.equal(result.ok, true);
      assert.equal(result.verification.state, 'ok');
      assert.equal((await run('status')).status.verification.state, 'ok');
      await run('apply', { edits: [{ op: 'renameSymbol', from: 'sub_C00B', to: 'delay' }] });
      assert.equal((await run('status')).status.verification.state, 'stale');
      assert.equal((await run('verify')).ok, true);
    });
  });
});
