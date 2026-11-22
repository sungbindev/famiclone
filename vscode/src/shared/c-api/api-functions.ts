// ------------------------------------------------------------------------
// 名称：api-functions.ts
// 说明：目标 C 接口文档：解析头文件里带文档注释的函数声明，按名称、模块、
//       系统调用号查询，格式化成文字。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：头文件的文档注释是 C 接口文档的唯一来源，查询、接口参考文档的生成与
//       接口检查都用这个解析器。注释写法见 targets/sb2000/include/sb2000-file.h：
//       首段是函数说明，@syscall 写对应的系统调用，@param 每个参数一行，
//       @return 写返回值。
// ------------------------------------------------------------------------

/** 头文件里的一个带文档注释的函数。 */
export interface ApiFunction {
  /** 函数名。 */
  readonly name: string;
  /** 所在头文件。 */
  readonly header: string;
  /** 声明（不含分号，已压成一行）。 */
  readonly signature: string;
  /** 返回类型文本。 */
  readonly returnType: string;
  /** 参数名。 */
  readonly params: readonly string[];
  /** 说明首行。 */
  readonly summary: string;
  /** 说明全文（@ 标签之前的部分，段落用换行连接）。 */
  readonly description: string;
  /** 文档里 @param 的参数名。 */
  readonly paramDocs: readonly string[];
  /** 每个 @param 的说明。 */
  readonly paramText: Readonly<Record<string, string>>;
  /** 文档里有 @return。 */
  readonly hasReturnDoc: boolean;
  /** @return 的说明。 */
  readonly returnText: string;
  /** 文档里 @syscall 标签的内容。 */
  readonly syscalls: readonly string[];
}

/** 查询的匹配方式。 */
export type ApiQueryKind = 'function' | 'module' | 'syscall' | 'text' | 'none';

/** 查询结果。 */
export interface ApiQueryResult {
  /** 匹配方式。 */
  readonly kind: ApiQueryKind;
  /** 匹配到的函数。 */
  readonly matches: readonly ApiFunction[];
}

/** 一个模块：头文件名与它的函数。 */
export interface ApiModule {
  /** 模块名，如 file。 */
  readonly name: string;
  /** 头文件名。 */
  readonly header: string;
  /** 模块里的函数。 */
  readonly functions: readonly ApiFunction[];
}

