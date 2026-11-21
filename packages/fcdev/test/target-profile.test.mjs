// ------------------------------------------------------------------------
// 名称：target-profile.test.mjs
// 说明：测试目标描述：内置的 nes 与 sb2000 描述、自带的描述文件、错误提示；反汇编 open 按目标描述选格式、符号集与图形数据端口；模拟器 load 按描述执行启动步骤。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：夹具都是手工拼的小映像，不需要 cc65；模拟器用假的调试客户端，只检查发出的请求与顺序。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { runOperation } from '../src/disasm/index.mjs';
import { runEmulatorOperation } from '../src/emulator/index.mjs';
import { closeSession, openSession } from '../src/emulator/operations/session.mjs';
import { BOOT_STEP_NAMES, listBuiltinProfiles, readTargetProfile, selectProfileInput } from '../src/shared/index.mjs';
import { createTempDir, writeTestFile } from './helpers.mjs';

describe('目标描述', () => {
  let temp;

  beforeEach(() => { temp = createTempDir(); });
  afterEach(() => temp.cleanup());

  it('内置的描述：nes 与 sb2000，符号集文件存在，图形数据端口是 $2007', () => {
    assert.deepEqual(listBuiltinProfiles(), ['nes', 'sb2000']);
    for (const name of ['nes', 'sb2000']) {
      const { profile } = readTargetProfile(name);
      assert.equal(profile.name, name);
      assert.deepEqual(profile.graphicsPorts, [0x2007]);
      assert.ok(profile.symbolSets.length === 1 && existsSync(profile.symbolSets[0]), `${name} 的符号集文件`);
    }
    assert.deepEqual(Object.keys(readTargetProfile('nes').profile.inputs), ['cartridge', 'family-basic']);
    assert.deepEqual(Object.keys(readTargetProfile('sb2000').profile.inputs), ['program', 'bios']);
  });

  it('内置描述里的启动步骤都是认识的步骤，选输入：默认输入、指定输入、不存在的输入', () => {
    const { profile } = readTargetProfile('sb2000');
    for (const input of Object.values(profile.inputs)) assert.ok(input.boot.every((step) => BOOT_STEP_NAMES.includes(step.step)));
    assert.equal(selectProfileInput(profile, undefined).name, 'program');
    assert.equal(selectProfileInput(profile, 'bios').format, 'paged');
    assert.throws(() => selectProfileInput(profile, 'tape'), (error) => error.code === 'PROFILE_INPUT_UNKNOWN' && error.hint.includes('bios'));
  });

  it('自带的描述文件：符号集路径相对描述文件；只有一种输入时它就是默认输入', () => {
    const file = writeTestFile(temp.dir, 'x/my.json', JSON.stringify({ name: 'my', symbolSets: ['regs.inc'], graphicsPorts: [8199, '$2006'], inputs: { rom: { format: 'flat', formatOptions: { address: 32768 } } } }));
    const { profile } = readTargetProfile(file);
    assert.deepEqual([profile.name, profile.defaultInput, profile.graphicsPorts], ['my', 'rom', [8199, 0x2006]]);
    assert.deepEqual(profile.symbolSets, [path.join(temp.dir, 'x', 'regs.inc')]);
    assert.deepEqual(readTargetProfile('my.json', { cwd: path.join(temp.dir, 'x') }).profile.name, 'my');
  });

  it('找不到、不是 JSON、结构不对时报错并指出位置', () => {
    assert.throws(() => readTargetProfile('nothing'), (error) => error.code === 'PROFILE_NOT_FOUND' && error.hint.includes('sb2000'));
    const broken = writeTestFile(temp.dir, 'broken.json', '{ not json');
    assert.throws(() => readTargetProfile(broken), (error) => error.code === 'PROFILE_INVALID' && error.message.includes('broken.json'));
    const cases = [
      [{}, '要写 name'],
      [{ name: 'a' }, 'inputs'],
      [{ name: 'a', inputs: { x: { format: 'flat' } }, defaultInput: 'y' }, 'defaultInput'],
      [{ name: 'a', inputs: { x: {} } }, 'format'],
      [{ name: 'a', inputs: { x: { format: 'flat', boot: [{ step: 'dance' }] } } }, '不认识'],
      [{ name: 'a', inputs: { x: { format: 'flat', boot: [{ step: 'run' }] } } }, '缺少字段 frames'],
      [{ name: 'a', inputs: { x: { format: 'flat', boot: [{ step: 'run', frames: 5, extra: 1 }] } } }, '没有字段 extra'],
      [{ name: 'a', graphicsPorts: ['zz'], inputs: { x: { format: 'flat' } } }, '16 位地址'],
    ];
    for (const [data, text] of cases) {
      const file = writeTestFile(temp.dir, 'case.json', JSON.stringify(data));
      assert.throws(() => readTargetProfile(file), (error) => error.code === 'PROFILE_INVALID' && error.message.includes(text), JSON.stringify(data));
    }
  });
});

