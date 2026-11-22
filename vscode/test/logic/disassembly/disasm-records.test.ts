// ------------------------------------------------------------------------
// 名称：disasm-records.test.ts
// 说明：测试运行记录与符号文件的解析、由记录生成入口、映像格式的判断，以及工程在磁盘上的读写。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------

import assert from 'node:assert/strict';
import { writeFileSync } from 'node:fs';
import * as path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'vitest';
import type { FcdevError } from '../../../src/shared/errors/fcdev-error';
import { CDL_CODE, CDL_DATA, CDL_JUMP_TARGET, CDL_SUB_ENTRY, parseCdl } from '../../../src/shared/disassembly/formats/cdl';
import { PROJECT_FILE, fromProjectPath, readProject, readProjectImage, readProjectSymbolSets, toProjectPath, writeProject } from '../../../src/logic/disassembly/workspace/project-store';
import { createAddressSpace } from '../../../src/shared/disassembly/analysis/address-space';
import { createLayout } from '../../../src/shared/disassembly/model/layout';
import { createProject } from '../../../src/shared/disassembly/model/project';
import { dataRunsFromFlags, entriesFromAddresses, entriesFromCodeFlags, mappedSize } from '../../../src/shared/disassembly/analysis/record-entries';
import { detectFormat } from '../../../src/shared/disassembly/formats/index';
import { parseCallLog } from '../../../src/shared/disassembly/formats/call-log';
import { parseFlat } from '../../../src/shared/disassembly/formats/flat';
import { parseSymbols } from '../../../src/shared/disassembly/profile/symbol-file';
import { parseTrace } from '../../../src/shared/disassembly/formats/trace-log';
import { createTempDir, type TempDir } from '../../support/build-fixtures';
import { NodeFileSystem } from '../../../src/infra/files/node-file-system';

describe('CDL、轨迹与符号文件', () => {
  it('CDL 只保留代码与数据两位', () => {
    const flags = parseCdl(Buffer.from([0x01, 0x02, 0x83, 0x00]), 4);
    assert.deepEqual([...flags], [1, 2, 3, 0]);
  });

  it('CDLv2 文件头被跳过，文件比程序短时报错', () => {
    const buffer = Buffer.concat([Buffer.from('CDLv2'), Buffer.alloc(4), Buffer.from([1, 0])]);
    assert.deepEqual([...parseCdl(buffer, 2)], [1, 0]);
    assert.throws(() => parseCdl(Buffer.from([1]), 4), (error: FcdevError) => error.code === 'DISASM_CDL_INVALID');
  });

  it('CDL 可以指定保留的位：跳转目标与子程序入口', () => {
    const buffer = Buffer.from([0x09, 0x04, 0x02, 0x80]);
    assert.deepEqual([...parseCdl(buffer, 4)], [1, 0, 2, 0]);
    assert.deepEqual([...parseCdl(buffer, 4, CDL_CODE | CDL_DATA | CDL_JUMP_TARGET | CDL_SUB_ENTRY)], [9, 4, 2, 0]);
  });

  it('调用记录取出被记录的指令地址，并由栈顶换算返回地址', () => {
    const text = [
      'C00B  A:00 X:00 Y:00 P:24 SP:FB CYC:77 | STACK 02 C0 FF | 0300: 11 22',
      'C00B  A:01 X:00 Y:00 P:24 SP:FB CYC:99 | STACK 02 C0 FF',
      'C00B  A:02 X:00 Y:00 P:24 SP:FB CYC:120',
      'C200  A:00 X:00 Y:00 P:24 SP:FD CYC:130 | STACK FF FF',
      '8000  78        SEI                             A:00 X:00 Y:00 P:24 SP:FD CYC:7',
    ].join('\n');
    assert.deepEqual(parseCallLog(text), {
      addresses: [0xc00b, 0xc200],
      returns: [{ target: 0xc00b, returnAddress: 0xc003 }, { target: 0xc200, returnAddress: 0x0000 }],
    });
  });

  it('轨迹取出不重复的指令地址，忽略其他行', () => {
    const text = [
      '8000  78        SEI                             A:00 X:00 Y:00 P:24 SP:FD CYC:7',
      '8001  D8        CLD                             A:00 X:00 Y:00 P:24 SP:FD CYC:9',
      '8000  78        SEI                             A:00 X:00 Y:00 P:24 SP:FD CYC:11',
      'C0DE  EA        NOP                             A:00 X:00 Y:00 P:24 SP:FD CYC:13',
      'C0DF  4C F5 C5  JMP $C5F5                       A:00 X:00 Y:00 P:24 SP:FD CYC:15',
      'R $2002 = 80  PC:C005 CYC:1234',
      'NMI  return:C0AB handler:C100 CYC:500',
      '8123  A:01 X:02 Y:03 P:24 SP:FD CYC:77 | STACK AA BB CC | 0300: 11 22',
      'T 9000 -000000 78 00 00 A=00',
    ].join('\n');
    assert.deepEqual(parseTrace(text), [0x8000, 0x8001, 0xc0de, 0xc0df]);
  });

  it('符号文件支持等号与空格、$ 与 0x 前缀、注释与空行', () => {
    const text = '; 注释\n\nplayer_x = $0010\nscore $0020 ; 分数\nvram 0x2007\n# 另一种注释\n';
    assert.deepEqual(parseSymbols(text), [{ name: 'player_x', address: 0x10 }, { name: 'score', address: 0x20 }, { name: 'vram', address: 0x2007 }]);
  });

  it('符号文件格式不对时指出行号', () => {
    assert.throws(() => parseSymbols('ok = $10\nbad line here\n'), (error: FcdevError) => error.code === 'DISASM_SYMBOL_INVALID' && error.message.includes('第 2 行'));
  });
});

