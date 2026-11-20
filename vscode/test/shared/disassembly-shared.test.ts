// ------------------------------------------------------------------------
// 名称：disassembly-shared.test.ts
// 说明：测试反汇编工具的纯函数：输入结构检查、结果文字的整理与截断、字节比较。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：无
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { firstDifference } from '../../src/shared/disassembly/image-compare';
import { MAX_RESULT_CHARS, formatResult, truncate } from '../../src/shared/disassembly/result-text';
import { validateInput, type JsonSchema } from '../../src/shared/disassembly/schema-validator';

const SCHEMA: JsonSchema = {
  type: 'object',
  required: ['projectDir', 'category'],
  additionalProperties: false,
  properties: {
    projectDir: { type: 'string', minLength: 1 },
    category: { type: 'string', enum: ['routines', 'entries'] },
    limit: { type: 'integer', minimum: 1, maximum: 500 },
    address: { type: ['string', 'integer'] },
    edits: { type: 'array', items: { type: 'object', required: ['op'], properties: { op: { type: 'string' } } } }
  }
};

describe('输入结构检查', () => {
  it('合规的输入没有问题', () => {
    expect(validateInput(SCHEMA, { projectDir: 'p', category: 'routines', limit: 10, address: '$C000', edits: [{ op: 'x', extra: 1 }] })).toEqual([]);
  });

  it('缺少必填、类型不对、枚举与范围越界、空字符串都要指出', () => {
    expect(validateInput(SCHEMA, { category: 'bad' })).toEqual(['缺少参数 projectDir', 'category 必须是 "routines"、"entries" 之一']);
    expect(validateInput(SCHEMA, { projectDir: '', category: 'entries', limit: 0 })).toEqual(['projectDir 不能为空', 'limit 不能小于 1']);
    expect(validateInput(SCHEMA, { projectDir: 'p', category: 'entries', limit: 501.5 })).toEqual(['limit 的类型应为 integer']);
    expect(validateInput(SCHEMA, { projectDir: 'p', category: 'entries', address: true })).toEqual(['address 的类型应为 string 或 integer']);
  });

  it('不认识的参数、数组项里的问题都带上位置', () => {
    expect(validateInput(SCHEMA, { projectDir: 'p', category: 'entries', extra: 1, edits: [{ op: 'a' }, {}] })).toEqual(['不认识的参数 extra', '缺少参数 edits[1].op']);
  });

  it('输入不是对象时报类型错误', () => {
    expect(validateInput(SCHEMA, 'x')).toEqual(['输入 的类型应为 object']);
  });
});

describe('结果文字', () => {
  it('普通字段写成缩进的 JSON，text 与 listing 单独成块原样输出', () => {
    const text = formatResult({ total: 2, text: 'a:\n  rts', listing: 'b:' });
    expect(text).toBe('{\n  "total": 2\n}\n\na:\n  rts\n\nb:');
  });

  it('只有 text 时不输出空的 JSON；数组与基本类型也能输出', () => {
    expect(formatResult({ text: 'x' })).toBe('x');
    expect(formatResult([1, 2])).toBe('[\n  1,\n  2\n]');
    expect(formatResult(null)).toBe('null');
  });

  it('超过上限时截断并说明怎么缩小范围，不在代理对中间切开', () => {
    const cut = truncate(`${'a'.repeat(9)}😀😀`, 10);
    expect(cut.startsWith('a'.repeat(9))).toBe(true);
    expect(cut).not.toContain('\ud83d\n');
    expect(cut).toContain('已截断');
    expect(formatResult({ text: 'x'.repeat(MAX_RESULT_CHARS + 5) }).length).toBeLessThan(MAX_RESULT_CHARS + 120);
    expect(truncate('短', 10)).toBe('短');
  });
});

describe('字节比较', () => {
  it('找第一处不同；长度不同时在较短的末尾；相同返回 null', () => {
    expect(firstDifference(Uint8Array.of(1, 2, 3), Uint8Array.of(1, 2, 3))).toBeNull();
    expect(firstDifference(Uint8Array.of(1, 2, 3), Uint8Array.of(1, 9, 3))).toBe(1);
    expect(firstDifference(Uint8Array.of(1, 2, 3), Uint8Array.of(1, 2))).toBe(2);
    expect(firstDifference(Uint8Array.of(1), Uint8Array.of(1, 2))).toBe(1);
  });
});
