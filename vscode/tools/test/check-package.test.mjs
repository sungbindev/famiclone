// ------------------------------------------------------------------------
// 名称：check-package.test.mjs
// 说明：测试 .vsix 内容核对脚本：完整的包通过；缺少工具链文件、校验值不一致、清单之外的工具链文件、
//       不该出现的文件、缺少使用文档或运行库时报出对应的问题。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用 store 方式（不压缩）在临时目录里写一个最小的 zip 当作 .vsix，不依赖真实打包。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { test } from 'node:test';
import { crc32 } from 'node:zlib';
import { checkPackage } from '../check/check-package.mjs';

/**
 * 按 store 方式写一个 zip 文件。
 * @param {string} file 输出路径
 * @param {Record<string, string | Buffer>} entries 文件名到内容
 */
function writeZip(file, entries) {
  const locals = [];
  const centrals = [];
  let offset = 0;
  for (const [name, content] of Object.entries(entries)) {
    const data = Buffer.from(content);
    const nameBuffer = Buffer.from(name, 'utf8');
    const crc = crc32(data);
    const local = Buffer.alloc(30);
    local.writeUInt32LE(0x04034b50, 0);
    local.writeUInt16LE(20, 4);
    local.writeUInt32LE(crc, 14);
    local.writeUInt32LE(data.length, 18);
    local.writeUInt32LE(data.length, 22);
    local.writeUInt16LE(nameBuffer.length, 26);
    const central = Buffer.alloc(46);
    central.writeUInt32LE(0x02014b50, 0);
    central.writeUInt16LE(20, 4);
    central.writeUInt16LE(20, 6);
    central.writeUInt32LE(crc, 16);
    central.writeUInt32LE(data.length, 20);
    central.writeUInt32LE(data.length, 24);
    central.writeUInt16LE(nameBuffer.length, 28);
    central.writeUInt32LE(offset, 42);
    locals.push(local, nameBuffer, data);
    centrals.push(central, nameBuffer);
    offset += local.length + nameBuffer.length + data.length;
  }
  const centralSize = centrals.reduce((sum, part) => sum + part.length, 0);
  const end = Buffer.alloc(22);
  end.writeUInt32LE(0x06054b50, 0);
  end.writeUInt16LE(Object.keys(entries).length, 8);
  end.writeUInt16LE(Object.keys(entries).length, 10);
  end.writeUInt32LE(centralSize, 12);
  end.writeUInt32LE(offset, 16);
  writeFileSync(file, Buffer.concat([...locals, ...centrals, end]));
}

/** 一个完整的最小包的文件内容 */
function completeEntries() {
  const toolchainFiles = { 'bin/cc65.exe': 'cc65', 'bin/ca65.exe': 'ca65', 'bin/ld65.exe': 'ld65', 'bin/ar65.exe': 'ar65', 'include/nes.h': 'header' };
  const sha = (text) => createHash('sha256').update(text).digest('hex');
  const entries = {
    'extension/package.json': JSON.stringify({ main: './out/bootstrap/extension.js' }),
    'extension/out/bootstrap/extension.js': 'module.exports = {};',
    'extension/debug/openrpc.json': '{}',
    'extension/LICENSE': 'Apache-2.0',
    'extension/README.md': '说明',
    'extension/CHANGELOG.md': '更新记录',
    'extension/toolchain/manifest.json': JSON.stringify({ files: Object.fromEntries(Object.entries(toolchainFiles).map(([name, text]) => [name, sha(text)])) }),
    'extension/templates/_common/gitignore': 'build/',
    'extension/docs/topics.json': JSON.stringify({ topics: [{ topic: 'build', file: '01-构建.md' }, { topic: 'api', file: 'targets/sb2000/api-reference.md', root: 'extension' }] }),
    'extension/docs/01-构建.md': '构建',
    'extension/targets/sb2000/api-reference.md': '接口'
  };
  for (const [name, text] of Object.entries(toolchainFiles)) entries[`extension/toolchain/${name}`] = text;
  for (const target of ['nes', 'sb2000', 'sb97']) {
    entries[`extension/targets/${target}/target.json`] = '{}';
    entries[`extension/targets/${target}/lib-build.json`] = JSON.stringify({ libraries: { [`lib/${target}.lib`]: [] }, objects: { 'lib/crt0.o': 'src/crt0.s' } });
    entries[`extension/targets/${target}/lib/${target}.lib`] = 'lib';
    entries[`extension/targets/${target}/lib/crt0.o`] = 'object';
  }
  for (const template of ['nes-c', 'nes-asm', 'sb2000-c', 'sb2000-asm', 'sb97-asm']) {
    entries[`extension/templates/${template}/template.json`] = '{}';
    entries[`extension/templates/${template}/README.md`] = '模板';
    entries[`extension/templates/${template}/src/hello.${template.endsWith('asm') ? 's' : 'c'}`] = '程序';
    if (!template.startsWith('nes')) entries[`extension/templates/${template}/disk/README.TXT`] = '使用说明';
  }
  return entries;
}

