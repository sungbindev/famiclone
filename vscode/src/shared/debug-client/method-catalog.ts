// ------------------------------------------------------------------------
// 名称：method-catalog.ts
// 说明：OpenRPC 接口描述文档的读取：列出请求、按名称查找、把一个请求的参数、结果与错误码整理成可读
//       的文字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-14
// 备注：“调用调试请求”“查看调试请求说明”命令都用它；文档由 Ember 的 npm run schema 生成（debug/
//       openrpc.json）。
// ------------------------------------------------------------------------

/** 文档里的 JSON Schema 节点（只列用到的字段）。 */
export interface OpenRpcSchema {
  readonly type?: string | readonly string[];
  readonly description?: string;
  readonly properties?: Readonly<Record<string, OpenRpcSchema>>;
  readonly required?: readonly string[];
  readonly items?: OpenRpcSchema;
  readonly enum?: readonly string[];
  readonly $ref?: string;
  readonly minimum?: number;
  readonly maximum?: number;
  readonly minLength?: number;
  readonly maxLength?: number;
  readonly 'x-binary'?: boolean;
}

/** 请求的一个参数。 */
export interface OpenRpcParam {
  readonly name: string;
  readonly required: boolean;
  readonly description?: string;
  readonly schema: OpenRpcSchema;
}

/** 一个请求（方法）。 */
export interface OpenRpcMethod {
  readonly name: string;
  readonly summary: string;
  readonly description: string;
  readonly params: readonly OpenRpcParam[];
  readonly result: { readonly name: string; readonly schema: OpenRpcSchema };
  readonly errors?: ReadonlyArray<{ readonly message: string }>;
  readonly 'x-debugOnly'?: boolean;
  readonly 'x-mainOnly'?: boolean;
}

/** 一个事件。 */
export interface OpenRpcEvent {
  readonly name: string;
  readonly description: string;
  readonly data: OpenRpcSchema;
}

/** OpenRPC 文档。 */
export interface OpenRpcDocument {
  readonly methods: readonly OpenRpcMethod[];
  readonly components: { readonly schemas: Readonly<Record<string, OpenRpcSchema>> };
  readonly 'x-events': readonly OpenRpcEvent[];
}

/** 引用路径的前缀。 */
const REF_PREFIX = '#/components/schemas/';

/** 引用的类型名（模块.类型）。 */
function refKey(ref: string): string {
  return ref.startsWith(REF_PREFIX) ? ref.slice(REF_PREFIX.length) : ref;
}

/** 请求名里的模块名。 */
export function moduleOf(method: OpenRpcMethod): string {
  return method.name.split('.')[0] ?? '';
}

/**
 * 列出请求；给出关键字时只留名称或简介里含有它的（不分大小写）。
 */
export function listMethods(document: OpenRpcDocument, filter?: string): readonly OpenRpcMethod[] {
  if (filter === undefined || filter === '') return document.methods;
  const keyword = filter.toLowerCase();
  return document.methods.filter((method) => method.name.toLowerCase().includes(keyword) || method.summary.toLowerCase().includes(keyword));
}

/** 按名称查找请求。 */
export function findMethod(document: OpenRpcDocument, name: string): OpenRpcMethod | undefined {
  return document.methods.find((method) => method.name === name);
}

/**
 * 一个类型的简短写法，如 integer（1 到 10000000）、"png" | "rgba"、runtime.RuntimeState、cartridge
 * s.RecentCartridge[]。
 */
export function typeSummary(schema: OpenRpcSchema): string {
  if (schema.$ref !== undefined) return refKey(schema.$ref);
  if (schema['x-binary'] === true) return '二进制';
  if (schema.type === undefined) return '任意 JSON';
  const types = Array.isArray(schema.type) ? schema.type : [schema.type];
  const base = types.find((type) => type !== 'null') ?? 'null';
  let text: string;
  if (base === 'array') text = `${schema.items === undefined ? '任意' : typeSummary(schema.items)}[]`;
  else if (base === 'string' && schema.enum !== undefined) text = schema.enum.map((value) => JSON.stringify(value)).join(' | ');
  else text = base;
  const range: string[] = [];
  if (schema.minimum !== undefined || schema.maximum !== undefined) range.push(`${schema.minimum ?? ''}..${schema.maximum ?? ''}`);
  if (schema.minLength !== undefined || schema.maxLength !== undefined) range.push(`长度 ${schema.minLength ?? ''}..${schema.maxLength ?? ''}`);
  const nullable = types.includes('null') ? ' | null' : '';
  return `${text}${nullable}${range.length === 0 ? '' : `（${range.join('，')}）`}`;
}

/** 对象节点的属性行：名称、必填、类型、说明；引用的对象类型不展开。 */
function propertyLines(schema: OpenRpcSchema, indent: string): string[] {
  const required = new Set(schema.required ?? []);
  return Object.entries(schema.properties ?? {}).map(([name, child]) => `${indent}${name}${required.has(name) ? '' : '?'}: ${typeSummary(child)}${child.description === undefined ? '' : ` — ${child.description}`}`);
}

/**
 * 把一个请求整理成可读的文字：标志、说明、参数、结果、错误码与调用示例。
 */
export function describeMethod(method: OpenRpcMethod): string {
  const flags = [method['x-debugOnly'] === true ? '只经调试服务' : '', method['x-mainOnly'] === true ? '只能由主进程发起' : ''].filter((flag) => flag !== '');
  const lines: string[] = [flags.length === 0 ? method.name : `${method.name}（${flags.join('，')}）`];
  lines.push('', method.description, '', '参数：');
  if (method.params.length === 0) lines.push('  （无）');
  for (const param of method.params) lines.push(`  ${param.name}${param.required ? '' : '?'}: ${typeSummary(param.schema)}${param.description === undefined ? '' : ` — ${param.description}`}`);
  lines.push('', '结果：', ...(method.result.schema.properties === undefined ? [`  ${typeSummary(method.result.schema)}`] : propertyLines(method.result.schema, '  ')));
  if ((method.errors ?? []).length > 0) lines.push('', `错误码：${(method.errors ?? []).map((error) => error.message).join('、')}`);
  const example = method.params.filter((param) => param.required).map((param) => `${param.name}=<${param.schema.type === undefined ? 'json' : typeSummary(param.schema).split('（')[0]}>`);
  lines.push('', `示例（命令“FC Dev：调用调试请求”里输入）：${method.name}${example.length === 0 ? '' : ` ${example.join(' ')}`}`);
  return lines.join('\n');
}

/** 按名称查找事件。 */
export function findEvent(document: OpenRpcDocument, name: string): OpenRpcEvent | undefined {
  return document['x-events'].find((event) => event.name === name);
}

/** 把一个事件整理成可读的文字：说明与数据字段。 */
export function describeEvent(event: OpenRpcEvent): string {
  return [`${event.name}（事件，需要先订阅）`, '', event.description, '', '数据：', ...propertyLines(event.data, '  ')].join('\n');
}