describe('由运行记录生成入口', () => {
  /** 平铺映像 6 字节，加载在 $8000 */
  const flat = parseFlat(Buffer.alloc(6), { address: 0x8000 }).layout;

  /** 两个段：固定区 $C000 占 2 字节，bank 0 的 $8000 占 2 字节，文件里首尾相接 */
  const banked = createLayout([
    { id: 0, fileOffset: 0, length: 2, address: 0xc000 },
    { id: 1, fileOffset: 2, length: 2, address: 0x8000, bank: 0 },
  ], { imageSize: 4 });

  it('地址：映像里的生成入口，不在映像里的原样返回在 skipped 里', () => {
    const { entries, skipped } = entriesFromAddresses(createAddressSpace(flat), [0x8000, 0x8003, 0x1234], { source: 'trace', description: '指令轨迹' });
    assert.deepEqual(entries.map((entry) => [entry.segmentId, entry.address, entry.source, entry.description]), [[0, 0x8000, 'trace', '指令轨迹'], [0, 0x8003, 'trace', '指令轨迹']]);
    assert.deepEqual(skipped.map((item) => item.address), [0x1234]);
  });

  it('地址：换页窗口里无法确定 bank 的地址不生成入口', () => {
    const space = createAddressSpace(createLayout([
      { id: 0, fileOffset: 0, length: 2, address: 0x8000, bank: 0 },
      { id: 1, fileOffset: 2, length: 2, address: 0x8000, bank: 1 },
    ], { imageSize: 4 }));
    const { entries, skipped } = entriesFromAddresses(space, [0x8000], { source: 'trace' });
    assert.deepEqual(entries, []);
    assert.equal(skipped.length, 1);
  });

  /** 字节标志对应的入口地址 */
  const flagEntries = (flags: number[]) => entriesFromCodeFlags(flat, Uint8Array.from(flags), { source: 'cdl', description: '代码数据日志' }).map((entry) => entry.address);

  it('字节标志：每段连续的代码字节的起点是入口', () => {
    assert.deepEqual(flagEntries([0, 0, 0, 1, 1, 1]), [0x8003]);
    assert.deepEqual(flagEntries([1, 1, 0, 1, 0, 1]), [0x8000, 0x8003, 0x8005]);
    assert.deepEqual(flagEntries([2, 3, 2, 0, 0, 0]), [0x8001]);
  });

  it('字节标志：按文件偏移顺序跨段对应，每个段的第一个字节也是起点；标志太短时报错', () => {
    const entries = entriesFromCodeFlags(banked, Uint8Array.of(1, 1, 1, 1), { source: 'cdl' });
    assert.deepEqual(entries.map((entry) => [entry.segmentId, entry.address]), [[0, 0xc000], [1, 0x8000]]);
    assert.equal(mappedSize(banked), 4);
    assert.throws(() => entriesFromCodeFlags(banked, Uint8Array.of(1, 1, 1), { source: 'cdl' }), (error: FcdevError) => error.code === 'DISASM_CDL_INVALID');
  });
});

