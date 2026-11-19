/*
 * ------------------------------------------------------------------------
 * 名称：css.mjs
 * 说明：令牌生成器用的 CSS 解析与求值：把样式文件拆成规则与声明，把自定义属性的取值求成带类型的结果（长度、数字、时间、颜色、列表、关键字）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：只支持组件库令牌用到的写法：var()（含备用值）、calc()、max()、min()、clamp()、rgb()、rgba()、color-mix(in srgb)、十六进制颜色、px、s、ms、%、无单位数字；
 *       遇到别的写法直接抛出异常，不跳过、不猜测。
 * ------------------------------------------------------------------------
 */

/**
 * 去掉 CSS 注释。
 * @param {string} text 样式文本。
 * @returns {string} 没有注释的文本。
 */
export function stripComments(text) {
  return text.replace(/\/\*[\s\S]*?\*\//g, '');
}

/**
 * 在括号与引号之外按分隔符拆分。
 * @param {string} text 文本。
 * @param {string} separator 单个分隔字符。
 * @returns {string[]} 各段，已去掉首尾空白，空段丢弃。
 */
export function splitTopLevel(text, separator) {
  const parts = [];
  let depth = 0;
  let quote = '';
  let start = 0;
  for (let index = 0; index < text.length; index += 1) {
    const char = text[index];
    if (quote) {
      if (char === quote) quote = '';
      continue;
    }
    if (char === '"' || char === "'") quote = char;
    else if (char === '(') depth += 1;
    else if (char === ')') depth -= 1;
    else if (char === separator && depth === 0) {
      parts.push(text.slice(start, index));
      start = index + 1;
    }
  }
  parts.push(text.slice(start));
  return parts.map((part) => part.trim()).filter((part) => part !== '');
}

/**
 * 解析样式文本里的规则（含 @media 内的规则）。
 * @param {string} source 样式文本。
 * @returns {{ selector: string, media: string, declarations: Map<string, string> }[]} 规则列表；@font-face 等没有选择器的块以 at 规则名作选择器。
 */
export function parseRules(source) {
  const text = stripComments(source);
  const rules = [];
  /**
   * 解析一段规则文本。
   * @param {string} body 文本。
   * @param {string} media 外层 @media 条件，没有时为空字符串。
   */
  const parseBody = (body, media) => {
    let index = 0;
    while (index < body.length) {
      const open = body.indexOf('{', index);
      const semicolon = body.indexOf(';', index);
      if (open === -1 || (semicolon !== -1 && semicolon < open)) {
        // 没有块的 at 规则（如 @import）。
        if (semicolon === -1) break;
        index = semicolon + 1;
        continue;
      }
      const prelude = body.slice(index, open).trim();
      let depth = 1;
      let cursor = open + 1;
      while (cursor < body.length && depth > 0) {
        if (body[cursor] === '{') depth += 1;
        else if (body[cursor] === '}') depth -= 1;
        cursor += 1;
      }
      const inner = body.slice(open + 1, cursor - 1);
      index = cursor;
      if (prelude.startsWith('@media')) parseBody(inner, prelude.slice('@media'.length).trim());
      else if (prelude.startsWith('@')) rules.push({ selector: prelude, media, declarations: parseDeclarations(inner) });
      else rules.push({ selector: prelude, media, declarations: parseDeclarations(inner) });
    }
  };
  parseBody(text, '');
  return rules;
}

/**
 * 解析声明块。
 * @param {string} block 块内文本。
 * @returns {Map<string, string>} 属性名到取值文本，后面的覆盖前面的。
 */
function parseDeclarations(block) {
  const declarations = new Map();
  for (const item of splitTopLevel(block, ';')) {
    const colon = item.indexOf(':');
    if (colon === -1) continue;
    declarations.set(item.slice(0, colon).trim(), item.slice(colon + 1).trim());
  }
  return declarations;
}

/** 词法单元的类型。 */
const NUMBER = /^[+-]?(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?/;

/**
 * 把取值文本切成节点。
 * @param {string} text 取值文本。
 * @returns {object[]} 节点列表；函数节点的参数是按逗号拆开的节点列表。
 */
export function parseValue(text) {
  const nodes = [];
  let index = 0;
  while (index < text.length) {
    const char = text[index];
    if (/\s/.test(char)) {
      index += 1;
      continue;
    }
    if (char === ',') {
      nodes.push({ t: 'comma' });
      index += 1;
      continue;
    }
    if (char === '(') {
      const end = matchParen(text, index);
      nodes.push({ t: 'group', args: splitArguments(text.slice(index + 1, end)) });
      index = end + 1;
      continue;
    }
    if (char === '"' || char === "'") {
      const end = text.indexOf(char, index + 1);
      if (end === -1) throw new Error(`字符串没有结束：${text}`);
      nodes.push({ t: 'str', v: text.slice(index + 1, end) });
      index = end + 1;
      continue;
    }
    if (char === '#') {
      const match = /^#[0-9a-fA-F]{3,8}/.exec(text.slice(index));
      if (!match) throw new Error(`不认识的颜色：${text}`);
      nodes.push({ t: 'hex', v: match[0] });
      index += match[0].length;
      continue;
    }
    const rest = text.slice(index);
    const number = NUMBER.exec(rest);
    // 数字后面直接跟字母或百分号是单位；“-” 开头的标识符（如 --x）不是数字。
    if (number && !/^-[a-zA-Z-]/.test(rest)) {
      const unitMatch = /^(%|[a-zA-Z]+)/.exec(rest.slice(number[0].length));
      const unit = unitMatch ? unitMatch[0] : '';
      nodes.push({ t: 'num', v: Number(number[0]), unit });
      index += number[0].length + unit.length;
      continue;
    }
    if (char === '+' || char === '*' || char === '/' || (char === '-' && !/^-{1,2}[a-zA-Z_]/.test(rest))) {
      nodes.push({ t: 'op', v: char });
      index += 1;
      continue;
    }
    const ident = /^-{0,2}[a-zA-Z_][a-zA-Z0-9_-]*/.exec(rest);
    if (!ident) throw new Error(`不认识的写法：${text.slice(index)}`);
    const name = ident[0];
    index += name.length;
    if (text[index] === '(') {
      const end = matchParen(text, index);
      nodes.push({ t: 'fn', name: name.toLowerCase(), args: splitArguments(text.slice(index + 1, end)) });
      index = end + 1;
    } else {
      nodes.push({ t: 'ident', v: name });
    }
  }
  return nodes;
}

/**
 * 找到与指定左括号配对的右括号。
 * @param {string} text 文本。
 * @param {number} open 左括号的位置。
 * @returns {number} 右括号的位置。
 */
function matchParen(text, open) {
  let depth = 0;
  for (let index = open; index < text.length; index += 1) {
    if (text[index] === '(') depth += 1;
    else if (text[index] === ')') {
      depth -= 1;
      if (depth === 0) return index;
    }
  }
  throw new Error(`括号没有配对：${text}`);
}

/**
 * 把函数参数按逗号拆开并各自解析。
 * @param {string} text 括号内的文本。
 * @returns {object[][]} 参数列表，每个参数是节点列表。
 */
function splitArguments(text) {
  return splitTopLevel(text, ',').map((argument) => parseValue(argument));
}

/** 求值失败时的异常，带上属性名便于定位。 */
export class CssValueError extends Error {}

/**
 * 求值环境：一个主题下全部自定义属性的取值。
 */
export class Evaluator {
  /**
   * @param {Map<string, string>} variables 自定义属性名到取值文本。
   */
  constructor(variables) {
    this.variables = variables;
    this.cache = new Map();
    this.resolving = new Set();
  }

  /**
   * 求一个自定义属性的取值。
   * @param {string} name 属性名，含 --。
   * @returns {object} 带类型的值。
   */
  resolve(name) {
    if (this.cache.has(name)) return this.cache.get(name);
    const raw = this.variables.get(name);
    if (raw === undefined) throw new CssValueError(`${name} 没有定义`);
    if (this.resolving.has(name)) throw new CssValueError(`${name} 的取值循环引用`);
    this.resolving.add(name);
    try {
      const value = this.evaluateText(raw);
      this.cache.set(name, value);
      return value;
    } catch (error) {
      throw error instanceof CssValueError ? new CssValueError(`${name}：${error.message}`) : error;
    } finally {
      this.resolving.delete(name);
    }
  }

  /**
   * 求一段取值文本。
   * @param {string} text 取值文本。
   * @returns {object} 带类型的值。
   */
  evaluateText(text) {
    return this.evaluateList(parseValue(text));
  }

  /**
   * 求节点列表：按逗号拆成逗号列表，每一段里多个值是空格列表。
   * @param {object[]} nodes 节点。
   * @returns {object} 值。
   */
  evaluateList(nodes) {
    const groups = [[]];
    for (const node of nodes) {
      if (node.t === 'comma') groups.push([]);
      else groups[groups.length - 1].push(node);
    }
    const values = groups.map((group) => this.evaluateSpaceList(group));
    return values.length === 1 ? values[0] : { k: 'list', sep: ',', items: values };
  }

  /**
   * 求空格分隔的节点。
   * @param {object[]} nodes 节点。
   * @returns {object} 值。
   */
  evaluateSpaceList(nodes) {
    const items = nodes.map((node) => this.evaluateNode(node));
    if (items.length === 1) return items[0];
    return { k: 'list', sep: ' ', items };
  }

  /**
   * 求单个节点。
   * @param {object} node 节点。
   * @returns {object} 值。
   */
  evaluateNode(node) {
    switch (node.t) {
      case 'num':
        return this.numberValue(node);
      case 'hex':
        return hexColor(node.v);
      case 'str':
        return { k: 'string', v: node.v };
      case 'ident':
        return node.v.toLowerCase() === 'transparent' ? { k: 'color', r: 0, g: 0, b: 0, a: 0 } : { k: 'ident', v: node.v };
      case 'group':
        return this.evaluateExpression(node.args[0]);
      case 'fn':
        return this.evaluateFunction(node);
      default:
        throw new CssValueError(`不认识的写法：${JSON.stringify(node)}`);
    }
  }

  /**
   * 数字节点转成值。
   * @param {{ v: number, unit: string }} node 节点。
   * @returns {object} 值。
   */
  numberValue(node) {
    const unit = node.unit.toLowerCase();
    if (unit === '') return { k: 'number', v: node.v };
    if (unit === 'px') return { k: 'length', v: node.v };
    if (unit === '%') return { k: 'percent', v: node.v };
    if (unit === 's') return { k: 'time', v: node.v * 1000 };
    if (unit === 'ms') return { k: 'time', v: node.v };
    if (unit === 'em') return { k: 'em', v: node.v };
    throw new CssValueError(`不支持的单位 ${node.unit}`);
  }

  /**
   * 求函数。
   * @param {{ name: string, args: object[][] }} node 函数节点。
   * @returns {object} 值。
   */
  evaluateFunction(node) {
    switch (node.name) {
      case 'var':
        return this.evaluateVar(node.args);
      case 'calc':
        return this.evaluateExpression(node.args[0]);
      case 'max':
      case 'min':
        return this.evaluateExtreme(node);
      case 'clamp':
        return this.evaluateClamp(node);
      case 'rgb':
      case 'rgba':
        return this.evaluateRgb(node.args);
      case 'color-mix':
        return this.evaluateColorMix(node.args);
      default:
        throw new CssValueError(`不支持的函数 ${node.name}()`);
    }
  }

  /**
   * 求 var()：先取自定义属性，没有时用备用值。
   * @param {object[][]} args 参数。
   * @returns {object} 值。
   */
  evaluateVar(args) {
    const nameNode = args[0]?.[0];
    if (!nameNode || nameNode.t !== 'ident' || !nameNode.v.startsWith('--')) throw new CssValueError('var() 的第一个参数必须是自定义属性名');
    if (this.variables.has(nameNode.v)) return this.resolve(nameNode.v);
    if (args.length < 2) throw new CssValueError(`${nameNode.v} 没有定义，也没有备用值`);
    return this.evaluateList(args.slice(1).flatMap((argument, index) => (index === 0 ? argument : [{ t: 'comma' }, ...argument])));
  }

  /**
   * 求 calc() 里的表达式（加减乘除与括号）。
   * @param {object[]} nodes 节点。
   * @returns {object} 值。
   */
  evaluateExpression(nodes) {
    let position = 0;
    const peek = () => nodes[position];
    const parseSum = () => {
      let left = parseProduct();
      while (peek()?.t === 'op' && (peek().v === '+' || peek().v === '-')) {
        const operator = nodes[position].v;
        position += 1;
        const right = parseProduct();
        left = combine(left, right, operator);
      }
      return left;
    };
    const parseProduct = () => {
      let left = parseUnary();
      while (peek()?.t === 'op' && (peek().v === '*' || peek().v === '/')) {
        const operator = nodes[position].v;
        position += 1;
        const right = parseUnary();
        left = combine(left, right, operator);
      }
      return left;
    };
    const parseUnary = () => {
      const node = peek();
      if (node === undefined) throw new CssValueError('表达式不完整');
      if (node.t === 'op' && node.v === '-') {
        position += 1;
        return combine({ k: 'number', v: 0 }, parseUnary(), '-');
      }
      position += 1;
      return this.evaluateNode(node);
    };
    const result = parseSum();
    if (position !== nodes.length) throw new CssValueError('表达式里有多余的内容');
    return result;
  }

  /**
   * 求 max() 与 min()。
   * @param {{ name: string, args: object[][] }} node 函数节点。
   * @returns {object} 值。
   */
  evaluateExtreme(node) {
    const values = node.args.map((argument) => this.evaluateExpression(argument));
    const kind = values[0].k;
    if (values.some((value) => value.k !== kind) || !['length', 'number', 'percent', 'time', 'em'].includes(kind)) throw new CssValueError(`${node.name}() 的参数类型必须相同`);
    const numbers = values.map((value) => value.v);
    return { k: kind, v: node.name === 'max' ? Math.max(...numbers) : Math.min(...numbers) };
  }

  /**
   * 求 clamp(最小, 首选, 最大)。
   * @param {{ args: object[][] }} node 函数节点。
   * @returns {object} 值。
   */
  evaluateClamp(node) {
    if (node.args.length !== 3) throw new CssValueError('clamp() 需要三个参数');
    const [low, preferred, high] = node.args.map((argument) => this.evaluateExpression(argument));
    if (low.k !== preferred.k || preferred.k !== high.k) throw new CssValueError('clamp() 的参数类型必须相同');
    return { k: preferred.k, v: Math.max(low.v, Math.min(preferred.v, high.v)) };
  }

  /**
   * 求 rgb() 与 rgba()，支持逗号写法与空格加斜杠写法。
   * @param {object[][]} args 参数。
   * @returns {object} 颜色。
   */
  evaluateRgb(args) {
    const flat = args.length === 1 ? args[0].filter((node) => !(node.t === 'op' && node.v === '/')) : args.map((argument) => argument[0]);
    if (flat.length !== 3 && flat.length !== 4) throw new CssValueError('rgb() 需要三个或四个分量');
    const channel = (node) => {
      const value = this.evaluateNode(node);
      if (value.k === 'percent') return Math.round((value.v / 100) * 255);
      if (value.k !== 'number') throw new CssValueError('rgb() 的分量必须是数字');
      return value.v;
    };
    const alphaNode = flat[3];
    let alpha = 1;
    if (alphaNode) {
      const value = this.evaluateNode(alphaNode);
      alpha = value.k === 'percent' ? value.v / 100 : value.v;
    }
    return { k: 'color', r: channel(flat[0]), g: channel(flat[1]), b: channel(flat[2]), a: alpha };
  }

  /**
   * 求 color-mix(in srgb, 颜色1 [比例], 颜色2 [比例])，按预乘透明度的 sRGB 插值。
   * @param {object[][]} args 参数。
   * @returns {object} 颜色。
   */
  evaluateColorMix(args) {
    if (args.length !== 3 || args[0].map((node) => node.v).join(' ') !== 'in srgb') throw new CssValueError('color-mix() 只支持 in srgb 与两个颜色');
    const parse = (argument) => {
      const values = argument.map((node) => this.evaluateNode(node));
      const percentValue = values.find((value) => value.k === 'percent');
      const colorValues = values.filter((value) => value.k !== 'percent');
      if (colorValues.length !== 1 || colorValues[0].k !== 'color') throw new CssValueError('color-mix() 的参数必须是颜色加可选的比例');
      return { color: colorValues[0], percent: percentValue ? percentValue.v : null };
    };
    const first = parse(args[1]);
    const second = parse(args[2]);
    let p1 = first.percent;
    let p2 = second.percent;
    if (p1 === null && p2 === null) {
      p1 = 50;
      p2 = 50;
    } else if (p1 === null) p1 = 100 - p2;
    else if (p2 === null) p2 = 100 - p1;
    const total = p1 + p2;
    const w1 = p1 / total;
    const w2 = p2 / total;
    const alpha = first.color.a * w1 + second.color.a * w2;
    if (alpha === 0) return { k: 'color', r: 0, g: 0, b: 0, a: 0 };
    const mix = (c1, c2) => (c1 * first.color.a * w1 + c2 * second.color.a * w2) / alpha;
    // 比例之和小于 100% 时整体透明度按比例之和缩小。
    const scale = Math.min(total, 100) / 100;
    return { k: 'color', r: mix(first.color.r, second.color.r), g: mix(first.color.g, second.color.g), b: mix(first.color.b, second.color.b), a: alpha * scale };
  }
}

/**
 * 十六进制颜色转成颜色值。
 * @param {string} text 如 #rgb、#rrggbb、#rrggbbaa。
 * @returns {object} 颜色。
 */
function hexColor(text) {
  let digits = text.slice(1);
  if (digits.length === 3 || digits.length === 4) digits = [...digits].map((digit) => digit + digit).join('');
  if (digits.length !== 6 && digits.length !== 8) throw new CssValueError(`不认识的颜色 ${text}`);
  const channel = (offset) => Number.parseInt(digits.slice(offset, offset + 2), 16);
  return { k: 'color', r: channel(0), g: channel(2), b: channel(4), a: digits.length === 8 ? channel(6) / 255 : 1 };
}

/**
 * 两个值做加减乘除。
 * @param {object} left 左值。
 * @param {object} right 右值。
 * @param {string} operator 运算符。
 * @returns {object} 结果。
 */
function combine(left, right, operator) {
  const numeric = ['length', 'number', 'percent', 'time', 'em'];
  if (!numeric.includes(left.k) || !numeric.includes(right.k)) throw new CssValueError('只能对长度、数字、百分比与时间做运算');
  if (operator === '+' || operator === '-') {
    if (left.k !== right.k) throw new CssValueError(`不能把 ${left.k} 与 ${right.k} 相加减`);
    return { k: left.k, v: operator === '+' ? left.v + right.v : left.v - right.v };
  }
  if (operator === '*') {
    if (left.k !== 'number' && right.k !== 'number') throw new CssValueError('相乘的两个值里必须有一个是数字');
    return { k: left.k === 'number' ? right.k : left.k, v: left.v * right.v };
  }
  if (right.k === 'number') return { k: left.k, v: left.v / right.v };
  if (left.k === right.k) return { k: 'number', v: left.v / right.v };
  throw new CssValueError('除数必须是数字，或与被除数同类型');
}