/**
 * 在临时目录里写一个 .vsix 并核对。
 * @param {(entries: Record<string, string | Buffer>) => void} change 对完整内容做的修改
 * @returns {{errors: string[], fileCount: number}} 核对结果
 */
function check(change) {
  const directory = mkdtempSync(path.join(tmpdir(), 'check-package-'));
  try {
    const entries = completeEntries();
    change(entries);
    const file = path.join(directory, 'fcdev-test.vsix');
    writeZip(file, entries);
    return checkPackage(file);
  } finally {
    rmSync(directory, { recursive: true, force: true });
  }
}

test('完整的包通过核对', () => {
  const { errors, fileCount } = check(() => {});
  assert.deepEqual(errors, []);
  assert.ok(fileCount > 20);
});

test('缺少工具链文件、校验值不一致与清单之外的工具链文件都会报出', () => {
  const missing = check((entries) => {
    delete entries['extension/toolchain/include/nes.h'];
  });
  assert.ok(missing.errors.some((error) => error.includes('toolchain/include/nes.h')));

  const changed = check((entries) => {
    entries['extension/toolchain/bin/cc65.exe'] = '被改过';
  });
  assert.ok(changed.errors.some((error) => error.includes('toolchain/bin/cc65.exe') && error.includes('SHA-256')));

  const extra = check((entries) => {
    entries['extension/toolchain/bin/extra.exe'] = '多出来的';
  });
  assert.ok(extra.errors.some((error) => error.includes('toolchain/bin/extra.exe') && error.includes('清单')));
});

test('测试、源码、密钥、Ember 与 BIOS 文件不该出现在包里', () => {
  const { errors } = check((entries) => {
    entries['extension/test/a.test.ts'] = '';
    entries['extension/src/a.ts'] = '';
    entries['extension/keys/signing.pem'] = '';
    entries['extension/bin/Ember.exe'] = '';
    entries['extension/roms/sb2000_bios.bin'] = '';
    entries['extension/roms/game.nes'] = '';
  });
  for (const name of ['test/a.test.ts', 'src/a.ts', 'keys/signing.pem', 'bin/Ember.exe', 'roms/sb2000_bios.bin', 'roms/game.nes']) {
    assert.ok(
      errors.some((error) => error.includes(`不应包含 ${name}`)),
      name
    );
  }
});

test('缺少使用文档、运行库、模板与入口文件时报出对应的问题', () => {
  const { errors } = check((entries) => {
    delete entries['extension/docs/01-构建.md'];
    delete entries['extension/targets/sb2000/lib/sb2000.lib'];
    delete entries['extension/templates/nes-c/src/hello.c'];
    delete entries['extension/out/bootstrap/extension.js'];
    delete entries['extension/targets/nes/target.json'];
  });
  assert.ok(errors.some((error) => error.includes('docs/01-构建.md')));
  assert.ok(errors.some((error) => error.includes('targets/sb2000/lib/sb2000.lib')));
  assert.ok(errors.some((error) => error.includes('templates/nes-c/src/hello')));
  assert.ok(errors.some((error) => error.includes('out/bootstrap/extension.js')));
  assert.ok(errors.some((error) => error.includes('缺少目标 nes')));
});

test('带磁盘能力的模板缺少 disk/README.TXT 时报出', () => {
  const { errors } = check((entries) => {
    delete entries['extension/templates/sb97-asm/disk/README.TXT'];
  });
  assert.ok(errors.some((error) => error.includes('templates/sb97-asm/disk/README.TXT')));
});
