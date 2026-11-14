/*
 * ------------------------------------------------------------------------
 * 名称：api-reference.ts
 * 说明：由 OpenRPC 文档生成协议的 Markdown 参考：总览页，加每个模块一页（请求、参数、结果、错误码、事件与数据类型）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：npm run docs 调用；不属于 SDK 的公开接口。
 * ------------------------------------------------------------------------
 */

import { moduleOf, typeSummary, type OpenRpcDocument, type OpenRpcMethod, type OpenRpcSchema } from './method-catalog.js';

/** 生成的一个文件。 */
export interface ApiReferenceFile {
  /** 相对输出目录的路径。 */
  readonly path: string;
  readonly content: string;
}

/** 表格单元格：转义竖线，去掉换行。 */
function cell(text: string): string {
  return text.replaceAll('|', '\\|').replaceAll(/\s*\n\s*/g, ' ');
}

/** 类型引用链接到所在模块页的类型小节。 */
function link(key: string): string {
  const [module = '', name = ''] = key.split('.');
  return `[${key}](${module}.md#${name.toLowerCase()})`;
}

/** 类型的 Markdown 写法：引用的类型链接到所在模块页的类型小节。 */
function typeText(schema: OpenRpcSchema): string {
  const summary = typeSummary(schema);
  if (schema.$ref !== undefined) return link(schema.$ref.slice('#/components/schemas/'.length));
  if (schema.type === 'array' && schema.items?.$ref !== undefined) return `${link(schema.items.$ref.slice('#/components/schemas/'.length))}[]`;
  return `\`${summary}\``;
}

/** 属性表。 */
function propertyTable(schema: OpenRpcSchema, firstColumn: string): string[] {
  const entries = Object.entries(schema.properties ?? {});
  if (entries.length === 0) return ['（无）'];
  const required = new Set(schema.required ?? []);
  return [`| ${firstColumn} | 类型 | 必填 | 说明 |`, '| --- | --- | --- | --- |', ...entries.map(([name, child]) => `| \`${name}\` | ${cell(typeText(child))} | ${required.has(name) ? '是' : '否'} | ${cell(child.description ?? '')} |`)];
}

/** 一个请求的小节。 */
function methodSection(method: OpenRpcMethod): string[] {
  const flags = [method['x-debugOnly'] === true ? '只经调试服务开放（界面不能发起）' : '', method['x-mainOnly'] === true ? '界面里只能由主进程发起，调试服务可以调用' : ''].filter((flag) => flag !== '');
  const lines = [`### ${method.name}`, ''];
  if (flags.length > 0) lines.push(`> ${flags.join('；')}`, '');
  lines.push(method.description, '', '**参数**', '');
  const params = { properties: Object.fromEntries(method.params.map((param) => [param.name, { ...param.schema, description: param.description ?? param.schema.description }])), required: method.params.filter((param) => param.required).map((param) => param.name) };
  lines.push(...propertyTable(params, '名称'), '', '**结果**', '');
  lines.push(...(method.result.schema.properties === undefined ? [typeText(method.result.schema)] : propertyTable(method.result.schema, '字段')), '');
  const errors = (method.errors ?? []).map((error) => `\`${error.message}\``);
  lines.push(`**错误码**：${errors.length === 0 ? '无专有错误码（参数不合格返回 `core.invalidParams`）' : errors.join('、')}`, '');
  const required = method.params.filter((param) => param.required).map((param) => `${param.name}=<${param.schema.type === undefined ? 'json' : typeSummary(param.schema).split('（')[0]}>`);
  lines.push('**示例**', '', '```text', `ember-debug call ${method.name}${required.length === 0 ? '' : ` ${required.join(' ')}`}`, '```', '');
  const args = method.params.filter((param) => param.required).map((param) => `${param.name}: …`);
  lines.push('```ts', `await client.api.${moduleOf(method)}.${method.name.split('.')[1]}(${args.length === 0 ? '' : `{ ${args.join(', ')} }`});`, '```', '');
  return lines;
}

