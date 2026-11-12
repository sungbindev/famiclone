// ------------------------------------------------------------------------
// 名称：ca65-grammar.test.ts
// 说明：用 TextMate 分词器真实加载 ca65 语法文件，检查各类词的作用域，以及指令与伪指令的覆盖。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：作用域取每个词最内层的一个；空白不参与比较。
// ------------------------------------------------------------------------

import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import * as path from 'node:path';
import { INITIAL, parseRawGrammar, Registry, type IGrammar } from 'vscode-textmate';
import { createOnigScanner, createOnigString, loadWASM } from 'vscode-oniguruma';
import { describe, expect, it } from 'vitest';
import { DIRECTIVES } from '../../src/shared/assembly/directive-reference';
import { INSTRUCTIONS } from '../../src/shared/assembly/instruction-reference';

const ROOT = path.resolve(__dirname, '..', '..');
const GRAMMAR_FILE = path.join(ROOT, 'syntaxes', 'ca65.tmLanguage.json');
const SCOPE_NAME = 'source.ca65';

/** 词和它最内层的作用域。 */
type ScopedToken = readonly [text: string, scope: string];

/** 加载分词器所需的 Oniguruma 库，整个文件只加载一次。 */
async function loadGrammar(): Promise<IGrammar> {
  const wasm = readFileSync(createRequire(__filename).resolve('vscode-oniguruma/release/onig.wasm'));
  await loadWASM(new Uint8Array(wasm).buffer);
  const registry = new Registry({
    onigLib: Promise.resolve({ createOnigScanner, createOnigString }),
    loadGrammar: async (scopeName) =>
      scopeName === SCOPE_NAME ? parseRawGrammar(readFileSync(GRAMMAR_FILE, 'utf8'), GRAMMAR_FILE) : null
  });
  const grammar = await registry.loadGrammar(SCOPE_NAME);
  if (grammar === null) throw new Error('语法文件加载失败');
  return grammar;
}

const grammarPromise = loadGrammar();

/** 给一行分词，返回非空白的词与作用域。 */
async function tokenize(line: string): Promise<ScopedToken[]> {
  const grammar = await grammarPromise;
  return grammar
    .tokenizeLine(line, INITIAL)
    .tokens.map((token): ScopedToken => [line.slice(token.startIndex, token.endIndex).trim(), token.scopes.at(-1) ?? ''])
    .filter(([text]) => text !== '');
}