/** 最小的 NES 卡带：16K PRG，复位向量 $C000，代码向 $2007 写一个字节后死循环 */
function makeNes() {
  const prg = Buffer.alloc(0x4000, 0xea);
  Buffer.from([0xa9, 0x01, 0x8d, 0x07, 0x20, 0x4c, 0x00, 0xc0]).copy(prg, 0);
  for (const offset of [0x3ffa, 0x3ffc, 0x3ffe]) prg.writeUInt16LE(0xc000, offset);
  return Buffer.concat([Buffer.from([0x4e, 0x45, 0x53, 0x1a, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]), prg]);
}

/** 最小的 SB-DOS 程序：文件头跳到 $8010，那里写 $2007 后返回 */
function makeProgram() {
  const bytes = Buffer.alloc(0x40, 0xea);
  Buffer.from([0x4c, 0x10, 0x80, 0xaa, 0x55, 0, 0, 0]).copy(bytes, 0);
  Buffer.from([0xa9, 0x01, 0x8d, 0x07, 0x20, 0x60]).copy(bytes, 0x10);
  return bytes;
}

/** 假的 BIOS：16 页，固定页 4 至 7 在 $C000，页 4 开头是启动代码，向量在页 7 末尾 */
function makeBios() {
  const rom = Buffer.alloc(16 * 0x1000, 0xff);
  Buffer.from([0xa9, 0x01, 0x8d, 0x07, 0x20, 0x4c, 0x00, 0xc0]).copy(rom, 4 * 0x1000);
  for (const offset of [0x8000 - 6, 0x8000 - 4, 0x8000 - 2]) rom.writeUInt16LE(0xc000, offset);
  return rom;
}

describe('反汇编 open 按目标描述', () => {
  let temp;
  let projectDir;

  beforeEach(() => {
    temp = createTempDir();
    projectDir = path.join(temp.dir, 'project');
  });
  afterEach(() => temp.cleanup());

  /** 写映像并按目标描述新建工程 */
  function open(file, content, params) {
    const image = path.join(temp.dir, file);
    writeFileSync(image, content);
    return runOperation('open', { projectDir, image, ...params });
  }

  it('nes：格式、符号集与图形数据端口取自描述，入口取自向量，清单里用符号集的名字', async () => {
    const opened = await open('game.nes', makeNes(), { target: 'nes' });
    assert.deepEqual([opened.format, opened.symbolSets, opened.profile], ['ines', ['fcnes.inc'], { name: 'nes', input: 'cartridge', graphicsPorts: [0x2007] }]);
    assert.equal(opened.counts.entries, 1);
    await runOperation('trace', { projectDir });
    const view = await runOperation('view', { projectDir, address: '$C000', length: 8 });
    assert.match(view.text, /sta PPUDATA/);
    assert.deepEqual((await runOperation('status', { projectDir })).profile.graphicsPorts, [0x2007]);
  });

  it('sb2000 的 program 输入：SB-DOS 程序，文件头的 JMP 成为入口，符号集取自 sb2000.inc', async () => {
    const opened = await open('hello.exe', makeProgram(), { target: 'sb2000' });
    assert.deepEqual([opened.format, opened.symbolSets, opened.profile.input], ['sbdos', ['sb2000.inc'], 'program']);
    assert.deepEqual((await runOperation('list', { projectDir, category: 'entries' })).items.map((item) => [item.address, item.source]), [[0x8000, 'vector']]);
    assert.equal((await runOperation('trace', { projectDir })).instructions, 4);
    assert.match((await runOperation('view', { projectDir, address: '$8010', length: 6 })).text, /sta HW_PPU_DATA/);
  });

  it('sb2000 的 bios 输入：按分页 ROM 解析，复位向量落在固定的 $C000，只有一个入口', async () => {
    const opened = await open('bios.bin', makeBios(), { target: 'sb2000', input: 'bios' });
    assert.equal(opened.format, 'paged');
    assert.deepEqual(opened.segments.map((segment) => [segment.address, segment.length, segment.bank]), [[0x8000, 0x4000, 0], [0xc000, 0x4000, null], [0x8000, 0x4000, 8], [0x8000, 0x4000, 12]]);
    assert.equal(opened.counts.entries, 1);
    assert.equal((await runOperation('trace', { projectDir })).instructions, 3);
  });

  it('自带的描述：路径相对工程目录；描述里的格式选项成为工程的格式选项', async () => {
    writeTestFile(projectDir, 'mine.json', JSON.stringify({ name: 'mine', symbolSets: [], graphicsPorts: [], inputs: { rom: { format: 'flat', formatOptions: { address: 0x8000 } } } }));
    const opened = await open('mine.bin', Buffer.from([0x60]), { target: 'mine.json', entries: ['$8000'] });
    assert.deepEqual([opened.format, opened.segments[0].address, opened.profile], ['flat', 0x8000, { name: 'mine', input: 'rom', graphicsPorts: [] }]);
  });

  it('input 不存在、没给 target、target 不存在时报错；明确给了地址就不用描述里的格式', async () => {
    await assert.rejects(open('a.nes', makeNes(), { target: 'nes', input: 'tape' }), (error) => error.code === 'PROFILE_INPUT_UNKNOWN');
    await assert.rejects(open('a.nes', makeNes(), { input: 'cartridge' }), (error) => error.code === 'DISASM_PARAM_INVALID');
    await assert.rejects(open('a.nes', makeNes(), { target: 'nothing' }), (error) => error.code === 'PROFILE_NOT_FOUND');
    const flat = await open('b.bin', Buffer.from([0x60]), { target: 'sb2000', address: '$8000', overwrite: true });
    assert.equal(flat.format, 'flat');
  });
});

