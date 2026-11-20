/*
 * ------------------------------------------------------------------------
 * 名称：mcp.test.ts
 * 说明：MCP 服务的测试：工具清单与输入 Schema（含引用类型的展开）、协议处理（initialize、tools/list、tools/call）、PNG 截图的图像块、错误与事件读取、后端的重连与事件记录。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

import { describe, expect, it } from 'vitest';

import { OPENRPC_DOCUMENT } from '../src/entry/document.js';
import { EmberDebugClient } from '../src/logic/debug-client.js';
import { EmberConnectionError, EmberRequestError } from '../src/shared/errors.js';
import { EmberBackend, MAX_RECORDED_EVENTS } from '../src/mcp/backend.js';
import { McpServer, type McpBackend } from '../src/mcp/server.js';
import { buildTools, EVENTS_TOOL_NAME, toolNameOf } from '../src/mcp/tools.js';
import { FakeTransport, HELLO_RESULT } from './support/fake-transport.js';

const TOOLS = buildTools(OPENRPC_DOCUMENT);

/** 假后端：记录调用，按配置返回结果。 */
function createBackend(call: McpBackend['call'] = async () => ({})) {
  const calls: Array<{ method: string; params: object }> = [];
  const backend: McpBackend = {
    call: async (method, params) => {
      calls.push({ method, params });
      return call(method, params);
    },
    drainEvents: async (patterns, max) => ({ events: [{ event: 'x.y', data: { patterns, max } }], remaining: 0 })
  };
  return { backend, calls };
}

