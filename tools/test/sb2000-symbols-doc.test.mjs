// ------------------------------------------------------------------------
// 名称：sb2000-symbols-doc.test.mjs
// 说明：测试 SB2000 符号参考文档：入库的文件与 sb2000.inc 的生成结果一致，生成规则正确，符号个数与 sb2000.inc 相符。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-18
// 备注：参考文档 targets/sb2000/symbols-reference.md 由 tools/gen-sb2000-symbols-doc.mjs 生成，改了 sb2000.inc 后要重新生成。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it } from 'node:test';
import { renderSymbolsReference, SYMBOLS_DOC_FILE } from '../gen-sb2000-symbols-doc.mjs';

const INC_FILE = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..', 'vscode', 'targets', 'sb2000', 'asminc', 'sb2000.inc');

describe('symbols-reference.md', () => {
  it('入库的文档与 sb2000.inc 的生成结果一致', () => {
    const stored = readFileSync(SYMBOLS_DOC_FILE, 'utf8').replaceAll('\r\n', '\n');
    assert.equal(stored, renderSymbolsReference(readFileSync(INC_FILE, 'utf8')), '改了 sb2000.inc 后要运行 node tools/gen-sb2000-symbols-doc.mjs');
  });

  it('每个符号一行：汇编名、SB_ 前缀的 C 名、零页两位其余四位的值、行尾注释；竖线被转义', () => {
    const text = renderSymbolsReference('; ---- 节一 ----\nA_B = $0c ; 说明 | 竖线\nC_D = $6300 ; 地址\n; ---- 空节 ----\n; ---- 节二 ----\nE_F = $FFF0 ; 入口\n');
    assert.match(text, /\| `A_B` \| `SB_A_B` \| `\$0C` \| 说明 \\\| 竖线 \|/);
    assert.match(text, /\| `C_D` \| `SB_C_D` \| `\$6300` \| 地址 \|/);
    assert.match(text, /共 3 个符号，2 节/);
    assert.doesNotMatch(text, /空节/);
  });

  it('文档里的符号个数等于 sb2000.inc 里的符号行数', () => {
    const inc = readFileSync(INC_FILE, 'utf8');
    const symbolLines = inc.split(/\r?\n/).filter((line) => /^[A-Z][A-Z0-9_]*\s*=\s*\$[0-9A-Fa-f]+/.test(line)).length;
    const rows = renderSymbolsReference(inc).split('\n').filter((line) => /^\| `[A-Z]/.test(line)).length;
    assert.equal(rows, symbolLines);
  });
});
