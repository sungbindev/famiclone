/*
 * ------------------------------------------------------------------------
 * 名称：tools.ts
 * 说明：把 OpenRPC 文档转成 MCP 的工具清单：每个请求一个工具（名称 ember_<模块>_<操作>），输入 Schema 由请求的参数生成。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：连接级的请求（debug.hello、debug.subscribe、debug.unsubscribe）与 rpc.discover 不做成工具；事件由单独的 ember_events 工具读取。
 * ------------------------------------------------------------------------
 */

import type { OpenRpcDocument, OpenRpcMethod, OpenRpcSchema } from '../logic/method-catalog.js';

/** MCP 工具的定义。 */
export interface McpTool {
  readonly name: string;
  readonly description: string;
  readonly inputSchema: Record<string, unknown>;
  /** 对应的调试服务请求；ember_events 没有。 */
  readonly method?: string;
}

/** 读取事件的工具名。 */
export const EVENTS_TOOL_NAME = 'ember_events';

/** 不做成工具的请求。 */
const EXCLUDED_METHODS = new Set(['debug.hello', 'debug.subscribe', 'debug.unsubscribe', 'rpc.discover']);
const REF_PREFIX = '#/components/schemas/';

/** 请求名对应的工具名。 */
export function toolNameOf(method: string): string {
  return `ember_${method.replaceAll('.', '_')}`;
}

/** 复制 Schema，把引用改成 $defs，并收集引用到的类型名。 */
function rewrite(value: unknown, used: Set<string>): unknown {
  if (Array.isArray(value)) return value.map((item) => rewrite(item, used));
  if (value === null || typeof value !== 'object') return value;
  const result: Record<string, unknown> = {};
  for (const [key, child] of Object.entries(value)) {
    if (key === 'x-binary') continue;
    if (key === '$ref' && typeof child === 'string' && child.startsWith(REF_PREFIX)) {
      const name = child.slice(REF_PREFIX.length);
      used.add(name);
      result.$ref = `#/$defs/${name}`;
    } else {
      result[key] = rewrite(child, used);
    }
  }
  return result;
}

/** 一个请求的输入 Schema：参数对象，加上引用到的类型定义（含间接引用）。 */
function inputSchemaOf(method: OpenRpcMethod, schemas: Readonly<Record<string, OpenRpcSchema>>): Record<string, unknown> {
  const used = new Set<string>();
  const properties = Object.fromEntries(method.params.map((param) => [param.name, rewrite({ ...param.schema, description: param.description ?? param.schema.description }, used)]));
  const defs: Record<string, unknown> = {};
  const queue = [...used];
  while (queue.length > 0) {
    const name = queue.pop() as string;
    if (name in defs) continue;
    const before = new Set(used);
    defs[name] = rewrite(schemas[name], used);
    for (const added of used) if (!before.has(added)) queue.push(added);
  }
  const schema: Record<string, unknown> = { type: 'object', properties, required: method.params.filter((param) => param.required).map((param) => param.name), additionalProperties: false };
  if (Object.keys(defs).length > 0) schema.$defs = defs;
  return schema;
}

/** 工具的说明：请求的说明，加上调用限制与错误码。 */
function descriptionOf(method: OpenRpcMethod): string {
  const notes: string[] = [];
  if (method['x-mainOnly'] === true) notes.push('这个请求原本只能由主进程发起，调试服务也可以调用。');
  const errors = (method.errors ?? []).map((error) => error.message);
  if (errors.length > 0) notes.push(`可能的错误码：${errors.join('、')}。`);
  return [method.description, ...notes].join('\n');
}

/** 读取事件的工具。 */
const EVENTS_TOOL: McpTool = {
  name: EVENTS_TOOL_NAME,
  description:
    '读取 Ember 发出的事件（如 runtime.stateChanged、runtime.runFinished、runtime.frame、preferences.changed）。第一次调用时开始记录（用 patterns 指定事件名，写法：事件名、模块名.*、*），之后每次调用返回自上次读取以来记录的事件并清空；记录最多保留 1000 条。',
  inputSchema: {
    type: 'object',
    properties: {
      patterns: { type: 'array', items: { type: 'string' }, description: '要记录的事件，如 ["runtime.*"]；省略表示全部。可以随时追加。' },
      max: { type: 'integer', minimum: 1, maximum: 1000, description: '最多返回的条数，默认 100；没返回的留到下次。' }
    },
    required: [],
    additionalProperties: false
  }
};

/** 由文档生成全部工具：每个请求一个，再加上读取事件的工具。 */
export function buildTools(document: OpenRpcDocument): readonly McpTool[] {
  const tools = document.methods
    .filter((method) => !EXCLUDED_METHODS.has(method.name))
    .map((method): McpTool => ({ name: toolNameOf(method.name), description: descriptionOf(method), inputSchema: inputSchemaOf(method, document.components.schemas), method: method.name }));
  return [...tools, EVENTS_TOOL];
}