describe('buildTools', () => {
  it('每个请求一个工具，连接级请求与 rpc.discover 除外，另有读事件的工具', () => {
    const names = TOOLS.map((tool) => tool.name);
    expect(names).toContain('ember_runtime_run');
    expect(names).toContain('ember_output_screenshot');
    expect(names).toContain(EVENTS_TOOL_NAME);
    for (const excluded of ['debug.hello', 'debug.subscribe', 'debug.unsubscribe', 'rpc.discover']) expect(names).not.toContain(toolNameOf(excluded));
    expect(new Set(names).size).toBe(names.length);
    expect(TOOLS.every((tool) => /^[a-zA-Z0-9_-]{1,64}$/.test(tool.name))).toBe(true);
  });

  it('输入 Schema 由参数生成：必填项、说明、范围', () => {
    const run = TOOLS.find((tool) => tool.name === 'ember_runtime_run');
    expect(run?.method).toBe('runtime.run');
    expect(run?.inputSchema).toMatchObject({ type: 'object', required: ['frames'], additionalProperties: false, properties: { frames: { type: 'integer', minimum: 1 }, wait: { type: 'boolean' } } });
    expect(run?.description).toContain('runtime.noCartridge');
  });

  it('引用的类型（含间接引用）放进 $defs，引用路径改成 #/$defs/，无关的类型不带', () => {
    const document = {
      methods: [
        {
          name: 'x.go',
          summary: '',
          description: '去',
          params: [{ name: 'target', required: true, schema: { $ref: '#/components/schemas/x.A' } }],
          result: { name: 'result', schema: { type: 'object' } }
        }
      ],
      components: {
        schemas: {
          'x.A': { type: 'object', properties: { inner: { $ref: '#/components/schemas/x.B' }, list: { type: 'array', items: { $ref: '#/components/schemas/x.B' } } } },
          'x.B': { type: 'object', properties: { n: { type: 'integer' } } },
          'x.Unused': { type: 'object', properties: {} }
        }
      },
      'x-events': []
    } as unknown as typeof OPENRPC_DOCUMENT;
    const tool = buildTools(document).find((item) => item.name === 'ember_x_go');
    const text = JSON.stringify(tool?.inputSchema);
    expect(text).not.toContain('#/components');
    expect(tool?.inputSchema.properties).toEqual({ target: { $ref: '#/$defs/x.A' } });
    expect(Object.keys((tool?.inputSchema.$defs ?? {}) as object).toSorted()).toEqual(['x.A', 'x.B']);
  });

  it('真实文档里每个工具的引用都能在 $defs 里找到', () => {
    for (const tool of TOOLS) {
      const text = JSON.stringify(tool.inputSchema);
      expect(text).not.toContain('#/components');
      const defs = (tool.inputSchema.$defs ?? {}) as Record<string, unknown>;
      for (const match of text.matchAll(/#\/\$defs\/([A-Za-z0-9.]+)/g)) expect(defs).toHaveProperty([match[1] as string]);
    }
  });
});

describe('McpServer 协议', () => {
  it('initialize 采用客户端要求的协议版本，不认识的版本用最新的，并声明工具能力', async () => {
    const server = new McpServer(TOOLS, createBackend().backend, { name: 'ember-debug-mcp', version: '1.2.3' });
    const known = await server.handle({ jsonrpc: '2.0', id: 1, method: 'initialize', params: { protocolVersion: '2025-03-26' } });
    expect(known).toMatchObject({ id: 1, result: { protocolVersion: '2025-03-26', capabilities: { tools: {} }, serverInfo: { name: 'ember-debug-mcp', version: '1.2.3' } } });
    const unknown = await server.handle({ jsonrpc: '2.0', id: 2, method: 'initialize', params: { protocolVersion: '1999-01-01' } });
    expect((unknown as { result: { protocolVersion: string } }).result.protocolVersion).toBe('2025-11-25');
  });

  it('通知没有回复；ping 返回空对象；未知方法返回 -32601', async () => {
    const server = new McpServer(TOOLS, createBackend().backend, { name: 'n', version: 'v' });
    expect(await server.handle({ jsonrpc: '2.0', method: 'notifications/initialized' })).toBeUndefined();
    expect(await server.handle({ jsonrpc: '2.0', id: 3, method: 'ping' })).toEqual({ jsonrpc: '2.0', id: 3, result: {} });
    expect(await server.handle({ jsonrpc: '2.0', id: 4, method: 'resources/list' })).toMatchObject({ error: { code: -32601 } });
  });

  it('tools/list 返回全部工具', async () => {
    const server = new McpServer(TOOLS, createBackend().backend, { name: 'n', version: 'v' });
    const reply = (await server.handle({ jsonrpc: '2.0', id: 5, method: 'tools/list' })) as { result: { tools: Array<{ name: string }> } };
    expect(reply.result.tools).toHaveLength(TOOLS.length);
  });
});

describe('McpServer 工具调用', () => {
  it('把参数转给后端，结果放进文本内容块', async () => {
    const { backend, calls } = createBackend(async () => ({ state: { power: 'on' }, frames: 3 }));
    const server = new McpServer(TOOLS, backend, { name: 'n', version: 'v' });
    const reply = (await server.handle({ jsonrpc: '2.0', id: 1, method: 'tools/call', params: { name: 'ember_runtime_run', arguments: { frames: 3, wait: true } } })) as { result: { content: Array<{ type: string; text: string }> } };
    expect(calls).toEqual([{ method: 'runtime.run', params: { frames: 3, wait: true } }]);
    expect(JSON.parse(reply.result.content[0]?.text ?? '')).toEqual({ state: { power: 'on' }, frames: 3 });
  });

  it('没有 arguments 时按空对象；PNG 截图多一个图像块，文本里不带 base64', async () => {
    const { backend, calls } = createBackend(async () => ({ format: 'png', width: 256, height: 240, byteLength: 3, data: new Uint8Array([1, 2, 3]) }));
    const server = new McpServer(TOOLS, backend, { name: 'n', version: 'v' });
    const reply = (await server.handle({ jsonrpc: '2.0', id: 1, method: 'tools/call', params: { name: 'ember_output_screenshot' } })) as { result: { content: Array<Record<string, string>> } };
    expect(calls[0]?.params).toEqual({});
    expect(reply.result.content).toHaveLength(2);
    expect(reply.result.content[0]?.text).toContain('<二进制 3 字节>');
    expect(reply.result.content[1]).toEqual({ type: 'image', data: 'AQID', mimeType: 'image/png' });
  });

  it('RGBA 截图只给文本，不带图像块', async () => {
    const { backend } = createBackend(async () => ({ format: 'rgba', data: new Uint8Array(4) }));
    const server = new McpServer(TOOLS, backend, { name: 'n', version: 'v' });
    const reply = (await server.handle({ jsonrpc: '2.0', id: 1, method: 'tools/call', params: { name: 'ember_output_screenshot', arguments: { format: 'rgba' } } })) as { result: { content: unknown[] } };
    expect(reply.result.content).toHaveLength(1);
  });

  it('请求错误与连接错误变成 isError 的工具结果，说明里有错误码', async () => {
    const failing = new McpServer(TOOLS, createBackend(async () => Promise.reject(new EmberRequestError(-32000, 'runtime.notPowered', '主机没有通电'))).backend, { name: 'n', version: 'v' });
    const reply = (await failing.handle({ jsonrpc: '2.0', id: 1, method: 'tools/call', params: { name: 'ember_runtime_pause' } })) as { result: { isError: boolean; content: Array<{ text: string }> } };
    expect(reply.result.isError).toBe(true);
    expect(reply.result.content[0]?.text).toBe('请求失败 runtime.notPowered：主机没有通电');
    const offline = new McpServer(TOOLS, createBackend(async () => Promise.reject(new EmberConnectionError('endpoint', '找不到端点文件'))).backend, { name: 'n', version: 'v' });
    const second = (await offline.handle({ jsonrpc: '2.0', id: 1, method: 'tools/call', params: { name: 'ember_runtime_pause' } })) as { result: { content: Array<{ text: string }> } };
    expect(second.result.content[0]?.text).toBe('连接 Ember 失败（endpoint）：找不到端点文件');
  });

  it('未知工具与不合格的 arguments 返回 -32602', async () => {
    const server = new McpServer(TOOLS, createBackend().backend, { name: 'n', version: 'v' });
    expect(await server.handle({ jsonrpc: '2.0', id: 1, method: 'tools/call', params: { name: 'ember_nope' } })).toMatchObject({ error: { code: -32602 } });
    expect(await server.handle({ jsonrpc: '2.0', id: 2, method: 'tools/call', params: { name: 'ember_runtime_run', arguments: [1] } })).toMatchObject({ error: { code: -32602 } });
  });

  it('ember_events 读取事件，patterns 与 max 转给后端', async () => {
    const server = new McpServer(TOOLS, createBackend().backend, { name: 'n', version: 'v' });
    const reply = (await server.handle({ jsonrpc: '2.0', id: 1, method: 'tools/call', params: { name: EVENTS_TOOL_NAME, arguments: { patterns: ['runtime.*', 5], max: 7 } } })) as { result: { content: Array<{ text: string }> } };
    expect(JSON.parse(reply.result.content[0]?.text ?? '').events[0].data).toEqual({ patterns: ['runtime.*'], max: 7 });
    const defaults = (await server.handle({ jsonrpc: '2.0', id: 2, method: 'tools/call', params: { name: EVENTS_TOOL_NAME } })) as { result: { content: Array<{ text: string }> } };
    expect(JSON.parse(defaults.result.content[0]?.text ?? '').events[0].data).toEqual({ max: 100 });
  });
});

/** 每次连接创建一个新的假传输层并记录。 */
function createFactory() {
  const transports: FakeTransport[] = [];
  const connect = () =>
    EmberDebugClient.open({
      url: 'ws://x/',
      token: 't',
      transportFactory: async () => {
        const transport = new FakeTransport((request) => {
          if (request.method === 'debug.hello') return { result: HELLO_RESULT };
          if (request.method === 'debug.subscribe') return { result: { events: request.params?.events } };
          return { result: { method: request.method } };
        });
        transports.push(transport);
        return transport;
      }
    });
  return { transports, connect };
}

describe('EmberBackend', () => {
  it('第一次调用时才连接，并复用这条连接', async () => {
    const { transports, connect } = createFactory();
    const backend = new EmberBackend(connect, 1000);
    expect(transports).toHaveLength(0);
    expect(await backend.call('runtime.status', {})).toEqual({ method: 'runtime.status' });
    await backend.call('runtime.pause', {});
    expect(transports).toHaveLength(1);
  });

  it('连接断开后下一次调用自动重连，并重新订阅记录中的事件', async () => {
    const { transports, connect } = createFactory();
    const backend = new EmberBackend(connect, 1000);
    await backend.drainEvents(['runtime.*'], 10);
    transports[0]?.closeFromServer(1001);
    await Promise.resolve();
    await backend.call('runtime.status', {});
    expect(transports).toHaveLength(2);
    expect(transports[1]?.sent.find((request) => request.method === 'debug.subscribe')?.params).toEqual({ events: ['runtime.*'] });
  });

  it('drainEvents 第一次订阅全部（或给定的）事件，之后分批取出并清空', async () => {
    const { transports, connect } = createFactory();
    const backend = new EmberBackend(connect, 1000);
    expect(await backend.drainEvents(undefined, 10)).toEqual({ events: [], remaining: 0 });
    expect(transports[0]?.sent.find((request) => request.method === 'debug.subscribe')?.params).toEqual({ events: ['*'] });
    for (let index = 0; index < 3; index += 1) transports[0]?.notify('runtime.frame', { frame: index });
    const first = await backend.drainEvents(undefined, 2);
    expect(first.events.map((item) => (item.data as { frame: number }).frame)).toEqual([0, 1]);
    expect(first.remaining).toBe(1);
    expect((await backend.drainEvents(undefined, 2)).events).toHaveLength(1);
    expect((await backend.drainEvents(undefined, 2)).events).toHaveLength(0);
  });

  it('记录最多保留 MAX_RECORDED_EVENTS 条，丢最早的', async () => {
    const { transports, connect } = createFactory();
    const backend = new EmberBackend(connect, 1000);
    await backend.drainEvents(undefined, 1);
    for (let index = 0; index < MAX_RECORDED_EVENTS + 5; index += 1) transports[0]?.notify('runtime.frame', { frame: index });
    const drained = await backend.drainEvents(undefined, MAX_RECORDED_EVENTS);
    expect(drained.events).toHaveLength(MAX_RECORDED_EVENTS);
    const oldest = drained.events.at(0)?.data as { frame: number } | undefined;
    expect(oldest?.frame).toBe(5);
  });
});
