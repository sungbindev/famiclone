// ------------------------------------------------------------------------
// 名称：schema-validator.ts
// 说明：检查工具输入是否符合 package.json 里声明的参数结构（JSON Schema 的一个子集）。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：支持 type（含数组写法）、enum、minimum、maximum、minLength、items、properties、required、
//       additionalProperties；其他关键字忽略。返回全部问题，由调用者决定怎么展示。
// ------------------------------------------------------------------------

/** 参数结构。 */
export interface JsonSchema {
  readonly type?: string | readonly string[];
  readonly enum?: readonly unknown[];
  readonly minimum?: number;
  readonly maximum?: number;
  readonly minLength?: number;
  readonly items?: JsonSchema;
  readonly properties?: Readonly<Record<string, JsonSchema>>;
  readonly required?: readonly string[];
  readonly additionalProperties?: boolean | JsonSchema;
}

/** 判断值是否是某个 JSON Schema 类型。 */
function hasType(value: unknown, type: string): boolean {
  switch (type) {
    case 'string':
      return typeof value === 'string';
    case 'integer':
      return typeof value === 'number' && Number.isInteger(value);
    case 'number':
      return typeof value === 'number' && Number.isFinite(value);
    case 'boolean':
      return typeof value === 'boolean';
    case 'array':
      return Array.isArray(value);
    case 'object':
      return typeof value === 'object' && value !== null && !Array.isArray(value);
    case 'null':
      return value === null;
    default:
      return true;
  }
}

/**
 * 检查一个值。
 * @param schema 参数结构。
 * @param value 要检查的值。
 * @param label 值的名字，用于出错说明，如 edits[0]。
 * @returns 全部问题的说明；没有问题时为空数组。
 */
export function validateInput(schema: JsonSchema, value: unknown, label = '输入'): string[] {
  const problems: string[] = [];
  const types = schema.type === undefined ? undefined : typeof schema.type === 'string' ? [schema.type] : schema.type;
  if (types !== undefined && !types.some((type) => hasType(value, type))) {
    return [`${label} 的类型应为 ${types.join(' 或 ')}`];
  }
  if (schema.enum !== undefined && !schema.enum.includes(value)) {
    problems.push(`${label} 必须是 ${schema.enum.map((item) => JSON.stringify(item)).join('、')} 之一`);
  }
  if (typeof value === 'number') {
    if (schema.minimum !== undefined && value < schema.minimum) problems.push(`${label} 不能小于 ${schema.minimum}`);
    if (schema.maximum !== undefined && value > schema.maximum) problems.push(`${label} 不能大于 ${schema.maximum}`);
  }
  if (typeof value === 'string' && schema.minLength !== undefined && value.length < schema.minLength) {
    problems.push(`${label} 不能为空`);
  }
  if (Array.isArray(value) && schema.items !== undefined) {
    value.forEach((item, index) => problems.push(...validateInput(schema.items as JsonSchema, item, `${label}[${index}]`)));
  }
  if (hasType(value, 'object') && (schema.properties !== undefined || schema.required !== undefined || schema.additionalProperties !== undefined)) {
    const record = value as Record<string, unknown>;
    for (const name of schema.required ?? []) {
      if (record[name] === undefined) problems.push(`缺少参数 ${label === '输入' ? '' : `${label}.`}${name}`);
    }
    for (const [name, item] of Object.entries(record)) {
      const child = schema.properties?.[name];
      const path = label === '输入' ? name : `${label}.${name}`;
      if (child !== undefined) problems.push(...validateInput(child, item, path));
      else if (schema.additionalProperties === false) problems.push(`不认识的参数 ${path}`);
      else if (typeof schema.additionalProperties === 'object') problems.push(...validateInput(schema.additionalProperties, item, path));
    }
  }
  return problems;
}