/** 模块页。 */
function modulePage(module: string, document: OpenRpcDocument): string {
  const lines = [`# ${module} 模块`, '', '请求与事件的规则、连接与认证见 [README](README.md) 与 Ember 的《调试服务》文档。', ''];
  const methods = document.methods.filter((method) => moduleOf(method) === module);
  lines.push('## 请求', '', '| 请求 | 简介 |', '| --- | --- |', ...methods.map((method) => `| [\`${method.name}\`](#${method.name.replaceAll('.', '').toLowerCase()}) | ${cell(method.summary)} |`), '');
  for (const method of methods) lines.push(...methodSection(method));
  const events = document['x-events'].filter((event) => event.name.startsWith(`${module}.`));
  if (events.length > 0) {
    lines.push('## 事件', '', '事件要先用 `debug.subscribe` 订阅才会收到，以 JSON-RPC 通知送达。', '');
    for (const event of events) lines.push(`### ${event.name}`, '', event.description, '', ...propertyTable(event.data, '字段'), '');
  }
  const types = Object.entries(document.components.schemas).filter(([key]) => key.startsWith(`${module}.`));
  if (types.length > 0) {
    lines.push('## 数据类型', '');
    for (const [key, schema] of types) lines.push(`### ${key.split('.')[1]}`, '', schema.description ?? '', '', ...propertyTable(schema, '字段'), '');
  }
  return `${lines.join('\n').replaceAll(/\n{3,}/g, '\n\n').trimEnd()}\n`;
}

/** 总览页。 */
function overviewPage(modules: readonly string[], document: OpenRpcDocument): string {
  const lines = [
    '# Ember 调试服务协议参考',
    '',
    '本目录由 `npm run docs` 根据随 SDK 打包的 OpenRPC 文档（`packages/ember-debug-sdk/generated/openrpc.json`）生成，不要手改。',
    '',
    '## 连接',
    '',
    '- 地址 `ws://127.0.0.1:<端口>/`（WebSocket），默认端口 5688；可选子协议 `ember-debug.v1`。',
    '- 报文是 JSON-RPC 2.0，参数按名称传递；事件是没有 `id` 的通知。',
    '- 每条连接的第一个请求必须是 `debug.hello`（带令牌），5 秒内不握手或令牌不对，连接以关闭码 4001 关闭。',
    '- 结果或事件数据里顶层的 `data` 是二进制时，整条消息改发一个二进制帧：4 字节大端的 JSON 长度、JSON 报文（不含 `data`）、`data` 的原始字节。SDK 已处理，`data` 直接是 `Uint8Array`。',
    '- 令牌与端口在 Ember 的首选项“调试服务”分组里；Ember 运行时用户数据目录的 `run/debug-endpoint.json` 里有当前的地址与令牌，SDK 与终端客户端用它自动连接。',
    '- 完整的协议规则（关闭码、限制、保活、事件队列）见 Ember 的《调试服务》文档（docs/04-调试服务.md）。',
    '',
    '## 错误',
    '',
    '错误响应的 `error.code` 是 JSON-RPC 错误码（-32700 报文不是 JSON、-32600 不是合法请求、-32601 方法不存在、-32602 参数不合格、-32603 核心内部错误、-32000 业务错误），`error.data.code` 是模块登记的错误码（如 `runtime.noCartridge`）。SDK 把它抛成 `EmberRequestError`（`code` 为错误码）。',
    '',
    '## 模块',
    ''
  ];
  for (const module of modules) {
    const methods = document.methods.filter((method) => moduleOf(method) === module);
    lines.push(`### [${module}](${module}.md)`, '', '| 请求 | 简介 |', '| --- | --- |', ...methods.map((method) => `| \`${method.name}\` | ${cell(method.summary)} |`), '');
    const events = document['x-events'].filter((event) => event.name.startsWith(`${module}.`));
    if (events.length > 0) lines.push(`事件：${events.map((event) => `\`${event.name}\``).join('、')}`, '');
  }
  return `${lines.join('\n').trimEnd()}\n`;
}

/**
 * 生成协议参考的全部文件。
 * @param document OpenRPC 文档。
 */
export function generateApiReference(document: OpenRpcDocument): readonly ApiReferenceFile[] {
  const modules = [...new Set(document.methods.map(moduleOf))];
  return [{ path: 'README.md', content: overviewPage(modules, document) }, ...modules.map((module) => ({ path: `${module}.md`, content: modulePage(module, document) }))];
}
