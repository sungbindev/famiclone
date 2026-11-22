// ------------------------------------------------------------------------
// 名称：sb2000-header.test.mjs
// 说明：测试 SB2000 的 C 符号头文件：入库的文件与 sb2000.inc 的生成结果一致，生成规则正确。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it } from 'node:test';
import { generateSymbolsHeader } from '../gen/gen-sb2000-header.mjs';

const TARGET_DIR = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..', 'targets', 'sb2000');

describe('sb2000-symbols.h', () => {
  it('入库的头文件与 sb2000.inc 的生成结果一致', () => {
    const inc = readFileSync(path.join(TARGET_DIR, 'asminc', 'sb2000.inc'), 'utf8');
    const committed = readFileSync(path.join(TARGET_DIR, 'include', 'sb2000-symbols.h'), 'utf8').replaceAll('\r\n', '\n');
    assert.equal(committed, generateSymbolsHeader(inc).replaceAll('\r\n', '\n'), '改了 sb2000.inc 后要运行 node vscode/tools/gen/gen-sb2000-header.mjs');
  });

  it('符号转成 SB_ 前缀的十六进制宏，零页符号两位、其余四位，行尾注释变成宏上方的文档注释', () => {
    const header = generateSymbolsHeader('; ---- 节 ----\nA_B = $0C ; 说明\nC_D = $6300\n');
    assert.match(header, /\/\*\* 说明 \*\/\n#define SB_A_B 0x0C\n/);
    assert.match(header, /#define SB_C_D 0x6300/);
    assert.match(header, /\/\* 节 \*\//);
  });

  it('注释里的 */ 会被拆开，避免提前结束 C 注释', () => {
    assert.match(generateSymbolsHeader('X = $01 ; a */ b\n'), /\/\*\* a \* \/ b \*\/\n#define SB_X 0x01/);
  });
});
