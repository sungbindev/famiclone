/*
 * ------------------------------------------------------------------------
 * 名称：schema-types.ts
 * 说明：模块 Schema 的类型：数据类型、请求（参数、结果、错误码，可标记只由主进程发起或只经调试服务开放）、主进程处理的请求与事件（数据）的定义，写法是 JSON Schema 的子集。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：Schema 文件是 schema/<模块>.schema.json，由 tools/schema-gen 生成到各模块的 generated/ 目录；格式规则见 docs/06-构建与模块登记.md 第 2 节。
 * ------------------------------------------------------------------------
 */

/** 基本类型名。 */
export type SchemaPrimitive = 'string' | 'integer' | 'number' | 'boolean' | 'array' | 'object';

/** 一个 Schema 节点；没有 type 也没有 $ref 时是任意 JSON。 */
export interface SchemaNode {
  readonly type?: SchemaPrimitive | readonly [SchemaPrimitive, 'null'] | readonly ['null', SchemaPrimitive];
  readonly description?: string;
  readonly properties?: Readonly<Record<string, SchemaNode>>;
  readonly required?: readonly string[];
  readonly items?: SchemaNode;
  readonly minItems?: number;
  readonly maxItems?: number;
  readonly minLength?: number;
  readonly maxLength?: number;
  readonly enum?: readonly string[];
  readonly minimum?: number;
  readonly maximum?: number;
  readonly $ref?: string;
}

/** 一个请求的定义。 */
export interface RequestSchema {
  readonly description?: string;
  readonly params: SchemaNode;
  readonly result: SchemaNode;
  readonly errors?: readonly string[];
  /** 只能由主进程发起：入口层拒绝渲染进程发送，预加载脚本不暴露。 */
  readonly mainOnly?: boolean;
  /** 只经调试服务开放：入口层拒绝渲染进程与主进程发送，预加载脚本不暴露。 */
  readonly debugOnly?: boolean;
}

/** 一个事件的定义。 */
export interface EventSchema {
  readonly description?: string;
  readonly data: SchemaNode;
}

/** 一个模块的 Schema。 */
export interface ModuleSchema {
  readonly module: string;
  readonly types: Readonly<Record<string, SchemaNode>>;
  readonly requests: Readonly<Record<string, RequestSchema>>;
  /** 主进程自己处理、不进入核心的请求（如系统对话框）。 */
  readonly mainRequests?: Readonly<Record<string, RequestSchema>>;
  readonly events: Readonly<Record<string, EventSchema>>;
}
