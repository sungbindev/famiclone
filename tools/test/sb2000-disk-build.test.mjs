// ------------------------------------------------------------------------
// 名称：sb2000-disk-build.test.mjs
// 说明：测试 SB2000 汇编开发盘的生成：受管文件与说明文件放进目录，目录里的全部文件进镜像。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：用临时目录构造来源文件，不依赖三个应用的真实构建产物。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildDisk, renderReadme, utf8ToGb2312 } from '../../apps/sb2000/disk/build-disk.mjs';
import { listFat12, readFat12File } from '../../vscode/out/shared/packaging/fat12-image.js';

describe('SB2000 汇编开发盘', () => {
  let root;
  let managedFiles;

  beforeEach(() => {
    root = mkdtempSync(path.join(tmpdir(), 'famiclone-disk-'));
    mkdirSync(path.join(root, 'src'));
    writeFileSync(path.join(root, 'src', 'EDIT.EXE'), Buffer.from([1, 2, 3]));
    writeFileSync(path.join(root, 'src', 'hello.asm'), '; 你好\nlda #1\n', 'utf8');
    managedFiles = [
      { name: 'EDIT.EXE', source: path.join(root, 'src', 'EDIT.EXE') },
      { name: 'HELLO.ASM', source: path.join(root, 'src', 'hello.asm'), utf8Text: true },
    ];
  });

  afterEach(() => {
    rmSync(root, { recursive: true, force: true });
  });

  /** 在临时目录里生成镜像 */
  function build(extra = {}) {
    return buildDisk({
      contentDir: path.join(root, 'content'),
      outputPath: path.join(root, 'out.img'),
      managedFiles,
      date: new Date(2026, 10, 12),
      ...extra,
    });
  }

  it('受管文件、说明文件和目录里已有的文件都进镜像', () => {
    mkdirSync(path.join(root, 'content'));
    writeFileSync(path.join(root, 'content', 'MARIO.NES'), Buffer.from([9, 9]));
    const { fileNames } = build();
    const image = readFileSync(path.join(root, 'out.img'));
    assert.deepEqual(fileNames.sort(), ['EDIT.EXE', 'HELLO.ASM', 'MARIO.NES', 'README.TXT']);
    assert.deepEqual(listFat12(image).map((entry) => entry.name).sort(), fileNames);
  });

  it('源码转成 GB2312 并统一成 CR LF，EXE 原样复制', () => {
    build();
    const image = readFileSync(path.join(root, 'out.img'));
    const read = (name) => readFat12File(image, listFat12(image).find((entry) => entry.name === name));
    assert.deepEqual(read('EDIT.EXE'), Buffer.from([1, 2, 3]));
    assert.deepEqual(read('HELLO.ASM'), utf8ToGb2312('; 你好\nlda #1\n'));
    assert.ok(read('HELLO.ASM').includes(Buffer.from('\r\n')));
  });

  it('说明文件按模板生成，文件列表含用户自己放进目录的文件', () => {
    mkdirSync(path.join(root, 'content'));
    writeFileSync(path.join(root, 'content', 'MARIO.NES'), Buffer.from([9]));
    build({ readmeTemplate: '日期 {{生成日期}}\n{{文件列表}}\n' });
    const bytes = readFileSync(path.join(root, 'content', 'README.TXT'));
    // 说明文件是 GB2312 字节，用 GBK 解码后与预期比较
    assert.equal(
      new TextDecoder('gbk').decode(bytes),
      '日期 2026-11-12\r\n  EDIT.EXE    文本编辑器\r\n  HELLO.ASM   示例：输出“你好，小霸王！”\r\n  MARIO.NES\r\n',
    );
  });

  it('来源文件不存在时报错并提示先构建', () => {
    managedFiles.push({ name: 'ASM.EXE', source: path.join(root, 'src', 'ASM.EXE') });
    assert.throws(() => build(), /npm run build/);
  });

  it('文本里有 GB2312 没有的字符时报错', () => {
    assert.throws(() => utf8ToGb2312('😀'), /GB2312/);
  });

  it('renderReadme 的文件列表按名字对齐并附说明', () => {
    const text = renderReadme('{{文件列表}}', ['ASM.EXE', 'X.NES'], new Date(2026, 10, 12));
    assert.equal(text, '  ASM.EXE     6502 汇编器\n  X.NES');
  });
});