/** 声明前带文档注释的函数：注释、声明、函数名、参数列表。 */
const FUNCTION_PATTERN = /\/\*\*((?:(?!\*\/)[\s\S])*)\*\/\s*((?:(?!\/\*)[^;{}#])*?\b(sb_\w+)\s*\(([^)]*)\))\s*;/g;

/**
 * 解析头文件里带文档注释的函数声明。
 * @param text 头文件内容（换行为 \n）。
 * @param header 头文件名。
 * @returns 函数列表。
 */
export function parseHeaderFunctions(text: string, header: string): ApiFunction[] {
  const functions: ApiFunction[] = [];
  for (const match of text.matchAll(FUNCTION_PATTERN)) {
    const rawDoc = match[1] ?? '';
    const declaration = match[2] ?? '';
    const name = match[3] ?? '';
    const rawParams = match[4] ?? '';
    const docLines = rawDoc.split('\n').map((line) => line.replace(/^\s*\*?\s?/, '').trimEnd());
    const params =
      rawParams.trim() === '' || rawParams.trim() === 'void'
        ? []
        : rawParams.split(',').map((part) => /(\w+)\s*(?:\[[^\]]*\])?\s*$/.exec(part.trim())?.[1] ?? part.trim());
    const firstTag = docLines.findIndex((line) => line.startsWith('@'));
    const descriptionLines = (firstTag < 0 ? docLines : docLines.slice(0, firstTag)).join('\n').trim();
    const paramText: Record<string, string> = {};
    let returnText = '';
    let current: { kind: 'param'; name: string } | { kind: 'return' } | null = null;
    for (const line of docLines.slice(Math.max(firstTag, 0))) {
      const paramMatch = /^@param\s+(\w+)\s*(.*)$/.exec(line);
      if (paramMatch !== null) {
        const paramName = paramMatch[1] ?? '';
        paramText[paramName] = paramMatch[2] ?? '';
        current = { kind: 'param', name: paramName };
      } else if (line.startsWith('@return')) {
        returnText = line.slice('@return'.length).trim();
        current = { kind: 'return' };
      } else if (line.startsWith('@')) {
        current = null;
      } else if (current !== null && line.trim() !== '') {
        if (current.kind === 'param') paramText[current.name] = `${paramText[current.name] ?? ''} ${line.trim()}`;
        else returnText += ` ${line.trim()}`;
      }
    }
    functions.push({
      name,
      header,
      signature: declaration.replace(/\s+/g, ' ').trim(),
      returnType: declaration.slice(0, declaration.indexOf(name)).replace(/__fastcall__/g, '').trim(),
      params,
      summary: docLines.find((line) => line !== '' && !line.startsWith('@')) ?? '',
      description: descriptionLines,
      paramDocs: docLines.filter((line) => line.startsWith('@param ')).map((line) => line.split(/\s+/)[1] ?? ''),
      paramText,
      hasReturnDoc: docLines.some((line) => line.startsWith('@return')),
      returnText,
      syscalls: docLines.filter((line) => line.startsWith('@syscall ')).map((line) => line.slice('@syscall '.length).trim())
    });
  }
  return functions;
}

/**
 * 头文件名对应的模块名：sb2000-file.h 的模块是 file。
 * @param header 头文件名。
 * @returns 模块名。
 */
export function moduleOf(header: string): string {
  return header.replace(/^[a-z0-9]+-/, '').replace(/\.h$/, '');
}

/**
 * 按模块分组。
 * @param functions 函数列表。
 * @returns 模块列表，保持头文件顺序。
 */
export function groupByModule(functions: readonly ApiFunction[]): ApiModule[] {
  const groups = new Map<string, ApiFunction[]>();
  for (const fn of functions) {
    const key = moduleOf(fn.header);
    groups.set(key, [...(groups.get(key) ?? []), fn]);
  }
  return [...groups].map(([name, items]) => ({ name, header: items[0]?.header ?? '', functions: items }));
}

/**
 * 一个 @syscall 标签是否涉及指定的功能号（与子功能）。
 * 标签可以是“$7B/$09”“$30/$04-$07”“$3C、$3D”“$7B/$01、$7B/$06”这样的写法。
 * @param tag 标签内容。
 * @param number 功能号，两位十六进制小写。
 * @param sub 子功能号，两位十六进制小写；省略时只比功能号。
 * @returns 是否涉及。
 */
export function syscallMatches(tag: string, number: string, sub?: string): boolean {
  for (const part of tag.split('、')) {
    const match = /\$([0-9A-Fa-f]{2})(?![0-9A-Fa-f])(?:\/\$([0-9A-Fa-f]{2})(?:-\$([0-9A-Fa-f]{2}))?)?/.exec(part);
    if (match === null || (match[1] ?? '').toLowerCase() !== number) continue;
    if (sub === undefined || match[2] === undefined) return true;
    const low = Number.parseInt(match[2], 16);
    const high = match[3] === undefined ? low : Number.parseInt(match[3], 16);
    const wanted = Number.parseInt(sub, 16);
    if (wanted >= low && wanted <= high) return true;
  }
  return false;
}

/**
 * 查询函数。依次尝试：函数名完全相同；模块名（file 或 sb2000-file.h）；
 * 系统调用号（如 $7B、$7B/$09，不分大小写）；名称、说明里包含该词。
 * @param functions 函数列表。
 * @param query 查询词。
 * @returns 查询结果。
 */
export function queryApi(functions: readonly ApiFunction[], query: string): ApiQueryResult {
  const word = query.trim();
  const exact = functions.filter((fn) => fn.name === word);
  if (exact.length > 0) return { kind: 'function', matches: exact };
  const moduleName = moduleOf(word.includes('.h') ? word : `x-${word}.h`);
  const byModule = functions.filter((fn) => moduleOf(fn.header) === moduleName);
  if (byModule.length > 0) return { kind: 'module', matches: byModule };
  const lower = word.toLowerCase();
  if (/^\$?[0-9a-f]{2}(\/\$?[0-9a-f]{2})?$/.test(lower)) {
    const [number = '', sub] = lower.replace(/\$/g, '').split('/');
    const bySyscall = functions.filter((fn) => fn.syscalls.some((tag) => syscallMatches(tag, number, sub)));
    if (bySyscall.length > 0) return { kind: 'syscall', matches: bySyscall };
  }
  const byText = functions.filter((fn) => fn.name.includes(lower) || fn.summary.toLowerCase().includes(lower));
  return { kind: byText.length > 0 ? 'text' : 'none', matches: byText };
}

/**
 * 格式化一个函数的完整文档。
 * @param fn 函数。
 * @returns 多行文字。
 */
export function formatFunction(fn: ApiFunction): string {
  const lines = [`${fn.signature};`, `  所在：${fn.header}`];
  if (fn.syscalls.length > 0) lines.push(`  系统调用：${fn.syscalls.join('；')}`);
  lines.push('', ...fn.description.split('\n').map((line) => `  ${line}`.trimEnd()));
  if (fn.params.length > 0) {
    lines.push('', '  参数：');
    for (const param of fn.params) lines.push(`    ${param}  ${fn.paramText[param] ?? ''}`.trimEnd());
  }
  if (fn.hasReturnDoc) lines.push('', `  返回：${fn.returnText}`);
  return lines.join('\n');
}

/**
 * 格式化函数的一行摘要。
 * @param fn 函数。
 * @param width 名称列宽。
 * @returns 一行文字。
 */
export function formatSummary(fn: ApiFunction, width: number): string {
  return `  ${fn.name.padEnd(width)}  ${fn.summary}`;
}

/**
 * 格式化模块列表：每个模块的函数个数与头文件。
 * @param title 目标的显示名。
 * @param modules 模块列表。
 * @returns 多行文字。
 */
export function formatModules(title: string, modules: readonly ApiModule[]): string {
  const count = modules.reduce((sum, item) => sum + item.functions.length, 0);
  const lines = [`${title} 的 C 接口：${count} 个函数，${modules.length} 个模块`, ''];
  for (const item of modules) lines.push(`  ${item.name.padEnd(10)} ${String(item.functions.length).padStart(3)} 个函数  ${item.header}`);
  return lines.join('\n');
}

/**
 * 格式化一次查询的结果。
 * @param query 查询词。
 * @param result 查询结果。
 * @returns 多行文字。
 */
export function formatQueryResult(query: string, result: ApiQueryResult): string {
  if (result.kind === 'function') return result.matches.map(formatFunction).join('\n\n');
  const width = Math.max(...result.matches.map((fn) => fn.name.length));
  const titles: Record<'module' | 'syscall' | 'text' | 'none', string> = {
    module: `模块 ${query}`,
    syscall: `系统调用 ${query}`,
    text: `包含“${query}”`,
    none: query
  };
  return `${titles[result.kind]}：${result.matches.length} 个函数\n${result.matches.map((fn) => formatSummary(fn, width)).join('\n')}`;
}
