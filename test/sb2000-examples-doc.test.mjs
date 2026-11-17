// ------------------------------------------------------------------------
// 名称：sb2000-examples-doc.test.mjs
// 说明：测试 SB2000 示例程序源码文档：入库的文件与示例项目的生成结果一致，每个示例项目都有一节，源码完整、换行统一。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-18
// 备注：文档 targets/sb2000/examples-source.md 由 tools/gen-sb2000-examples-doc.mjs 生成，改了示例源码后要重新生成。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { describe, it } from 'node:test';
import { EXAMPLES_DIR, EXAMPLES_DOC_FILE, listExampleNames, renderExamplesSource } from '../tools/gen-sb2000-examples-doc.mjs';

/** 在临时目录里造一个示例项目。 */
function writeExample(root, name, config, files) {
  mkdirSync(path.join(root, name, 'src'), { recursive: true });
  writeFileSync(path.join(root, name, 'fcdev.json'), JSON.stringify(config));
  for (const [file, text] of Object.entries(files)) writeFileSync(path.join(root, name, 'src', file), text);
}

describe('examples-source.md', () => {
  it('入库的文档与示例项目的生成结果一致', () => {
    const stored = readFileSync(EXAMPLES_DOC_FILE, 'utf8').replaceAll('\r\n', '\n');
    assert.equal(stored, renderExamplesSource(), '改了示例源码后要运行 node tools/gen-sb2000-examples-doc.mjs');
  });

  it('每个示例项目都有一节，并包含它的源文件全文', () => {
    const text = renderExamplesSource();
    for (const name of listExampleNames()) {
      assert.match(text, new RegExp(`^## ${name}$`, 'm'), name);
      const sourceDir = path.join(EXAMPLES_DIR, name, 'src');
      const source = readFileSync(path.join(sourceDir, `${name}.${name === 'play' ? 's' : 'c'}`), 'utf8').replace(/\r\n/g, '\n').replace(/\s+$/, '');
      assert.ok(text.includes(source), `${name} 的源码不完整`);
    }
  });

  it('生成的文档里没有回车符，示例按约定的顺序排在前面，没列出的按名字排在最后', () => {
    const temp = mkdtempSync(path.join(tmpdir(), 'sb2000-examples-'));
    try {
      writeExample(temp, 'zeta', { name: 'zeta' }, { 'zeta.c': 'int z;\r\n' });
      writeExample(temp, 'alpha', { name: 'alpha' }, { 'alpha.s': '  nop\r\n' });
      writeExample(temp, 'hello', { name: 'hello' }, { 'hello.c': 'int h;\n' });
      assert.deepEqual(listExampleNames(temp), ['hello', 'alpha', 'zeta']);
      const text = renderExamplesSource(temp);
      assert.equal(text.includes('\r'), false);
      assert.match(text, /```c\nint z;\n```/);
      assert.match(text, /```text\n {2}nop\n```/);
      assert.match(text, /程序名 `ZETA`/);
    } finally {
      rmSync(temp, { recursive: true, force: true });
    }
  });
});
