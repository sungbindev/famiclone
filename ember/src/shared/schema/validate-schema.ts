/*
 * ------------------------------------------------------------------------
 * 名称：validate-schema.ts
 * 说明：按模块 Schema 校验数据：类型、必填、长度、范围、枚举与数组项数，跨模块的类型引用按模块名查找。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-13
 * 备注：主进程的入口层用它校验渲染进程发来的请求参数；C++ 一侧按同一份 Schema 生成的代码再做一次结构校验。
 * ------------------------------------------------------------------------
 */

import type { ModuleSchema, SchemaNode } from './schema-types';

/** 校验结果：通过时 errors 为空。 */
export interface SchemaValidation {
  readonly valid: boolean;
  readonly errors: readonly string[];
}

/** 按模块名查找 Schema。 */
type SchemaLookup = (module: string) => ModuleSchema | undefined;

/** 节点的类型与是否可空。 */
function typeOf(node: SchemaNode): { type: string | undefined; nullable: boolean } {
  const { type } = node;
  if (Array.isArray(type)) return { type: type.find((item) => item !== 'null'), nullable: type.includes('null') };
  return { type: type as string | undefined, nullable: false };
}

/** 解析类型引用。 */
function resolveRef(ref: string, current: ModuleSchema, lookup: SchemaLookup): { node: SchemaNode; module: ModuleSchema } {
  const match = /^(?:([a-z][a-z0-9-]*))?#\/types\/([A-Z][A-Za-z0-9]*)$/.exec(ref);
  const module = match?.[1] ? lookup(match[1]) : current;
  const node = match?.[2] ? module?.types[match[2]] : undefined;
  if (module === undefined || node === undefined) throw new Error(`Schema 引用不存在：${ref}`);
  return { node, module };
}

/** 校验一个值，错误追加到 errors。 */
function check(node: SchemaNode, value: unknown, path: string, current: ModuleSchema, lookup: SchemaLookup, errors: string[]): void {
  if (node.$ref !== undefined) {
    const target = resolveRef(node.$ref, current, lookup);
    check(target.node, value, path, target.module, lookup, errors);
    return;
  }
  const { type, nullable } = typeOf(node);
  if (type === undefined) return;
  if (value === null) {
    if (!nullable) errors.push(`${path} 不能为空`);
    return;
  }
  switch (type) {
    case 'string': {
      if (typeof value !== 'string') {
        errors.push(`${path} 应为字符串`);
        return;
      }
      if (node.minLength !== undefined && value.length < node.minLength) errors.push(`${path} 至少 ${node.minLength} 个字符`);
      if (node.maxLength !== undefined && value.length > node.maxLength) errors.push(`${path} 最多 ${node.maxLength} 个字符`);
      if (node.enum !== undefined && !node.enum.includes(value)) errors.push(`${path} 只能是 ${node.enum.join('、')}`);
      return;
    }
    case 'integer':
    case 'number': {
      if (typeof value !== 'number' || !Number.isFinite(value) || (type === 'integer' && !Number.isInteger(value))) {
        errors.push(`${path} 应为${type === 'integer' ? '整数' : '数字'}`);
        return;
      }
      if (node.minimum !== undefined && value < node.minimum) errors.push(`${path} 不能小于 ${node.minimum}`);
      if (node.maximum !== undefined && value > node.maximum) errors.push(`${path} 不能大于 ${node.maximum}`);
      return;
    }
    case 'boolean':
      if (typeof value !== 'boolean') errors.push(`${path} 应为布尔值`);
      return;
    case 'array': {
      if (!Array.isArray(value)) {
        errors.push(`${path} 应为数组`);
        return;
      }
      if (node.minItems !== undefined && value.length < node.minItems) errors.push(`${path} 至少 ${node.minItems} 项`);
      if (node.maxItems !== undefined && value.length > node.maxItems) errors.push(`${path} 最多 ${node.maxItems} 项`);
      if (node.items !== undefined) value.forEach((item, index) => check(node.items as SchemaNode, item, `${path}[${index}]`, current, lookup, errors));
      return;
    }
    case 'object': {
      if (typeof value !== 'object' || Array.isArray(value)) {
        errors.push(`${path} 应为对象`);
        return;
      }
      const record = value as Record<string, unknown>;
      const properties = node.properties ?? {};
      for (const name of node.required ?? []) if (!(name in record) || record[name] === undefined) errors.push(`${path}.${name} 缺少`);
      for (const [name, child] of Object.entries(record)) {
        const definition = properties[name];
        if (definition === undefined) errors.push(`${path}.${name} 是不认识的字段`);
        else if (child !== undefined) check(definition, child, `${path}.${name}`, current, lookup, errors);
      }
      return;
    }
    default:
      errors.push(`${path} 的 Schema 类型无效`);
  }
}

/**
 * 按 Schema 校验数据。
 * @param node 要校验的节点。
 * @param value 数据。
 * @param module 节点所在的模块。
 * @param schemas 全部模块的 Schema，用于跨模块引用。
 * @param path 数据在错误信息中的名称，默认 params。
 * @returns 校验结果。
 */
export function validateSchema(node: SchemaNode, value: unknown, module: ModuleSchema, schemas: readonly ModuleSchema[], path = 'params'): SchemaValidation {
  const errors: string[] = [];
  const lookup: SchemaLookup = (name) => schemas.find((schema) => schema.module === name);
  check(node, value, path, module, lookup, errors);
  return { valid: errors.length === 0, errors };
}
