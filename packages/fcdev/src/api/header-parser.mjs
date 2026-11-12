// ------------------------------------------------------------------------
// 名称：header-parser.mjs
// 说明：解析目标头文件里带文档注释的函数声明：说明、@param、@return、@syscall。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：头文件的文档注释是 C 接口文档的唯一来源，fcdev api 命令、API 参考文档的生成与接口检查都用这个解析器。
//       注释写法见 targets/sb2000/include/sb2000-file.h：首段是函数说明，@syscall 写对应的系统调用，@param 每个参数一行，@return 写返回值。
// ------------------------------------------------------------------------

/**
 * @typedef {object} ApiFunction
 * @property {string} name 函数名
 * @property {string} header 所在头文件
 * @property {string} signature 声明（不含分号，已压成一行）
 * @property {string} returnType 返回类型文本
 * @property {string[]} params 参数名
 * @property {string} summary 说明首行
 * @property {string} description 说明全文（@ 标签之前的部分，段落用换行连接）
 * @property {string[]} paramDocs 文档里 @param 的参数名
 * @property {Record<string, string>} paramText 每个 @param 的说明
 * @property {boolean} hasReturnDoc 文档里有 @return
 * @property {string} returnText @return 的说明
 * @property {string[]} syscalls 文档里 @syscall 标签的内容
 */

/**
 * 解析头文件里带文档注释的函数声明。
 * @param {string} text 头文件内容
 * @param {string} header 头文件名
 * @returns {ApiFunction[]} 函数列表
 */
export function parseHeaderFunctions(text, header) {
  const pattern = /\/\*\*((?:(?!\*\/)[\s\S])*)\*\/\s*((?:(?!\/\*)[^;{}#])*?\b(sb_\w+)\s*\(([^)]*)\))\s*;/g;
  const functions = [];
  for (const match of text.matchAll(pattern)) {
    const [, rawDoc, declaration, name, rawParams] = match;
    const docLines = rawDoc.split('\n').map((line) => line.replace(/^\s*\*?\s?/, '').trimEnd());
    const params = rawParams.trim() === '' || rawParams.trim() === 'void'
      ? []
      : rawParams.split(',').map((part) => /(\w+)\s*(?:\[[^\]]*\])?\s*$/.exec(part.trim())?.[1] ?? part.trim());
    const firstTag = docLines.findIndex((line) => line.startsWith('@'));
    const descriptionLines = (firstTag < 0 ? docLines : docLines.slice(0, firstTag)).join('\n').trim();
    const paramText = {};
    let returnText = '';
    let current = null;
    for (const line of docLines.slice(Math.max(firstTag, 0))) {
      const paramMatch = /^@param\s+(\w+)\s*(.*)$/.exec(line);
      if (paramMatch) {
        paramText[paramMatch[1]] = paramMatch[2];
        current = { kind: 'param', name: paramMatch[1] };
      } else if (line.startsWith('@return')) {
        returnText = line.slice('@return'.length).trim();
        current = { kind: 'return' };
      } else if (line.startsWith('@')) {
        current = null;
      } else if (current && line.trim() !== '') {
        if (current.kind === 'param') paramText[current.name] += ` ${line.trim()}`;
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
      paramDocs: docLines.filter((line) => line.startsWith('@param ')).map((line) => line.split(/\s+/)[1]),
      paramText,
      hasReturnDoc: docLines.some((line) => line.startsWith('@return')),
      returnText,
      syscalls: docLines.filter((line) => line.startsWith('@syscall ')).map((line) => line.slice('@syscall '.length).trim()),
    });
  }
  return functions;
}
