// ------------------------------------------------------------------------
// 名称：line-tokens.test.ts
// 说明：测试行词法辅助：注释与字符串的识别、光标处的词与角色、补全上下文。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { completionContextAt, findTokenAt, layoutLine, splitComment } from '../../src/shared/assembly/line-tokens';

describe('注释与字符串', () => {
  it('字符串里的分号不是注释起点', () => {
    expect(layoutLine('  .byte "a;b" ; 注释').commentStart).toBe(14);
  });

  it('splitComment 分出代码与注释正文', () => {
    expect(splitComment('lda #1 ;  加载 ')).toEqual({ code: 'lda #1 ', comment: '加载' });
    expect(splitComment('rts')).toEqual({ code: 'rts', comment: '' });
  });
});

describe('光标处的词', () => {
  it('缩进后的第一个词是指令位置', () => {
    expect(findTokenAt('        lda #$10', 9)).toMatchObject({ text: 'lda', start: 8, end: 11, role: 'instruction' });
  });

  it('标签之后的第一个词也是指令位置，标签本身是标签', () => {
    const line = 'start:  lda value';
    expect(findTokenAt(line, 2)).toMatchObject({ text: 'start', role: 'label' });
    expect(findTokenAt(line, 9)).toMatchObject({ text: 'lda', role: 'instruction' });
    expect(findTokenAt(line, 14)).toMatchObject({ text: 'value', role: 'operand' });
  });

  it('行首的名字后面跟等号是常量定义，不是指令', () => {
    expect(findTokenAt('SYSCALL = $FFF0', 3)).toMatchObject({ text: 'SYSCALL', role: 'operand' });
  });

  it('汇编指令带前导点，局部标签带 @', () => {
    expect(findTokenAt('        .segment "CODE"', 11)).toMatchObject({ text: '.segment' });
    expect(findTokenAt('        bne @loop', 14)).toMatchObject({ text: '@loop', role: 'operand' });
  });

  it('光标在注释或字符串里没有词', () => {
    expect(findTokenAt('lda #1 ; loop here', 12)).toBeUndefined();
    expect(findTokenAt('.byte "hello"', 9)).toBeUndefined();
  });

  it('标识符中间的点不算汇编指令的前导点', () => {
    expect(findTokenAt('        lda a.b', 12)?.text).toBe('a');
    expect(findTokenAt('        lda a.b', 14)).toBeUndefined();
  });
});

describe('补全上下文', () => {
  it('行首与缩进后是指令位置', () => {
    expect(completionContextAt('        ld', 10)).toMatchObject({ prefix: 'ld', prefixStart: 8, atInstructionSlot: true });
    expect(completionContextAt('', 0).atInstructionSlot).toBe(true);
  });

  it('标签之后是指令位置，操作数位置不是', () => {
    expect(completionContextAt('start: l', 8).atInstructionSlot).toBe(true);
    expect(completionContextAt('        lda va', 14)).toMatchObject({ prefix: 'va', atInstructionSlot: false });
  });

  it('前缀包含前导点与 @', () => {
    expect(completionContextAt('        .seg', 12).prefix).toBe('.seg');
    expect(completionContextAt('        bne @lo', 15).prefix).toBe('@lo');
  });

  it('注释与字符串里不补全', () => {
    expect(completionContextAt('lda #1 ; lo', 11).insideCommentOrString).toBe(true);
    expect(completionContextAt('.byte "lo', 9).insideCommentOrString).toBe(true);
  });
});