describe('ca65 语法：行与词的作用域', () => {
  it('指令、立即数、十六进制数与行尾注释', async () => {
    expect(await tokenize('  lda #$10 ; 读取')).toEqual([
      ['lda', 'keyword.other.mnemonic.ca65'],
      ['#', 'keyword.operator.immediate.ca65'],
      ['$10', 'constant.numeric.hex.ca65'],
      ['; 读取', 'comment.line.semicolon.ca65']
    ]);
  });

  it('指令不分大小写', async () => {
    expect((await tokenize('LDA 1'))[0]).toEqual(['LDA', 'keyword.other.mnemonic.ca65']);
  });

  it('分支、跳转与返回指令归为流程控制', async () => {
    for (const mnemonic of ['bne', 'jmp', 'jsr', 'rts', 'rti', 'brk']) {
      expect((await tokenize(`  ${mnemonic} x`))[0]).toEqual([mnemonic, 'keyword.control.flow.ca65']);
    }
  });

  it('标签定义：普通、局部与全局', async () => {
    expect(await tokenize('start:')).toEqual([
      ['start', 'entity.name.function.label.ca65'],
      [':', 'punctuation.separator.label.ca65']
    ]);
    expect((await tokenize('@loop:'))[0]).toEqual(['@loop', 'entity.name.function.label.ca65']);
    expect(await tokenize('main::')).toEqual([
      ['main', 'entity.name.function.label.ca65'],
      ['::', 'punctuation.separator.label.ca65']
    ]);
  });

  it('标签后同一行可以接指令', async () => {
    expect(await tokenize('loop: dex')).toEqual([
      ['loop', 'entity.name.function.label.ca65'],
      [':', 'punctuation.separator.label.ca65'],
      ['dex', 'keyword.other.mnemonic.ca65']
    ]);
  });

  it('匿名标签的引用不会被当成标签定义', async () => {
    expect(await tokenize('  bne :+')).toEqual([
      ['bne', 'keyword.control.flow.ca65'],
      [':+', 'variable.other.anonymous.ca65']
    ]);
    expect((await tokenize(':'))[0]).toEqual([':', 'entity.name.function.label.anonymous.ca65']);
  });

  it('常量定义', async () => {
    expect(await tokenize('PPU_CTRL = $2000')).toEqual([
      ['PPU_CTRL', 'variable.other.constant.ca65'],
      ['=', 'keyword.operator.assignment.ca65'],
      ['$2000', 'constant.numeric.hex.ca65']
    ]);
    expect((await tokenize('X := 1'))[1]).toEqual([':=', 'keyword.operator.assignment.ca65']);
  });

  it('过程、作用域与结构的名字', async () => {
    expect(await tokenize('.proc reset')).toEqual([
      ['.proc', 'keyword.other.directive.ca65'],
      ['reset', 'entity.name.function.ca65']
    ]);
    expect(await tokenize('.struct Sprite')).toEqual([
      ['.struct', 'keyword.other.directive.ca65'],
      ['Sprite', 'entity.name.type.ca65']
    ]);
  });

  it('条件汇编、伪函数与普通汇编指令', async () => {
    expect((await tokenize('.ifdef DEBUG'))[0]).toEqual(['.ifdef', 'keyword.control.conditional.ca65']);
    expect((await tokenize('.elseif 1'))[0]).toEqual(['.elseif', 'keyword.control.conditional.ca65']);
    expect((await tokenize('.if .defined(X)'))[1]).toEqual(['.defined', 'support.function.pseudo.ca65']);
    expect((await tokenize('.byte 1'))[0]).toEqual(['.byte', 'keyword.other.directive.ca65']);
  });

  it('字符串里的分号不是注释', async () => {
    const tokens = await tokenize('.byte "a;b" ; 尾注释');
    expect(tokens.filter(([, scope]) => scope === 'comment.line.semicolon.ca65')).toEqual([['; 尾注释', 'comment.line.semicolon.ca65']]);
    expect(tokens.filter(([, scope]) => scope === 'string.quoted.double.ca65').map(([text]) => text).join('')).toBe('"a;b"');
  });

  it('寄存器：累加器操作数与变址', async () => {
    expect(await tokenize('  asl a')).toEqual([
      ['asl', 'keyword.other.mnemonic.ca65'],
      ['a', 'variable.language.register.ca65']
    ]);
    const indexed = await tokenize('  lda $10,x');
    expect(indexed.at(-1)).toEqual(['x', 'variable.language.register.ca65']);
  });

  it('数字：二进制与十进制，局部标签引用', async () => {
    expect(await tokenize('  ldx #%1010')).toContainEqual(['%1010', 'constant.numeric.binary.ca65']);
    expect(await tokenize('  ldx #42')).toContainEqual(['42', 'constant.numeric.decimal.ca65']);
    expect(await tokenize('  bne @loop')).toContainEqual(['@loop', 'variable.other.local.ca65']);
  });

  it('不是指令的标识符不着色', async () => {
    expect(await tokenize('  jsr land')).toContainEqual(['land', SCOPE_NAME]);
  });
});

describe('ca65 语法：与参考资料的覆盖', () => {
  const MNEMONIC_SCOPES = ['keyword.other.mnemonic.ca65', 'keyword.control.flow.ca65'];

  it('参考资料里的每条 6502 指令都被识别', async () => {
    const missed: string[] = [];
    for (const { mnemonic } of INSTRUCTIONS) {
      const [first] = await tokenize(`  ${mnemonic} $10`);
      if (!MNEMONIC_SCOPES.includes(first?.[1] ?? '')) missed.push(mnemonic);
    }
    expect(missed).toEqual([]);
  });

  it('参考资料里的每条汇编指令与伪函数都被识别', async () => {
    const missed: string[] = [];
    for (const { name } of DIRECTIVES) {
      const [first] = await tokenize(`  ${name}`);
      if (first?.[1] === SCOPE_NAME || first?.[1] === undefined) missed.push(name);
    }
    expect(missed).toEqual([]);
  });

  it('伪函数都归为伪函数', async () => {
    const pseudo = DIRECTIVES.filter((doc) => /^\.\w+\(/.test(doc.syntax)).map((doc) => doc.name);
    expect(pseudo.length).toBeGreaterThan(0);
    const wrong: string[] = [];
    for (const name of pseudo) {
      const [first] = await tokenize(`  ${name}(1)`);
      if (first?.[1] !== 'support.function.pseudo.ca65') wrong.push(name);
    }
    expect(wrong).toEqual([]);
  });
});
