// ------------------------------------------------------------------------
// 名称：method-catalog.test.ts
// 说明：接口描述文档读取的测试：用随 SDK 打包的真实文档检查请求的列出、查找、类型简写与可读说明。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-14
// 备注：从调试 SDK 的测试并入扩展，用假传输层，不连真实的 Ember。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';

import { OPENRPC_DOCUMENT } from '../../support/openrpc-document';
import { describeMethod, findMethod, listMethods, typeSummary } from '../../../src/shared/debug-client/method-catalog';

describe('method-catalog', () => {
  it('文档里有运行、卡带、输入、截图、握手等请求与事件', () => {
    for (const name of ['runtime.run', 'cartridges.load', 'input.pad', 'output.screenshot', 'debug.hello', 'rpc.discover']) expect(findMethod(OPENRPC_DOCUMENT, name)).toBeDefined();
    expect(OPENRPC_DOCUMENT['x-events'].map((event) => event.name)).toContain('runtime.runFinished');
  });

  it('listMethods 按名称或简介过滤，不分大小写', () => {
    expect(listMethods(OPENRPC_DOCUMENT)).toBe(OPENRPC_DOCUMENT.methods);
    const names = listMethods(OPENRPC_DOCUMENT, 'RUNTIME.').map((method) => method.name);
    expect(names).toContain('runtime.run');
    expect(names.every((name) => name.startsWith('runtime.'))).toBe(true);
    expect(listMethods(OPENRPC_DOCUMENT, '没有这个关键字xyz')).toEqual([]);
  });

  it('typeSummary 写出引用、枚举、数组、范围与二进制', () => {
    expect(typeSummary({ $ref: '#/components/schemas/runtime.RuntimeState' })).toBe('runtime.RuntimeState');
    expect(typeSummary({ type: 'string', enum: ['png', 'rgba'] })).toBe('"png" | "rgba"');
    expect(typeSummary({ type: 'integer', minimum: 1, maximum: 10 })).toBe('integer（1..10）');
    expect(typeSummary({ type: 'array', items: { type: 'string' } })).toBe('string[]');
    expect(typeSummary({ type: ['string', 'null'] })).toBe('string | null');
    expect(typeSummary({ 'x-binary': true })).toBe('二进制');
    expect(typeSummary({})).toBe('任意 JSON');
  });

  it('screenshot 的 data 在文档里标记为二进制', () => {
    const result = findMethod(OPENRPC_DOCUMENT, 'output.screenshot')?.result.schema;
    expect(result?.properties?.data?.['x-binary']).toBe(true);
  });

  it('describeMethod 写出标志、参数、结果、错误码与示例', () => {
    const run = describeMethod(findMethod(OPENRPC_DOCUMENT, 'runtime.run')!);
    expect(run).toContain('runtime.run（只经调试服务）');
    expect(run).toContain('frames?: integer（1..10000000）');
    expect(run).toContain('wait?: boolean');
    expect(run).toContain('stopped?:');
    expect(run).toContain('runtime.noCartridge、runtime.notPowered');
    expect(run).toContain('示例（命令“fcdev：调用调试请求”里输入）：runtime.run');
    expect(describeMethod(findMethod(OPENRPC_DOCUMENT, 'runtime.pause')!)).toContain('参数：\n  （无）');
  });
});
