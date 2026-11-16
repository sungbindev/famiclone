/*
 * ------------------------------------------------------------------------
 * 名称：schema-gen.mjs
 * 说明：Schema 代码生成：读取 schema/<模块>.schema.json，生成 TypeScript 的请求名、事件名、数据类型与校验用的 Schema，
 *       以及 C++ 的数据结构、请求名常量、JSON 转换与结构校验；另生成汇总全部模块的 TypeScript 文件。
 *       主进程处理的请求（mainRequests）只生成 TypeScript，不生成 C++；
 *       请求可标记 mainOnly（只能由主进程发起）或 debugOnly（只经调试服务开放），两者都不进入界面可发起的请求名单与 CoreApi 的方法。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：只支持 JSON Schema 的子集（见 docs/06-构建与模块登记.md 第 2 节）；本仓库的生成目录不提交，内容没变的文件不改写；
 *       已删除的模块，其生成目录一并删除；调试 SDK（packages/ember-debug-sdk）的 generated 目录一并写入接口文档与类型。可作为模块被测试引用（generate），也可直接运行。
 * ------------------------------------------------------------------------
 */

import { existsSync, mkdirSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { generateOpenRpc, generateOpenRpcCpp } from './openrpc.mjs';
import { generateSdkTypes } from './sdk-ts.mjs';

const MODULE_NAME_PATTERN = /^[a-z][a-z0-9]*(-[a-z0-9]+)*$/;
/** 各模块 Schema 文件名的后缀，前面是模块名。 */
const SCHEMA_FILE_SUFFIX = '.schema.json';
const MEMBER_NAME_PATTERN = /^[a-z][A-Za-z0-9]*$/;
const TYPE_NAME_PATTERN = /^[A-Z][A-Za-z0-9]*$/;
const NODE_KEYWORDS = new Set(['type', 'description', 'properties', 'required', 'items', 'minItems', 'maxItems', 'minLength', 'maxLength', 'enum', 'minimum', 'maximum', '$ref']);
const PRIMITIVES = new Set(['string', 'integer', 'number', 'boolean']);
/** 属性名会成为 C++ 结构体的成员名，不能是 C++ 关键字。 */
const CPP_KEYWORDS = new Set(
  'alignas alignof and asm auto bitand bitor bool break case catch char char8_t char16_t char32_t class compl concept const consteval constexpr constinit const_cast continue co_await co_return co_yield decltype default delete do double dynamic_cast else enum explicit export extern false float for friend goto if inline int long mutable namespace new noexcept not nullptr operator or private protected public register reinterpret_cast requires return short signed sizeof static static_assert static_cast struct switch template this thread_local throw true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor'.split(' ')
);
const TS_HEADER = (source) =>
  `// 由 tools/schema-gen/schema-gen.mjs 根据 ${source} 生成，不要手改。\n/* eslint-disable */\n\n`;
const CPP_HEADER = (source) => `// 由 tools/schema-gen/schema-gen.mjs 根据 ${source} 生成，不要手改。\n\n`;

/** kebab-case 转 PascalCase。 */
function pascal(name) {
  return name
    .split(/[-_]/)
    .map((part) => part.charAt(0).toUpperCase() + part.slice(1))
    .join('');
}

/** kebab-case 或 camelCase 转 snake_case。 */
function snake(name) {
  return name
    .replace(/-/g, '_')
    .replace(/([a-z0-9])([A-Z])/g, '$1_$2')
    .toLowerCase();
}

/** 抛出带位置的格式错误。 */
function fail(file, path, message) {
  throw new Error(`${file}：${path} ${message}`);
}

/** 解析 $ref：返回引用的模块与类型名。 */
function parseRef(ref, currentModule, file, path) {
  const match = /^(?:([a-z][a-z0-9-]*))?#\/types\/([A-Z][A-Za-z0-9]*)$/.exec(ref);
  if (!match) fail(file, path, `的 $ref 写法不对：${ref}，应为 #/types/名称 或 模块#/types/名称`);
  return { module: match[1] ?? currentModule, name: match[2] };
}

/** 节点的类型与是否可空；没有 type 也没有 $ref 的节点是任意 JSON。 */
function typeOf(node) {
  if (node.type === undefined && node.$ref === undefined) return { type: 'any', nullable: false };
  if (Array.isArray(node.type)) {
    const rest = node.type.filter((type) => type !== 'null');
    return { type: rest[0], nullable: node.type.includes('null') };
  }
  return { type: node.type, nullable: false };
}

/** 检查一个取值节点（属性、数组项）。 */
function checkValueNode(node, file, path, currentModule, refs) {
  if (node === null || typeof node !== 'object' || Array.isArray(node)) fail(file, path, '必须是对象');
  for (const key of Object.keys(node)) if (!NODE_KEYWORDS.has(key)) fail(file, path, `不支持的关键字：${key}`);
  if (node.$ref !== undefined) {
    refs.push(parseRef(node.$ref, currentModule, file, path));
    return;
  }
  if (node.type === undefined) {
    for (const key of Object.keys(node)) if (key !== 'description') fail(file, path, `是任意 JSON，只能写 description，不能写 ${key}`);
    return;
  }
  const { type } = typeOf(node);
  if (Array.isArray(node.type) && (node.type.length !== 2 || !node.type.includes('null'))) fail(file, path, '可空类型只能写成 ["类型", "null"]');
  if (type === 'object') fail(file, path, '属性不能直接写对象，请在 types 中定义后用 $ref 引用');
  if (type === 'array') {
    if (node.items === undefined) fail(file, path, '数组必须有 items');
    checkValueNode(node.items, file, `${path}.items`, currentModule, refs);
    return;
  }
  if (!PRIMITIVES.has(type)) fail(file, path, `不支持的类型：${String(node.type)}`);
  if (node.enum !== undefined && (type !== 'string' || !Array.isArray(node.enum) || node.enum.some((item) => typeof item !== 'string'))) {
    fail(file, path, '只支持字符串枚举');
  }
}

/** 检查一个对象节点（类型定义、请求参数与结果、事件数据）。 */
function checkObjectNode(node, file, path, currentModule, refs) {
  if (node === null || typeof node !== 'object' || node.type !== 'object') fail(file, path, '必须是 type 为 object 的对象');
  for (const key of Object.keys(node)) if (!['type', 'description', 'properties', 'required'].includes(key)) fail(file, path, `不支持的关键字：${key}`);
  const properties = node.properties ?? {};
  const required = node.required ?? [];
  for (const name of required) if (!(name in properties)) fail(file, path, `的 required 中有不存在的属性：${name}`);
  for (const [name, child] of Object.entries(properties)) {
    if (!MEMBER_NAME_PATTERN.test(name)) fail(file, `${path}.${name}`, '属性名必须是 camelCase');
    if (CPP_KEYWORDS.has(name)) fail(file, `${path}.${name}`, '属性名不能是 C++ 关键字');
    checkValueNode(child, file, `${path}.${name}`, currentModule, refs);
  }
}

/**
 * 读取并检查全部模块的 Schema。
 * @param {string} root 项目根目录。
 * @returns {Array<object>} 各模块的 Schema（按模块名排序）。
 */
export function loadSchemas(root) {
  const schemaDir = join(root, 'schema');
  if (!existsSync(schemaDir)) return [];
  const modules = [];
  for (const fileName of readdirSync(schemaDir).toSorted()) {
    if (!fileName.endsWith(SCHEMA_FILE_SUFFIX)) throw new Error(`schema/${fileName} 不是 <模块>${SCHEMA_FILE_SUFFIX}，schema/ 下只放各模块的 Schema 文件`);
    const name = fileName.slice(0, -SCHEMA_FILE_SUFFIX.length);
    const file = join(schemaDir, fileName);
    if (!MODULE_NAME_PATTERN.test(name)) throw new Error(`模块名必须是 kebab-case：${name}`);
    const relative = `schema/${fileName}`;
    const schema = JSON.parse(readFileSync(file, 'utf8'));
    if (schema.module !== name) fail(relative, 'module', `必须与文件名一致：${name}`);
    for (const key of Object.keys(schema)) if (!['$comment', 'module', 'types', 'requests', 'mainRequests', 'events'].includes(key)) fail(relative, key, '是不支持的顶层字段');
    const refs = [];
    for (const [typeName, node] of Object.entries(schema.types ?? {})) {
      if (!TYPE_NAME_PATTERN.test(typeName)) fail(relative, `types.${typeName}`, '类型名必须是 PascalCase');
      checkObjectNode(node, relative, `types.${typeName}`, name, refs);
    }
    for (const [operation, request] of Object.entries(schema.requests ?? {})) {
      if (!MEMBER_NAME_PATTERN.test(operation)) fail(relative, `requests.${operation}`, '操作名必须是 camelCase');
      for (const key of Object.keys(request)) if (!['description', 'params', 'result', 'errors', 'mainOnly', 'debugOnly'].includes(key)) fail(relative, `requests.${operation}.${key}`, '是不支持的字段');
      if (request.mainOnly !== undefined && request.mainOnly !== true) fail(relative, `requests.${operation}.mainOnly`, '只能写 true');
      if (request.debugOnly !== undefined && request.debugOnly !== true) fail(relative, `requests.${operation}.debugOnly`, '只能写 true');
      if (request.mainOnly === true && request.debugOnly === true) fail(relative, `requests.${operation}`, '不能同时标记 mainOnly 与 debugOnly');
      checkObjectNode(request.params, relative, `requests.${operation}.params`, name, refs);
      checkObjectNode(request.result, relative, `requests.${operation}.result`, name, refs);
      for (const code of request.errors ?? []) if (!code.startsWith(`${name}.`)) fail(relative, `requests.${operation}.errors`, `的错误码必须以模块名开头：${code}`);
    }
    // 主进程处理的请求：界面经预加载脚本发给主进程，不进入核心；参数与结果的类型名与核心请求共用一套命名，所以操作名不能重复。
    for (const [operation, request] of Object.entries(schema.mainRequests ?? {})) {
      if (!MEMBER_NAME_PATTERN.test(operation)) fail(relative, `mainRequests.${operation}`, '操作名必须是 camelCase');
      if (operation in (schema.requests ?? {})) fail(relative, `mainRequests.${operation}`, '与 requests 中的操作重名');
      for (const key of Object.keys(request)) if (!['description', 'params', 'result', 'errors'].includes(key)) fail(relative, `mainRequests.${operation}.${key}`, '是不支持的字段');
      checkObjectNode(request.params, relative, `mainRequests.${operation}.params`, name, refs);
      checkObjectNode(request.result, relative, `mainRequests.${operation}.result`, name, refs);
      for (const code of request.errors ?? []) if (!code.startsWith(`${name}.`)) fail(relative, `mainRequests.${operation}.errors`, `的错误码必须以模块名开头：${code}`);
    }
    for (const [event, definition] of Object.entries(schema.events ?? {})) {
      if (!MEMBER_NAME_PATTERN.test(event)) fail(relative, `events.${event}`, '事件名必须是 camelCase');
      checkObjectNode(definition.data, relative, `events.${event}.data`, name, refs);
    }
    modules.push({ name, file: relative, schema, refs });
  }
  // 引用的模块与类型必须存在。
  const byName = new Map(modules.map((module) => [module.name, module]));
  for (const module of modules) {
    for (const ref of module.refs) {
      const target = byName.get(ref.module);
      if (target === undefined || !(ref.name in (target.schema.types ?? {}))) throw new Error(`${module.file}：引用的类型不存在：${ref.module}#/types/${ref.name}`);
    }
  }
  return modules;
}

// ---------------------------------------------------------------- TypeScript

/** 取值节点的 TypeScript 类型。 */
function tsValueType(node, currentModule, imports) {
  if (node.$ref !== undefined) {
    const ref = parseRef(node.$ref, currentModule, '', '');
    if (ref.module !== currentModule) imports.add(`${ref.module}:${ref.name}`);
    return ref.name;
  }
  const { type, nullable } = typeOf(node);
  let base;
  if (type === 'any') return 'unknown';
  if (type === 'array') base = `${tsValueType(node.items, currentModule, imports)}[]`;
  else if (type === 'string') base = node.enum ? node.enum.map((item) => JSON.stringify(item)).join(' | ') : 'string';
  else if (type === 'integer' || type === 'number') base = 'number';
  else base = 'boolean';
  if (type === 'array' && node.items.enum) base = `Array<${tsValueType(node.items, currentModule, imports)}>`;
  return nullable ? `${base} | null` : base;
}

/** 对象节点的 TypeScript 接口。 */
function tsInterface(name, node, currentModule, imports) {
  const lines = [];
  if (node.description) lines.push(`/** ${node.description} */`);
  const properties = Object.entries(node.properties ?? {});
  if (properties.length === 0) {
    lines.push(`export type ${name} = Record<string, never>;`);
    return lines.join('\n');
  }
  lines.push(`export interface ${name} {`);
  const required = new Set(node.required ?? []);
  for (const [property, child] of properties) {
    if (child.description) lines.push(`  /** ${child.description} */`);
    lines.push(`  ${property}${required.has(property) ? '' : '?'}: ${tsValueType(child, currentModule, imports)};`);
  }
  lines.push('}');
  return lines.join('\n');
}

/** 生成一个模块的 TypeScript 文件。 */
function generateTs(module) {
  const { name, schema, file } = module;
  const prefix = pascal(name);
  const constant = snake(name).toUpperCase();
  const imports = new Set();
  const blocks = [];
  for (const [typeName, node] of Object.entries(schema.types ?? {})) blocks.push(tsInterface(typeName, node, name, imports));
  const requestEntries = Object.entries(schema.requests ?? {});
  const mainRequestEntries = Object.entries(schema.mainRequests ?? {});
  const eventEntries = Object.entries(schema.events ?? {});
  for (const [operation, request] of [...requestEntries, ...mainRequestEntries]) {
    blocks.push(tsInterface(`${prefix}${pascal(operation)}Params`, request.params, name, imports));
    blocks.push(tsInterface(`${prefix}${pascal(operation)}Result`, request.result, name, imports));
  }
  for (const [event, definition] of eventEntries) blocks.push(tsInterface(`${prefix}${pascal(event)}Data`, definition.data, name, imports));
  const types = schema.types ?? {};

  const requestNames = requestEntries.map(([operation]) => `  ${operation}: '${name}.${operation}'`).join(',\n');
  const mainRequestNames = mainRequestEntries.map(([operation]) => `  ${operation}: '${name}.${operation}'`).join(',\n');
  const eventNames = eventEntries.map(([event]) => `  ${event}: '${name}.${event}'`).join(',\n');
  const toMap = (entries) =>
    entries.map(([operation]) => `  '${name}.${operation}': { params: ${prefix}${pascal(operation)}Params; result: ${prefix}${pascal(operation)}Result };`).join('\n');
  const requestMap = toMap(requestEntries);
  const mainRequestMap = toMap(mainRequestEntries);
  const eventMap = eventEntries.map(([event]) => `  '${name}.${event}': ${prefix}${pascal(event)}Data;`).join('\n');
  const importLines = [...imports]
    .toSorted()
    .map((entry) => {
      const [target, type] = entry.split(':');
      return `import type { ${type} } from '../../${target}/generated/${target}';`;
    });

  return [
    TS_HEADER(file),
    `import type { ModuleSchema } from '../../../shared/schema/schema-types';`,
    ...importLines,
    '',
    blocks.join('\n\n'),
    '',
    `/** 本模块的请求名。 */`,
    `export const ${constant}_REQUESTS = {\n${requestNames}\n} as const;`,
    '',
    `/** 本模块由主进程处理的请求名。 */`,
    `export const ${constant}_MAIN_REQUESTS = {\n${mainRequestNames}\n} as const;`,
    '',
    `/** 本模块的事件名。 */`,
    `export const ${constant}_EVENTS = {\n${eventNames}\n} as const;`,
    '',
    `/** 请求名到参数与结果类型的对照。 */`,
    requestMap ? `export interface ${prefix}RequestMap {\n${requestMap}\n}` : `export type ${prefix}RequestMap = Record<never, never>;`,
    '',
    `/** 主进程处理的请求名到参数与结果类型的对照。 */`,
    mainRequestMap ? `export interface ${prefix}MainRequestMap {\n${mainRequestMap}\n}` : `export type ${prefix}MainRequestMap = Record<never, never>;`,
    '',
    `/** 事件名到数据类型的对照。 */`,
    eventMap ? `export interface ${prefix}EventMap {\n${eventMap}\n}` : `export type ${prefix}EventMap = Record<never, never>;`,
    '',
    `/** 本模块的 Schema，供入口校验使用。 */`,
    `export const ${constant}_SCHEMA: ModuleSchema = ${JSON.stringify({ module: name, types, requests: schema.requests ?? {}, mainRequests: schema.mainRequests ?? {}, events: schema.events ?? {} }, null, 2)};`,
    ''
  ].join('\n');
}

/** 界面可以发起的请求：只能由主进程发起（mainOnly）与只经调试服务开放（debugOnly）的请求不进入预加载脚本暴露的名单与方法。 */
function rendererRequests(module) {
  return Object.entries(module.schema.requests ?? {})
    .filter(([, request]) => request.mainOnly !== true && request.debugOnly !== true)
    .map(([operation]) => operation);
}

/** 生成汇总全部模块的 TypeScript 文件。 */
function generateTsIndex(modules) {
  const imports = modules.map((module) => {
    const prefix = pascal(module.name);
    const constant = snake(module.name).toUpperCase();
    return `import { ${constant}_SCHEMA, type ${prefix}EventMap, type ${prefix}MainRequestMap, type ${prefix}RequestMap } from '../../${module.name}/generated/${module.name}';`;
  });
  const requestMaps = modules.map((module) => `${pascal(module.name)}RequestMap`);
  const mainRequestMaps = modules.map((module) => `${pascal(module.name)}MainRequestMap`);
  const eventMaps = modules.map((module) => `${pascal(module.name)}EventMap`);
  const requestNames = modules.flatMap((module) => rendererRequests(module).map((operation) => `  '${module.name}.${operation}'`));
  const mainRequestNames = modules.flatMap((module) => Object.keys(module.schema.mainRequests ?? {}).map((operation) => `  '${module.name}.${operation}'`));
  const eventNames = modules.flatMap((module) => Object.keys(module.schema.events ?? {}).map((event) => `  '${module.name}.${event}'`));
  const api = modules.map((module) => {
    const operations = rendererRequests(module).map(
      (operation) => `    ${operation}: (params: CoreParams<'${module.name}.${operation}'>) => Promise<CoreResult<'${module.name}.${operation}'>>;`
    );
    return `  ${JSON.stringify(module.name)}: {\n${operations.join('\n')}\n  };`;
  });
  const mainApi = modules
    .filter((module) => Object.keys(module.schema.mainRequests ?? {}).length > 0)
    .map((module) => {
      const operations = Object.keys(module.schema.mainRequests).map(
        (operation) => `    ${operation}: (params: MainParams<'${module.name}.${operation}'>) => Promise<MainResult<'${module.name}.${operation}'>>;`
      );
      return `  ${JSON.stringify(module.name)}: {\n${operations.join('\n')}\n  };`;
    });
  return [
    TS_HEADER('schema/ 下全部模块'),
    `import type { ModuleSchema } from '../../../shared/schema/schema-types';`,
    ...imports,
    '',
    `/** 全部请求名到参数与结果类型的对照。 */`,
    requestMaps.length > 0 ? `export type CoreRequestMap = ${requestMaps.join(' & ')};` : 'export type CoreRequestMap = Record<never, never>;',
    '',
    `/** 全部主进程处理的请求名到参数与结果类型的对照。 */`,
    mainRequestMaps.length > 0 ? `export type MainRequestMap = ${mainRequestMaps.join(' & ')};` : 'export type MainRequestMap = Record<never, never>;',
    '',
    `/** 全部事件名到数据类型的对照。 */`,
    eventMaps.length > 0 ? `export type CoreEventMap = ${eventMaps.join(' & ')};` : 'export type CoreEventMap = Record<never, never>;',
    '',
    `/** 请求名。 */`,
    'export type CoreRequestName = keyof CoreRequestMap & string;',
    `/** 事件名。 */`,
    'export type CoreEventName = keyof CoreEventMap & string;',
    `/** 请求的参数类型。 */`,
    `export type CoreParams<N extends CoreRequestName> = CoreRequestMap[N] extends { params: infer P } ? P : never;`,
    `/** 请求的结果类型。 */`,
    `export type CoreResult<N extends CoreRequestName> = CoreRequestMap[N] extends { result: infer R } ? R : never;`,
    `/** 事件的数据类型。 */`,
    `export type CoreEventData<N extends CoreEventName> = CoreEventMap[N];`,
    `/** 主进程处理的请求名。 */`,
    'export type MainRequestName = keyof MainRequestMap & string;',
    `/** 主进程处理的请求的参数类型。 */`,
    `export type MainParams<N extends MainRequestName> = MainRequestMap[N] extends { params: infer P } ? P : never;`,
    `/** 主进程处理的请求的结果类型。 */`,
    `export type MainResult<N extends MainRequestName> = MainRequestMap[N] extends { result: infer R } ? R : never;`,
    '',
    `/** 界面可以发起的请求名；只能由主进程发起与只经调试服务开放的请求不在其中。 */`,
    `export const CORE_REQUEST_NAMES: readonly string[] = [\n${requestNames.join(',\n')}\n];`,
    '',
    `/** 由主进程处理的请求名。 */`,
    `export const MAIN_REQUEST_NAMES: readonly string[] = [\n${mainRequestNames.join(',\n')}\n];`,
    '',
    `/** 全部事件名。 */`,
    `export const CORE_EVENT_NAMES: readonly string[] = [\n${eventNames.join(',\n')}\n];`,
    '',
    `/** 全部模块的 Schema。 */`,
    `export const CORE_SCHEMAS: readonly ModuleSchema[] = [${modules.map((module) => `${snake(module.name).toUpperCase()}_SCHEMA`).join(', ')}];`,
    '',
    `/** 按模块分组的请求方法：预加载脚本按它向界面暴露具名方法。 */`,
    `export interface CoreApi {\n${api.join('\n')}\n}`,
    '',
    `/** 按模块分组的主进程请求方法：预加载脚本按它向界面暴露具名方法。 */`,
    mainApi.length > 0 ? `export interface MainApi {\n${mainApi.join('\n')}\n}` : 'export type MainApi = Record<never, never>;',
    ''
  ].join('\n');
}

// ---------------------------------------------------------------- C++

/** 取值节点的 C++ 类型（不含可选包装）。 */
function cppBaseType(node, currentModule, includes) {
  if (node.$ref !== undefined) {
    const ref = parseRef(node.$ref, currentModule, '', '');
    if (ref.module !== currentModule) {
      includes.add(ref.module);
      return `::ember::contracts::${snake(ref.module)}::${ref.name}`;
    }
    return ref.name;
  }
  const { type } = typeOf(node);
  if (type === 'any') return 'nlohmann::json';
  if (type === 'array') return `std::vector<${cppValueType(node.items, currentModule, includes)}>`;
  if (type === 'string') return 'std::string';
  if (type === 'integer') return 'std::int64_t';
  if (type === 'number') return 'double';
  return 'bool';
}

/** 取值节点的 C++ 类型（可空时为 std::optional）。 */
function cppValueType(node, currentModule, includes) {
  const base = cppBaseType(node, currentModule, includes);
  return typeOf(node).nullable ? `std::optional<${base}>` : base;
}

/** C++ 字符串字面量。 */
function cppString(value) {
  return `"${String(value).replace(/\\/g, '\\\\').replace(/"/g, '\\"')}"`;
}

/** 可选的长度、项数。 */
function optionalSize(value) {
  return value === undefined ? 'std::nullopt' : `std::optional<std::size_t>(${value})`;
}

/** 可选的整数范围。 */
function optionalInt(value) {
  return value === undefined ? 'std::nullopt' : `std::optional<std::int64_t>(${value})`;
}

/** 可选的数字范围。 */
function optionalDouble(value) {
  return value === undefined ? 'std::nullopt' : `std::optional<double>(${value})`;
}

/** 读取一个取值节点的 C++ 表达式。 */
function cppReadExpr(node, json, path, currentModule) {
  if (node.$ref !== undefined) {
    const ref = parseRef(node.$ref, currentModule, '', '');
    const qualifier = ref.module === currentModule ? '' : `::ember::contracts::${snake(ref.module)}::`;
    return `${qualifier}parse${ref.name}(${json}, ${path})`;
  }
  const { type } = typeOf(node);
  if (type === 'any') return `nlohmann::json(${json})`;
  if (type === 'string') {
    const allowed = node.enum ? `std::vector<std::string>{${node.enum.map(cppString).join(', ')}}` : 'std::vector<std::string>{}';
    return `json_fields::readString(${json}, ${path}, ${optionalSize(node.minLength)}, ${optionalSize(node.maxLength)}, ${allowed})`;
  }
  if (type === 'integer') return `json_fields::readInteger(${json}, ${path}, ${optionalInt(node.minimum)}, ${optionalInt(node.maximum)})`;
  if (type === 'number') return `json_fields::readNumber(${json}, ${path}, ${optionalDouble(node.minimum)}, ${optionalDouble(node.maximum)})`;
  if (type === 'boolean') return `json_fields::readBoolean(${json}, ${path})`;
  const itemType = cppValueType(node.items, currentModule, new Set());
  const item = cppReadNullable(node.items, 'item', 'itemPath', currentModule);
  return `json_fields::readArray<${itemType}>(${json}, ${path}, ${optionalSize(node.minItems)}, ${optionalSize(node.maxItems)}, [](const nlohmann::json& item, const std::string& itemPath) { return ${item}; })`;
}

/** 读取一个可能可空的取值节点。 */
function cppReadNullable(node, json, path, currentModule) {
  const expression = cppReadExpr(node, json, path, currentModule);
  if (!typeOf(node).nullable) return expression;
  const base = cppBaseType(node, currentModule, new Set());
  return `((${json}).is_null() ? std::optional<${base}>() : std::optional<${base}>(${expression}))`;
}

/** 对象节点的 C++ 结构体声明。 */
function cppStruct(name, node, currentModule, includes) {
  const lines = [];
  if (node.description) lines.push(`/** ${node.description} */`);
  lines.push(`struct ${name} {`);
  const required = new Set(node.required ?? []);
  for (const [property, child] of Object.entries(node.properties ?? {})) {
    if (child.description) lines.push(`  /** ${child.description} */`);
    const type = cppValueType(child, currentModule, includes);
    const optional = !required.has(property) && !typeOf(child).nullable;
    lines.push(`  ${optional ? `std::optional<${type}>` : type} ${property}{};`);
  }
  lines.push('};');
  return lines.join('\n');
}

/** 对象节点的读取与写出函数实现。 */
function cppCodec(name, node, currentModule) {
  const required = new Set(node.required ?? []);
  const reads = [];
  const writes = [];
  for (const [property, child] of Object.entries(node.properties ?? {})) {
    const key = cppString(property);
    const path = `path + ".${property}"`;
    const read = cppReadNullable(child, '*field', path, currentModule);
    const isNullable = typeOf(child).nullable;
    if (required.has(property)) {
      reads.push(`  {\n    const nlohmann::json* field = json_fields::find(json, ${key});\n    if (field == nullptr) throw ::ember::shared::SchemaError(${path} + " 缺少");\n    value.${property} = ${read};\n  }`);
      writes.push(isNullable ? `  json[${key}] = value.${property} ? nlohmann::json(*value.${property}) : nlohmann::json(nullptr);` : `  json[${key}] = value.${property};`);
    } else if (isNullable) {
      reads.push(`  {\n    const nlohmann::json* field = json_fields::find(json, ${key});\n    if (field != nullptr) value.${property} = ${read};\n  }`);
      writes.push(`  if (value.${property}) json[${key}] = *value.${property};`);
    } else {
      reads.push(`  {\n    const nlohmann::json* field = json_fields::find(json, ${key});\n    if (field != nullptr && !field->is_null()) value.${property} = ${cppReadExpr(child, '*field', path, currentModule)};\n  }`);
      writes.push(`  if (value.${property}) json[${key}] = *value.${property};`);
    }
  }
  const known = Object.keys(node.properties ?? {}).map(cppString).join(', ');
  return [
    `${name} parse${name}(const nlohmann::json& json, const std::string& path) {`,
    '  json_fields::requireObject(json, path);',
    `  json_fields::rejectUnknown(json, path, {${known}});`,
    `  ${name} value;`,
    ...reads,
    '  return value;',
    '}',
    '',
    `void to_json(nlohmann::json& json, const ${name}& value) {`,
    '  json = nlohmann::json::object();',
    ...(writes.length === 0 ? ['  static_cast<void>(value);'] : writes),
    '}'
  ].join('\n');
}

/** 生成一个模块的 C++ 头文件与源文件。 */
function generateCpp(module) {
  const { name, schema, file } = module;
  const prefix = pascal(name);
  const constant = snake(name).toUpperCase();
  const namespace = `ember::contracts::${snake(name)}`;
  const includes = new Set();
  const objects = [];
  for (const [typeName, node] of Object.entries(schema.types ?? {})) objects.push([typeName, node]);
  for (const [operation, request] of Object.entries(schema.requests ?? {})) {
    objects.push([`${prefix}${pascal(operation)}Params`, request.params]);
    objects.push([`${prefix}${pascal(operation)}Result`, request.result]);
  }
  for (const [event, definition] of Object.entries(schema.events ?? {})) objects.push([`${prefix}${pascal(event)}Data`, definition.data]);
  const structs = objects.map(([typeName, node]) => cppStruct(typeName, node, name, includes));
  const declarations = objects.map(
    ([typeName]) => `/** 读取并校验 ${typeName}；不合格时抛出 SchemaError。 */\n${typeName} parse${typeName}(const nlohmann::json& json, const std::string& path);\nvoid to_json(nlohmann::json& json, const ${typeName}& value);`
  );
  const requestConstants = Object.keys(schema.requests ?? {}).map(
    (operation) => `/** 请求名：${name}.${operation}。 */\ninline constexpr std::string_view ${constant}_${snake(operation).toUpperCase()}_REQUEST = "${name}.${operation}";`
  );
  const eventConstants = Object.keys(schema.events ?? {}).map(
    (event) => `/** 事件名：${name}.${event}。 */\ninline constexpr std::string_view ${constant}_${snake(event).toUpperCase()}_EVENT = "${name}.${event}";`
  );
  const includeLines = [...includes].toSorted().map((target) => `#include "contracts/${target}/generated/${target}.hpp"`);
  const header = [
    CPP_HEADER(file),
    '#pragma once',
    '',
    '#include <cstdint>',
    '#include <optional>',
    '#include <string>',
    '#include <string_view>',
    '#include <vector>',
    '',
    '#include <nlohmann/json.hpp>',
    ...includeLines,
    '',
    `namespace ${namespace} {`,
    '',
    ...requestConstants,
    ...eventConstants,
    '',
    structs.join('\n\n'),
    '',
    declarations.join('\n\n'),
    '',
    `}  // namespace ${namespace}`,
    ''
  ].join('\n');
  const source = [
    CPP_HEADER(file),
    `#include "contracts/${name}/generated/${name}.hpp"`,
    '',
    '#include "shared/json/json-fields.hpp"',
    '',
    `namespace ${namespace} {`,
    '',
    'namespace json_fields = ::ember::shared::json_fields;',
    '',
    objects.map(([typeName, node]) => cppCodec(typeName, node, name)).join('\n\n'),
    '',
    `}  // namespace ${namespace}`,
    ''
  ].join('\n');
  return { header, source };
}

// ---------------------------------------------------------------- 输出

/** 内容有变化时才写入。 */
function writeIfChanged(path, content, written) {
  if (existsSync(path) && readFileSync(path, 'utf8') === content) return;
  mkdirSync(dirname(path), { recursive: true });
  writeFileSync(path, content, 'utf8');
  written.push(path);
}

/** 删除已不存在的模块的生成目录。 */
function removeStale(baseDir, modules, removed) {
  if (!existsSync(baseDir)) return;
  for (const name of readdirSync(baseDir)) {
    const generated = join(baseDir, name, 'generated');
    if (name === 'core-gateway' || modules.has(name) || !existsSync(generated)) continue;
    rmSync(generated, { recursive: true, force: true });
    removed.push(generated);
  }
}

/**
 * 生成全部代码。
 * @param {string} root 项目根目录。
 * @param {{ sdkDirectory?: string }} [options] sdkDirectory：调试 SDK 的目录（packages/ember-debug-sdk），目录存在时才写入它的 generated。
 * @returns {{ written: string[], removed: string[], modules: string[] }} 写入与删除的文件。
 */
export function generate(root, options = {}) {
  const modules = loadSchemas(root);
  const written = [];
  const removed = [];
  for (const module of modules) {
    writeIfChanged(join(root, 'src', 'contracts', module.name, 'generated', `${module.name}.ts`), generateTs(module), written);
    const cpp = generateCpp(module);
    writeIfChanged(join(root, 'core', 'src', 'contracts', module.name, 'generated', `${module.name}.hpp`), cpp.header, written);
    writeIfChanged(join(root, 'core', 'src', 'contracts', module.name, 'generated', `${module.name}.cpp`), cpp.source, written);
  }
  writeIfChanged(join(root, 'src', 'contracts', 'core-gateway', 'generated', 'core-api.ts'), generateTsIndex(modules), written);
  // 调试服务的接口描述文档（OpenRPC）：生成核心 debug 模块里的 C++ 常量，并写到调试 SDK 的 generated（帮助文档里的协议参考由 SDK 这份文档生成）。
  if (modules.some((module) => module.name === 'debug')) {
    const document = generateOpenRpc(modules);
    const cpp = generateOpenRpcCpp(document, CPP_HEADER('schema/*.schema.json'));
    writeIfChanged(join(root, 'core', 'src', 'contracts', 'debug', 'generated', 'openrpc.hpp'), cpp.header, written);
    writeIfChanged(join(root, 'core', 'src', 'contracts', 'debug', 'generated', 'openrpc.cpp'), cpp.source, written);
    // 调试 SDK（packages/ember-debug-sdk）：接口文档与类型写到它的 generated，目录不存在时跳过。
    const sdkGenerated = options.sdkDirectory === undefined ? undefined : join(options.sdkDirectory, 'generated');
    if (sdkGenerated !== undefined && existsSync(options.sdkDirectory)) {
      writeIfChanged(join(sdkGenerated, 'openrpc.json'), `${JSON.stringify(document, null, 2)}\n`, written);
      writeIfChanged(join(sdkGenerated, 'api.ts'), generateSdkTypes(document), written);
    }
  }
  const names = new Set(modules.map((module) => module.name));
  removeStale(join(root, 'src', 'contracts'), names, removed);
  removeStale(join(root, 'core', 'src', 'contracts'), names, removed);
  return { written, removed, modules: [...names] };
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  const root = resolve(dirname(fileURLToPath(import.meta.url)), '..', '..');
  try {
    const result = generate(root, { sdkDirectory: join(root, '..', 'packages', 'ember-debug-sdk') });
    console.log(`Schema 代码生成：${result.modules.length} 个模块，写入 ${result.written.length} 个文件，删除 ${result.removed.length} 个目录。`);
  } catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exit(1);
  }
}
