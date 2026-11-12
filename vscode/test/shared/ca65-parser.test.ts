// ------------------------------------------------------------------------
// 名称：ca65-parser.test.ts
// 说明：测试 ca65 解析器：常量、标签、过程、宏、定义、包含文件，以及说明与分组的归属。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：样例取自 fcdev 自带的符号定义与运行库的写法。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { parseAssembly } from '../../src/shared/assembly/ca65-parser';

/** 解析后按名字取符号。 */
function symbolOf(text: string, name: string) {
  const symbol = parseAssembly(text).symbols.find((item) => item.name === name);
  if (symbol === undefined) throw new Error(`没有解析出符号 ${name}`);
  return symbol;
}

describe('常量', () => {
  it('等号定义取右边的表达式，行尾注释作为说明', () => {
    const symbol = symbolOf('SYSCALL             = $FFF0             ; 系统调用入口\n', 'SYSCALL');
    expect(symbol).toMatchObject({ kind: 'constant', value: '$FFF0', documentation: '系统调用入口', line: 0, isLocal: false });
    expect(symbol.startColumn).toBe(0);
    expect(symbol.endColumn).toBe(7);
  });

  it(':= 也是常量定义', () => {
    expect(symbolOf('LIMIT := 10\n', 'LIMIT')).toMatchObject({ kind: 'constant', value: '10' });
  });

  it('注释里的等号和字符串里的分号不影响解析', () => {
    const symbol = symbolOf('NAME = "a;b"  ; x = y\n', 'NAME');
    expect(symbol.value).toBe('"a;b"');
    expect(symbol.documentation).toBe('x = y');
  });
});

describe('标签', () => {
  it('单独一行的标签，行尾注释是它的说明', () => {
    expect(symbolOf('start:  ; 入口\n', 'start')).toMatchObject({ kind: 'label', documentation: '入口' });
  });

  it('标签后面还有指令时，行尾注释属于指令，说明取上方的注释块', () => {
    const symbol = symbolOf('; 等待垂直消隐\n_nes_wait_vblank: bit $2002  ; 读状态\n', '_nes_wait_vblank');
    expect(symbol.documentation).toBe('等待垂直消隐');
  });

  it('局部标签带 @ 并标记为局部', () => {
    expect(symbolOf('@loop: dex\n', '@loop')).toMatchObject({ kind: 'label', isLocal: true });
  });

  it('指令行与匿名标签不当作符号', () => {
    const parsed = parseAssembly('        lda #1\n:       bne :-\n        jsr foo\n');
    expect(parsed.symbols).toEqual([]);
  });
});

describe('汇编指令定义的符号', () => {
  it('.proc、.macro、.define、.scope、.struct、.enum', () => {
    const text = '.proc init\n.endproc\n.macro push2 a, b\n.endmacro\n.define COUNT 4\n.scope S\n.endscope\n.struct Pt\n.endstruct\n.enum E\n.endenum\n';
    const parsed = parseAssembly(text);
    expect(parsed.symbols.map((item) => [item.name, item.kind])).toEqual([
      ['init', 'procedure'],
      ['push2', 'macro'],
      ['COUNT', 'define'],
      ['S', 'scope'],
      ['Pt', 'struct'],
      ['E', 'enum']
    ]);
    expect(parsed.symbols[1]?.value).toBe('a, b');
    expect(parsed.symbols[2]?.value).toBe('4');
  });

  it('指令名不区分大小写，.import 与 .export 不产生符号', () => {
    const parsed = parseAssembly('.PROC Main\n.import foo\n.export Main\n');
    expect(parsed.symbols.map((item) => item.name)).toEqual(['Main']);
  });
});

describe('说明与分组', () => {
  it('紧贴在上方的注释块作为说明，多行用换行连接', () => {
    const symbol = symbolOf('; 第一行\n; 第二行\nfoo:\n', 'foo');
    expect(symbol.documentation).toBe('第一行\n第二行');
  });

  it('隔着空行的注释不是说明', () => {
    expect(symbolOf('; 文件说明\n\nfoo:\n', 'foo').documentation).toBe('');
  });

  it('分组标题不是说明，只作为所属分组', () => {
    const text = '; ---- 系统调用 ----\nSYSCALL = $FFF0\n; ---- 零页 ----\nPTR = $4B ; 指针\n';
    const parsed = parseAssembly(text);
    expect(parsed.symbols[0]).toMatchObject({ documentation: '', section: '系统调用' });
    expect(parsed.symbols[1]).toMatchObject({ documentation: '指针', section: '零页' });
  });

  it('分隔线不是说明', () => {
    expect(symbolOf('; ------------\nfoo:\n', 'foo').documentation).toBe('');
  });
});

describe('包含文件', () => {
  it('记录 .include 的文件名与行号，不区分大小写', () => {
    const parsed = parseAssembly('.include "fcnes.inc"\n  .INCLUDE "../inc/a.inc"  ; x\n');
    expect(parsed.includes).toEqual([
      { name: 'fcnes.inc', line: 0 },
      { name: '../inc/a.inc', line: 1 }
    ]);
  });

  it('兼容 CRLF 换行', () => {
    expect(symbolOf('A = 1 ; 一\r\nB = 2\r\n', 'A').documentation).toBe('一');
  });
});
