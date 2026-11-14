/*
 * ------------------------------------------------------------------------
 * 名称：api-reference.test.ts
 * 说明：协议参考生成的测试：用真实文档检查总览页与模块页的内容、表格转义、类型链接，以及每个请求与事件都有小节。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';

import { OPENRPC_DOCUMENT } from '../src/entry/document.js';
import { generateApiReference } from '../src/logic/api-reference.js';

const FILES = generateApiReference(OPENRPC_DOCUMENT);
const page = (path: string): string => FILES.find((file) => file.path === path)?.content ?? '';

describe('generateApiReference', () => {
  it('有总览页和每个模块一页', () => {
    const modules = [...new Set(OPENRPC_DOCUMENT.methods.map((method) => method.name.split('.')[0]))];
    expect(FILES.map((file) => file.path)).toEqual(['README.md', ...modules.map((module) => `${module}.md`)]);
    expect(page('README.md')).toContain('debug.hello');
  });

  it('每个请求与事件在所属模块页里都有小节', () => {
    for (const method of OPENRPC_DOCUMENT.methods) expect(page(`${method.name.split('.')[0]}.md`)).toContain(`### ${method.name}\n`);
    for (const event of OPENRPC_DOCUMENT['x-events']) expect(page(`${event.name.split('.')[0]}.md`)).toContain(`### ${event.name}\n`);
  });

  it('请求小节有参数表、结果表、错误码与两种示例', () => {
    const runtime = page('runtime.md');
    expect(runtime).toContain('| `frames` | `integer（1..10000000）` | 是 |');
    expect(runtime).toContain('**错误码**：`runtime.noCartridge`、`runtime.notPowered`');
    expect(runtime).toContain('ember-debug call runtime.run frames=<integer>');
    expect(runtime).toContain('await client.api.runtime.run({ frames: … });');
    expect(runtime).toContain('> 只经调试服务开放');
  });

  it('引用的类型链接到所在模块页的类型小节，类型小节存在', () => {
    expect(page('runtime.md')).toContain('[runtime.RuntimeState](runtime.md#runtimestate)');
    expect(page('runtime.md')).toContain('\n### RuntimeState\n');
  });

  it('表格里的竖线被转义，不会撑破表格', () => {
    for (const file of FILES) {
      for (const line of file.content.split('\n').filter((text) => text.startsWith('| `'))) expect(line.replaceAll('\\|', '').match(/\|/g)?.length).toBeGreaterThanOrEqual(3);
    }
    expect(page('output.md')).toContain('`"png" \\| "rgba"`');
  });
});
