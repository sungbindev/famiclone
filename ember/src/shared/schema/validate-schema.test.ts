/*
 * ------------------------------------------------------------------------
 * 名称：validate-schema.test.ts
 * 说明：Schema 校验的测试：类型、必填、未知字段、长度、范围、枚举、可空、数组与跨模块引用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';
import type { ModuleSchema } from './schema-types';
import { validateSchema } from './validate-schema';

const SHARED: ModuleSchema = {
  module: 'shared-types',
  types: { Tag: { type: 'object', properties: { name: { type: 'string', minLength: 1 } }, required: ['name'] } },
  requests: {},
  events: {}
};

const DEMO: ModuleSchema = {
  module: 'demo',
  types: {},
  requests: {
    save: {
      params: {
        type: 'object',
        properties: {
          title: { type: 'string', maxLength: 3 },
          count: { type: 'integer', minimum: 0, maximum: 9 },
          ratio: { type: 'number' },
          kind: { type: 'string', enum: ['a', 'b'] },
          note: { type: ['string', 'null'] },
          tags: { type: 'array', items: { $ref: 'shared-types#/types/Tag' }, maxItems: 2 }
        },
        required: ['title']
      },
      result: { type: 'object' }
    }
  },
  events: {}
};

const SCHEMAS = [SHARED, DEMO];

function check(value: unknown) {
  return validateSchema(DEMO.requests.save!.params, value, DEMO, SCHEMAS);
}

describe('validateSchema', () => {
  it('接受合格的数据', () => {
    expect(check({ title: 'abc', count: 3, ratio: 0.5, kind: 'a', note: null, tags: [{ name: 'x' }] })).toEqual({ valid: true, errors: [] });
  });

  it('报告缺少的字段与不认识的字段', () => {
    expect(check({ other: 1 }).errors).toEqual(['params.title 缺少', 'params.other 是不认识的字段']);
  });

  it('检查类型、长度、范围与枚举', () => {
    const result = check({ title: 'abcd', count: 1.5, ratio: Number.NaN, kind: 'c' });
    expect(result.valid).toBe(false);
    expect(result.errors).toEqual(['params.title 最多 3 个字符', 'params.count 应为整数', 'params.ratio 应为数字', 'params.kind 只能是 a、b']);
    expect(check({ title: 'a', count: 10 }).errors).toEqual(['params.count 不能大于 9']);
  });

  it('只有声明可空的字段接受 null', () => {
    expect(check({ title: null }).errors).toEqual(['params.title 不能为空']);
    expect(check({ title: 'a', note: null }).valid).toBe(true);
  });

  it('按引用校验数组项并报告位置', () => {
    expect(check({ title: 'a', tags: [{ name: '' }] }).errors).toEqual(['params.tags[0].name 至少 1 个字符']);
    expect(check({ title: 'a', tags: [{ name: 'a' }, { name: 'b' }, { name: 'c' }] }).errors).toEqual(['params.tags 最多 2 项']);
    expect(check({ title: 'a', tags: {} }).errors).toEqual(['params.tags 应为数组']);
  });

  it('拒绝不是对象的参数', () => {
    expect(check([]).errors).toEqual(['params 应为对象']);
    expect(check('x').errors).toEqual(['params 应为对象']);
  });

  it('引用不存在时抛出异常', () => {
    expect(() => validateSchema({ $ref: 'missing#/types/X' }, {}, DEMO, SCHEMAS)).toThrow('Schema 引用不存在');
  });

  it('没有 type 的节点接受任意 JSON', () => {
    const node = { type: 'object', properties: { schema: { description: '任意' } }, required: ['schema'] } as const;
    for (const value of [null, 1, 'x', [1], { a: { b: true } }]) expect(validateSchema(node, { schema: value }, DEMO, SCHEMAS).valid).toBe(true);
  });
});
