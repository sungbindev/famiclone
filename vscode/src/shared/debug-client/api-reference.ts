// ------------------------------------------------------------------------
// 名称：api-reference.ts
// 说明：由 OpenRPC 文档生成协议的 Markdown 参考：总览页（index.md），加每个模块一页（请求、参数、结
//       果、错误码、事件与数据类型）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-14
// 备注：vscode/tools/gen/gen-debug-api-docs.mjs 调用，输出到 Ember 帮助文档的
//       help/api；页面里的“连接与报文”链接指向帮助文档里手写的 help/debug/protocol.md。
// ------------------------------------------------------------------------

import { moduleOf, typeSummary, type OpenRpcDocument, type OpenRpcMethod, type OpenRpcSchema } from './method-catalog';

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
  lines.push('**示例**', '', '在 VS Code 的命令“FC Dev：调用调试请求”里输入：', '', '```text', `${method.name}${required.length === 0 ? '' : ` ${required.join(' ')}`}`, '```', '');
  return lines;
}

/** 模块页。 */
function modulePage(module: string, document: OpenRpcDocument): string {
  const lines = [`# ${module} 模块`, '', '连接、认证与报文规则见[连接与报文](../debug/protocol.md)，全部请求见[请求与事件总览](index.md)。', ''];
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
    '<!-- 本文件由 npm run help:build 根据 OpenRPC 文档生成，不要手改 -->',
    '',
    '# 请求与事件总览',
    '',
    '调试服务的全部请求与事件，按模块分页。连接、认证、报文格式、二进制帧、限制与关闭码见[连接与报文](../debug/protocol.md)。',
    '',
    '## 错误',
    '',
    '错误响应的 `error.code` 是 JSON-RPC 错误码（-32700 报文不是 JSON、-32600 不是合法请求、-32601 方法不存在、-32602 参数不合格、-32603 核心内部错误、-32000 业务错误），`error.data.code` 是模块登记的错误码（如 `runtime.noCartridge`）。扩展的调试客户端把它抛成 `EmberRequestError`（`code` 为错误码）。',
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
  return [{ path: 'index.md', content: overviewPage(modules, document) }, ...modules.map((module) => ({ path: `${module}.md`, content: modulePage(module, document) }))];
}