describe('由字节标志取起点与数据段', () => {
  /** 平铺映像 6 字节，加载在 $8000 */
  const flat = parseFlat(Buffer.alloc(6), { address: 0x8000 }).layout;

  it('起点位：代码段中间被标成跳转目标或子程序入口的字节也是入口', () => {
    const flags = Uint8Array.of(1, 1, 1 | CDL_SUB_ENTRY, 1, 1 | CDL_JUMP_TARGET, 0);
    const run = (startBits: number) => entriesFromCodeFlags(flat, flags, { source: 'cdl', startBits }).map((entry) => entry.address);
    assert.deepEqual(run(0), [0x8000]);
    assert.deepEqual(run(CDL_SUB_ENTRY), [0x8000, 0x8002]);
    assert.deepEqual(run(CDL_SUB_ENTRY | CDL_JUMP_TARGET), [0x8000, 0x8002, 0x8004]);
  });

  it('数据段：只被当数据读取的连续字节，当过代码的不算', () => {
    assert.deepEqual(dataRunsFromFlags(flat, Uint8Array.of(0, 2, 2, 3, 2, 0)), [{ segmentId: 0, address: 0x8001, offset: 1, length: 2 }, { segmentId: 0, address: 0x8004, offset: 4, length: 1 }]);
    assert.deepEqual(dataRunsFromFlags(flat, Uint8Array.of(2, 2, 2, 2, 2, 2)), [{ segmentId: 0, address: 0x8000, offset: 0, length: 6 }]);
    assert.throws(() => dataRunsFromFlags(flat, Uint8Array.of(2)), (error: FcdevError) => error.code === 'DISASM_CDL_INVALID');
  });
});

describe('判断映像格式', () => {
  it('按文件头判断，认不出的是平铺二进制', () => {
    assert.equal(detectFormat(Buffer.from([0x4e, 0x45, 0x53, 0x1a, 1, 1])), 'ines');
    assert.equal(detectFormat(Buffer.from([0x4c, 0x00, 0x80])), 'flat');
    assert.equal(detectFormat(Buffer.alloc(0)), 'flat');
  });
});

/** 平铺映像的工程 */
function makeProject(bytes: number[], imagePath: string) {
  const buffer = Buffer.from(bytes);
  const { layout } = parseFlat(buffer, { address: 0xc000 });
  return { buffer, project: createProject({ imagePath, bytes: buffer, format: 'flat', formatOptions: { address: 0xc000 }, layout, symbolSets: [{ name: 'regs.inc', path: '../regs.inc' }] }) };
}

describe('工程在磁盘上的读写', () => {
  const files = new NodeFileSystem();
  let temp: TempDir;

  beforeEach(() => { temp = createTempDir(); });
  afterEach(() => temp.cleanup());

  it('路径相对工程目录、用 / 分隔，能还原', () => {
    const dir = path.join(temp.dir, 'out', 'proj');
    const file = path.join(temp.dir, 'game', 'a.bin');
    assert.equal(toProjectPath(dir, file), '../../game/a.bin');
    assert.equal(fromProjectPath(dir, '../../game/a.bin'), file);
  });

  it('保存后再读回得到同一个工程；目录里没有工程文件时报错', () => {
    const { project } = makeProject([0x60, 0x60], 'a.bin');
    const dir = path.join(temp.dir, 'proj');
    assert.equal(writeProject(files, dir, project), path.join(dir, PROJECT_FILE));
    assert.deepEqual(readProject(files, dir).layout, project.layout);
    assert.deepEqual(readProject(files, dir).symbolSets, project.symbolSets);
    assert.throws(() => readProject(files, path.join(temp.dir, 'none')), (error: FcdevError) => error.code === 'DISASM_PROJECT_MISSING');
  });

  it('读映像：与工程记录的摘要一致才返回，改动或缺失时报错', () => {
    const { project, buffer } = makeProject([0x60, 0x60], 'image.bin');
    writeFileSync(path.join(temp.dir, 'image.bin'), buffer);
    assert.deepEqual(readProjectImage(files, temp.dir, project), buffer);
    writeFileSync(path.join(temp.dir, 'image.bin'), Buffer.from([0x60, 0x61]));
    assert.throws(() => readProjectImage(files, temp.dir, project), (error: FcdevError) => error.code === 'DISASM_IMAGE_MISMATCH');
    assert.throws(() => readProjectImage(files, path.join(temp.dir, 'elsewhere'), project), (error: FcdevError) => error.code === 'DISASM_IMAGE_MISSING');
  });

  it('读符号集：按工程里的路径读文本，缺失时报错', () => {
    const { project } = makeProject([0x60], 'a.bin');
    const dir = path.join(temp.dir, 'proj');
    writeFileSync(path.join(temp.dir, 'regs.inc'), 'REG = $10\n');
    assert.deepEqual([...readProjectSymbolSets(files, dir, project)], [['regs.inc', 'REG = $10\n']]);
    assert.throws(() => readProjectSymbolSets(files, path.join(temp.dir, 'x', 'y'), project), (error: FcdevError) => error.code === 'DISASM_SYMBOL_SET_MISSING');
  });
});