/**
 * 假的调试客户端：记录请求，运行与状态请求给出固定的结果。
 * @returns {{calls: Array<{method: string, params: unknown}>, request: Function, close: Function, closed: Promise<void>}} 客户端
 */
function createFakeClient() {
  const calls = [];
  return {
    calls,
    closed: new Promise(() => {}),
    close() {},
    async request(method, params) {
      calls.push({ method, params });
      if (method === 'runtime.status') return { power: 'on', mode: 'paused', frame: 0, cycles: 0, speed: 100, mouse: false };
      return {};
    },
  };
}

describe('模拟器 load 按目标描述', () => {
  let temp;

  beforeEach(() => { temp = createTempDir(); });
  afterEach(() => {
    closeSession();
    temp.cleanup();
  });

  it('nes 的 cartridge：载入卡带并通电；family-basic 先接入 Family BASIC 键盘', async () => {
    const file = writeTestFile(temp.dir, 'game.nes', 'x');
    const client = createFakeClient();
    openSession({ client });
    const loaded = await runEmulatorOperation('load', { target: 'nes', file });
    assert.deepEqual([loaded.target, loaded.input, loaded.power], ['nes', 'cartridge', 'on']);
    assert.deepEqual(client.calls.slice(0, 1), [{ method: 'cartridges.load', params: { path: file, powerOn: true, resume: false } }]);
    client.calls.length = 0;
    await runEmulatorOperation('load', { target: 'nes', input: 'family-basic', file });
    assert.deepEqual(client.calls.slice(0, 2).map((call) => [call.method, call.params.connected ?? call.params.path]), [['input.setFamilyKeyboard', true], ['cartridges.load', file]]);
  });

  it('sb2000 的 program：添加固件、开机、放入磁盘、跑 900 帧、回车两次、输入程序名、回车', async () => {
    const firmware = writeTestFile(temp.dir, 'bios.bin', 'x');
    const diskImage = writeTestFile(temp.dir, 'disk.img', 'x');
    const client = createFakeClient();
    openSession({ client });
    await runEmulatorOperation('load', { target: 'sb2000', firmware, diskImage, program: 'Hello' });
    assert.deepEqual(client.calls.slice(0, 4).map((call) => call.method), ['machines.setRom', 'machines.boot', 'media.floppyInsert', 'runtime.run']);
    assert.deepEqual(client.calls[3].params, { frames: 900, wait: true });
    const downs = client.calls.filter((call) => call.method === 'input.key' && call.params.down).map((call) => call.params.code);
    assert.deepEqual(downs, ['Enter', 'Enter', 'KeyH', 'KeyE', 'KeyL', 'KeyL', 'KeyO', 'Enter']);
    assert.ok(client.calls.filter((call) => call.method === 'runtime.run').slice(1).every((call) => call.params.frames === 8));
  });

  it('缺少步骤要用的参数、给了 input 没给 target、同时给 target 与 machine 时报错', async () => {
    const client = createFakeClient();
    openSession({ client });
    await assert.rejects(runEmulatorOperation('load', { target: 'nes' }), (error) => error.code === 'EMULATOR_PARAM_INVALID' && error.message.includes('file'));
    await assert.rejects(runEmulatorOperation('load', { target: 'sb2000' }), (error) => error.code === 'EMULATOR_PARAM_INVALID' && error.message.includes('program'));
    await assert.rejects(runEmulatorOperation('load', { target: 'nes', machine: 'sb2000' }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(runEmulatorOperation('load', { input: 'bios', file: 'a.nes' }), (error) => error.code === 'EMULATOR_PARAM_INVALID');
    await assert.rejects(runEmulatorOperation('load', { target: 'sb2000', input: 'tape' }), (error) => error.code === 'PROFILE_INPUT_UNKNOWN');
  });
});
