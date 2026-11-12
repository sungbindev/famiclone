// ------------------------------------------------------------------------
// 名称：assembly-reference.test.ts
// 说明：测试汇编的小工具与参考资料：数字字面量、路径文字、6502 指令表、汇编指令表、悬停文字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { DIRECTIVES, findDirective } from '../../src/shared/assembly/directive-reference';
import { escapeMarkdown, formatDirectiveHover, formatInstructionHover, formatSymbolHover } from '../../src/shared/assembly/hover-text';
import { INSTRUCTIONS, findInstruction } from '../../src/shared/assembly/instruction-reference';
import { describeValue, parseNumberLiteral } from '../../src/shared/assembly/number-literal';
import { basenameOf, dirnameOf, joinPath } from '../../src/shared/assembly/path-text';

describe('数字字面量', () => {
  it('识别 $十六进制、0x、%二进制与十进制', () => {
    expect(parseNumberLiteral('$FFF0')).toBe(0xfff0);
    expect(parseNumberLiteral('0x20')).toBe(32);
    expect(parseNumberLiteral('%1010')).toBe(10);
    expect(parseNumberLiteral(' 42 ')).toBe(42);
  });

  it('表达式与标识符不是字面量', () => {
    expect(parseNumberLiteral('$10+1')).toBeUndefined();
    expect(parseNumberLiteral('FOO')).toBeUndefined();
  });

  it('非十进制字面量附上十进制，十进制与表达式原样显示', () => {
    expect(describeValue('$FFF0')).toBe('$FFF0（十进制 65520）');
    expect(describeValue('42')).toBe('42');
    expect(describeValue('FOO + 1')).toBe('FOO + 1');
  });
});

describe('路径文字', () => {
  it('取目录与文件名，两种分隔符都认', () => {
    expect(dirnameOf('d:\\a\\b\\c.s')).toBe('d:\\a\\b');
    expect(dirnameOf('/a/b/c.s')).toBe('/a/b');
    expect(basenameOf('d:\\a\\c.s')).toBe('c.s');
    expect(dirnameOf('c.s')).toBe('');
  });

  it('拼接并规整 . 与 .. 段，沿用基准目录的分隔符', () => {
    expect(joinPath('d:\\proj\\src', '..\\inc\\a.inc')).toBe('d:\\proj\\inc\\a.inc');
    expect(joinPath('/proj/src', './a/../b.inc')).toBe('/proj/src/b.inc');
  });

  it('盘符改成小写，绝对路径不再拼接基准目录', () => {
    expect(joinPath('D:\\proj', 'x.inc')).toBe('d:\\proj\\x.inc');
    expect(joinPath('/proj', 'C:\\abs\\x.inc')).toBe('c:/abs/x.inc');
  });
});

describe('6502 指令表', () => {
  it('收录 56 条官方指令，助记符不重复', () => {
    expect(INSTRUCTIONS).toHaveLength(56);
    expect(new Set(INSTRUCTIONS.map((doc) => doc.mnemonic)).size).toBe(56);
  });

  it('查询不区分大小写，非官方指令查不到', () => {
    expect(findInstruction('LDA')?.title).toBe('加载累加器');
    expect(findInstruction('xyz')).toBeUndefined();
  });

  it('寻址方式与字节数正确', () => {
    const sta = findInstruction('sta');
    expect(sta?.modes.map((mode) => mode.syntax)).toEqual(['$nn', '$nn,X', '$nnnn', '$nnnn,X', '$nnnn,Y', '($nn,X)', '($nn),Y']);
    expect(findInstruction('jmp')?.modes.map((mode) => mode.bytes)).toEqual([3, 3]);
    expect(findInstruction('bne')?.modes[0]?.bytes).toBe(2);
    expect(findInstruction('rts')?.modes[0]).toMatchObject({ syntax: '', bytes: 1 });
  });
});

describe('汇编指令表', () => {
  it('名称不重复，都以点开头且是小写', () => {
    expect(new Set(DIRECTIVES.map((doc) => doc.name)).size).toBe(DIRECTIVES.length);
    expect(DIRECTIVES.every((doc) => /^\.[a-z]+$/.test(doc.name))).toBe(true);
  });

  it('查询不区分大小写', () => {
    expect(findDirective('.SEGMENT')?.syntax).toContain('.segment');
    expect(findDirective('.nosuch')).toBeUndefined();
  });
});

describe('悬停文字', () => {
  it('Markdown 特殊字符被转义', () => {
    expect(escapeMarkdown('a_b*c')).toBe('a\\_b\\*c');
  });

  it('符号：代码块、种类与值、说明、分组与来源', () => {
    const text = formatSymbolHover({
      name: 'SYSCALL',
      kind: 'constant',
      value: '$FFF0',
      documentation: '系统调用入口',
      section: '系统调用',
      line: 11,
      fileLabel: 'sb2000.inc'
    });
    expect(text).toContain('```ca65\nSYSCALL = $FFF0\n```');
    expect(text).toContain('常量，值 $FFF0（十进制 65520）');
    expect(text).toContain('系统调用入口');
    expect(text).toContain('分组：系统调用');
    expect(text).toContain('定义于 sb2000.inc 第 12 行');
  });

  it('标签与宏的定义写法', () => {
    const base = { documentation: '', section: '', line: 0, fileLabel: 'a.s', value: '' };
    expect(formatSymbolHover({ ...base, name: 'start', kind: 'label' })).toContain('start:');
    expect(formatSymbolHover({ ...base, name: 'push2', kind: 'macro', value: 'a, b' })).toContain('.macro push2 a, b');
  });

  it('指令：名称、标志位与寻址方式表', () => {
    const doc = findInstruction('lda');
    const text = formatInstructionHover(doc ?? INSTRUCTIONS[0]!);
    expect(text).toContain('**LDA**：加载累加器');
    expect(text).toContain('影响的标志位：N Z');
    expect(text).toContain('| 立即数 | `lda #$nn` | 2 |');
  });

  it('汇编指令：写法与说明', () => {
    const text = formatDirectiveHover(findDirective('.segment') ?? DIRECTIVES[0]!);
    expect(text).toContain('**.segment**');
    expect(text).toContain('.segment "段名"');
  });
});
