// ------------------------------------------------------------------------
// 名称：disasm-workflow.test.mjs
// 说明：按 skill disassemble-6502 的八个步骤，用反汇编操作把示例工程（apps/disasm-example）完整做一遍，并核对每步的完成标准。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：这里的操作序列就是 Copilot 工具会发出的调用，名字与注释是按固定规则生成的，只验证流程与完成标准能达到，不评价命名质量；需要 cc65，没有时跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { cpSync, existsSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { fileURLToPath } from 'node:url';
import { buildPath } from '../src/build/index.mjs';
import { runOperation } from '../src/disasm/index.mjs';
import { createTempDir, SKIP_WITHOUT_TOOLCHAIN } from './helpers.mjs';

/** 示例工程目录 */
const EXAMPLE_DIR = fileURLToPath(new URL('../../../apps/disasm-example/', import.meta.url));

/** 示例里没有被调用到的库代码起点后，追踪新入口时一次最多加几轮 */
const MAX_ROUNDS = 10;

/** 比这更短的“没有识别的字节”当作遗漏的代码，更长的是数据 */
const SHORT_UNKNOWN = 100;

describe('按 skill 的八个步骤反汇编示例工程（需要 cc65）', { skip: SKIP_WITHOUT_TOOLCHAIN }, () => {
  let temp;
  let projectDir;

  beforeEach(() => {
    temp = createTempDir();
    projectDir = path.join(temp.dir, 'disasm');
  });
  afterEach(() => temp.cleanup());

  /** 运行操作，projectDir 取夹具的工程目录 */
  const run = (name, params = {}) => runOperation(name, { projectDir, ...params });

  /** 取工程的进度报告 */
  async function progress() {
    return (await run('status')).status;
  }

  it('识别、追踪、区域分类、命名、注释、分层、生成与回读都达到完成标准', async () => {
    cpSync(EXAMPLE_DIR, path.join(temp.dir, 'example'), { recursive: true });
    const { result } = await buildPath({ pathArg: 'src', cwd: path.join(temp.dir, 'example') });

    // 1 识别：目标描述决定格式与符号集，向量成为入口
    const opened = await run('open', { image: result.outputFile, target: 'nes' });
    assert.deepEqual([opened.format, opened.symbolSets, opened.counts.entries], ['ines', ['fcnes.inc'], 3]);
    const layout = await run('layout');
    assert.deepEqual(layout.segments.map((segment) => [segment.address, segment.length]), [[0x8000, 0x8000]]);
    assert.deepEqual((await run('list', { category: 'entries' })).items.map((item) => item.source), ['vector', 'vector', 'vector']);

    // 2 追踪：静态追踪之后，短的“没有识别”的范围多半是没被调用的库代码，逐个加入口再追踪
    const traced = await run('trace');
    assert.deepEqual(traced.unresolved, { indirectJumps: 0, crossBank: 0 });
    for (let round = 0; round < MAX_ROUNDS; round += 1) {
      const short = (await run('list', { category: 'unknown' })).items.filter((item) => item.length < SHORT_UNKNOWN);
      if (short.length === 0) break;
      await run('trace', { entries: short.map((item) => item.address) });
    }
    assert.ok((await run('list', { category: 'unknown' })).items.every((item) => item.length >= SHORT_UNKNOWN));

    // 3 区域分类：候选只是线索，用依据核实后标注
    const candidates = (await run('detect')).items;
    const text = candidates.find((candidate) => candidate.type === 'string' && /HELLO, NES!/.test(candidate.evidence));
    assert.ok(text, '应当识别出字符串 HELLO, NES!');
    const padding = candidates.find((candidate) => candidate.type === 'padding' && candidate.params.value === 0);
    const vectors = candidates.find((candidate) => candidate.type === 'words' && candidate.address === 0xfffa);
    assert.ok(padding && vectors);
    // 调色板数据写进 $2007，被当作图形候选，它的长度只是上限；看循环的次数才知道是 32 字节
    const tail = (await run('list', { category: 'unknown' })).items.at(-1);
    const paletteLength = text.address - tail.address;
    assert.equal(paletteLength, 32);
    await run('apply', {
      edits: [
        { op: 'setRegion', address: tail.address, length: paletteLength, type: 'bytes', params: { perLine: 8 } },
        { op: 'setRegion', address: text.address, length: text.length, type: 'string', params: text.params },
        { op: 'setRegion', address: padding.address, length: padding.length, type: 'padding', params: padding.params },
        { op: 'setRegion', address: vectors.address, length: vectors.length, type: 'words', params: { perLine: 3, adjust: 0 } },
      ],
    });
    // 字符串与填充之间还剩一段：带 $FFFF 占位的字节，是复制到 RAM 里再改写的代码模板，不能标成 code
    const rest = (await run('list', { category: 'unknown' })).items;
    assert.deepEqual(rest.map((item) => [item.address, item.length]), [[text.address + text.length, padding.address - text.address - text.length]]);
    await run('apply', {
      edits: [
        { op: 'setRegion', address: rest[0].address, length: rest[0].length, type: 'bytes', params: { perLine: 8 } },
        { op: 'setComment', address: rest[0].address, kind: 'block', text: '未确认：复制到 RAM 再改写 $FFFF 占位的代码模板' },
      ],
    });
    assert.equal((await run('list', { category: 'unknown' })).total, 0);

    // 4 命名：自动符号一次换完；向量入口与例程用有含义的名字，其余按种类与地址起名
    const autoSymbols = (await run('list', { category: 'auto-symbols', limit: 500 })).items;
    assert.ok(autoSymbols.length > 20);
    const fixedNames = new Map([[tail.address, 'palette'], [text.address, 'message']]);
    const renames = autoSymbols.map((symbol) => {
      const hex = symbol.address.toString(16).toUpperCase().padStart(4, '0');
      const prefix = { procedure: 'routine', label: 'label', variable: 'ram', data: 'table', constant: 'const' }[symbol.kind];
      const keep = ['reset', 'nmi', 'irq'].includes(symbol.name);
      return { op: 'renameSymbol', from: symbol.name, to: keep ? symbol.name : (fixedNames.get(symbol.address) ?? `${prefix}_${hex}`) };
    });
    await run('apply', { edits: renames });
    assert.equal((await progress()).symbols.auto, 0);

    // 5 注释：每个例程一条块注释，关键地址一条行尾注释
    const routines = (await run('list', { category: 'routines' })).items;
    assert.equal((await progress()).proceduresWithoutComment, routines.length);
    await run('apply', {
      edits: [
        ...routines.map((routine) => ({ op: 'setComment', address: routine.address, kind: 'block', text: `${routine.name}：示例注释（作用、参数、返回值、副作用）` })),
        { op: 'setComment', address: routines[0].address, kind: 'inline', text: '复位入口' },
      ],
    });
    assert.equal((await progress()).proceduresWithoutComment, 0);

    // 6 分层：代码一个模块，数据与向量一个模块
    const codeEnd = Math.max(...routines.map((routine) => routine.end));
    await run('apply', {
      edits: [
        { op: 'assignModule', name: 'code', address: 0x8000, length: codeEnd - 0x8000, description: '启动、中断与库代码' },
        { op: 'assignModule', name: 'data', address: codeEnd, length: 0x10000 - codeEnd, description: '调色板、字符串与向量' },
      ],
    });

    // 7 生成与校验：回读一致；之后的编辑使结果过期
    const generated = await run('generate');
    assert.deepEqual(generated.modules.sort(), ['code', 'data']);
    for (const file of ['src/code.s', 'src/data.s', 'include/fcnes.inc', 'link.cfg']) assert.ok(existsSync(path.join(projectDir, file)), file);
    const verified = await run('verify');
    assert.equal(verified.ok, true, JSON.stringify(verified));
    const finished = await progress();
    assert.deepEqual([finished.verification.state, finished.symbols.auto, finished.proceduresWithoutComment, finished.regions.unknown.bytes], ['ok', 0, 0, 0]);
    assert.match(readFileSync(path.join(projectDir, 'src', 'code.s'), 'utf8'), /PPUADDR/);

    await run('apply', { edits: [{ op: 'setComment', address: routines[0].address, kind: 'inline', text: '改过的注释' }] });
    assert.equal((await progress()).verification.state, 'stale');
    assert.equal((await run('verify')).ok, true);
  });
});
