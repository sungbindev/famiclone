// ------------------------------------------------------------------------
// 名称：gasm.mjs
// 说明：步步高 GABBG 方言的 6502 汇编器。读取原厂源码（GBK 字节原样保留），
//       两遍汇编，输出字节映像、符号表与未定义符号，用于还原缺失的符号文件并与原厂二进制比对。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-12
// 备注：语法依据《步步高多媒体学生电脑 BIOS 系统 DOS 系统使用手册》第二章与原厂源码：
//       大小写无关；表达式没有优先级，从左到右计算；立即数前缀“<”取高字节、“>”取低字节
//       （与常见汇编器相反，源码里 RTI 地址与自修改跳转的写法均可印证）、“^”取页号；
//       前向引用的操作数按绝对寻址处理；orgrun/orgreal 由原厂 tvdef.asm 的宏定义，这里按
//       “运行地址与存放地址分离”的方式内置实现。
// ------------------------------------------------------------------------

import { readFileSync, existsSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { OPCODE_BY_FORM } from '../../sb2000/bios/tools/opcodes.mjs';

/** 汇编错误，带源文件与行号 */
export class GasmError extends Error {
  /**
   * @param {string} message 错误说明
   * @param {string} file 源文件路径
   * @param {number} line 行号（从 1 起）
   */
  constructor(message, file, line) {
    super(`${file}(${line}): ${message}`);
    this.file = file;
    this.line = line;
  }
}

const BRANCH_MNEMONICS = new Set(['BPL', 'BMI', 'BVC', 'BVS', 'BCC', 'BCS', 'BNE', 'BEQ']);
const SHIFT_MNEMONICS = new Set(['ASL', 'LSR', 'ROL', 'ROR']);
const DIRECTIVES = new Set(['ORG', 'EQU', 'DB', 'DW', 'DS', 'INCLUDE', 'END', 'PAGE', 'GLOBAL', 'COMMENT', 'ORGRUN', 'ORGREAL', '=']);
const MNEMONICS = new Set([...OPCODE_BY_FORM.keys()].map((key) => key.split(' ')[0]));

/**
 * 去掉行尾注释；引号内的分号保留。
 * @param {string} line 源码行
 * @returns {string} 去注释后的文本
 */
function stripComment(line) {
  let quote = '';
  for (let i = 0; i < line.length; i += 1) {
    const ch = line[i];
    if (quote) {
      if (ch === quote) quote = '';
    } else if (ch === '"' || ch === "'") {
      quote = ch;
    } else if (ch === ';') {
      return line.slice(0, i);
    }
  }
  return line;
}

/**
 * 按顶层逗号拆分（括号与引号内的逗号不拆）。
 * @param {string} text 待拆分文本
 * @returns {string[]} 各段，已去首尾空白
 */
function splitTopLevel(text) {
  const parts = [];
  let depth = 0;
  let quote = '';
  let start = 0;
  for (let i = 0; i < text.length; i += 1) {
    const ch = text[i];
    if (quote) {
      if (ch === quote) quote = '';
    } else if (ch === '"' || ch === "'") {
      quote = ch;
    } else if (ch === '(') {
      depth += 1;
    } else if (ch === ')') {
      depth -= 1;
    } else if (ch === ',' && depth === 0) {
      parts.push(text.slice(start, i).trim());
      start = i + 1;
    }
  }
  parts.push(text.slice(start).trim());
  return parts;
}

/**
 * 解析一行：标号、助记符或伪指令、操作数文本。
 * @param {string} raw 去注释后的行
 * @returns {{label: string|null, op: string|null, args: string}} 解析结果
 */
function parseLine(raw) {
  const text = raw.replace(/\s+$/, '');
  if (!text.trim()) return { label: null, op: null, args: '' };
  const atColumn0 = !/^\s/.test(text);
  let rest = text.trim();
  let label = null;
  const labelMatch = /^([A-Za-z_@][A-Za-z0-9_@]*)\s*:\s*/.exec(rest);
  if (labelMatch) {
    label = labelMatch[1];
    rest = rest.slice(labelMatch[0].length);
  } else {
    const wordMatch = /^([A-Za-z_@][A-Za-z0-9_@]*)(?:\s+|$)(.*)$/.exec(rest);
    if (wordMatch) {
      const word = wordMatch[1].toUpperCase();
      const next = /^([A-Za-z=]+)/.exec(wordMatch[2]);
      const nextWord = next ? next[1].toUpperCase() : '';
      const isOp = MNEMONICS.has(word) || DIRECTIVES.has(word);
      const nextIsDirective = nextWord === 'EQU' || nextWord === '=' || nextWord === 'DB' || nextWord === 'DW' || nextWord === 'DS';
      if ((!isOp && (atColumn0 || nextIsDirective)) || (isOp && nextIsDirective)) {
        label = wordMatch[1];
        rest = wordMatch[2].trim();
      }
    }
  }
  if (!rest) return { label, op: null, args: '' };
  const opMatch = /^([A-Za-z=]+)\s*(.*)$/.exec(rest);
  if (!opMatch) return { label, op: null, args: rest };
  return { label, op: opMatch[1].toUpperCase(), args: opMatch[2].trim() };
}

/**
 * 表达式求值器：无优先级，从左到右；开头的 <、>、^ 作用于整个表达式。
 */
class Evaluator {
  /**
   * @param {(name: string) => number|undefined} lookup 符号查找，未知返回 undefined
   * @param {number} pc 当前地址（“@”）
   */
  constructor(lookup, pc) {
    this.lookup = lookup;
    this.pc = pc;
    this.unresolved = [];
  }

  /**
   * 求值，遇到未知符号记入 unresolved 并按 0 计。
   * @param {string} text 表达式
   * @returns {number} 值
   */
  evaluate(text) {
    this.text = text.trim();
    this.pos = 0;
    let wrap = '';
    if ('<>^'.includes(this.text[0] ?? '')) {
      wrap = this.text[0];
      this.pos = 1;
    }
    const value = this.parseChain();
    this.skip();
    if (this.pos < this.text.length) throw new Error(`表达式多余内容：${this.text.slice(this.pos)}`);
    if (wrap === '<') return (value >> 8) & 0xff;
    if (wrap === '>') return value & 0xff;
    if (wrap === '^') return 0;
    return value;
  }

  skip() {
    while (this.pos < this.text.length && /\s/.test(this.text[this.pos])) this.pos += 1;
  }

  /** 读下一个二元运算符，没有则返回空串 */
  peekOperator() {
    this.skip();
    const ch = this.text[this.pos];
    if (ch === '+' || ch === '-' || ch === '*' || ch === '/') return ch;
    const word = /^(AND|OR|XOR)(?![A-Za-z0-9_])/i.exec(this.text.slice(this.pos));
    return word ? word[1].toUpperCase() : '';
  }

  parseChain() {
    let left = this.parseUnary();
    for (;;) {
      const op = this.peekOperator();
      if (!op) return left;
      this.pos += op.length;
      const right = this.parseUnary();
      if (op === '+') left += right;
      else if (op === '-') left -= right;
      else if (op === '*') left *= right;
      else if (op === '/') left = right === 0 ? 0 : Math.trunc(left / right);
      else if (op === 'AND') left &= right;
      else if (op === 'OR') left |= right;
      else left ^= right;
    }
  }

  parseUnary() {
    this.skip();
    const ch = this.text[this.pos];
    if (ch === '-') {
      this.pos += 1;
      return -this.parseUnary();
    }
    if (ch === '+') {
      this.pos += 1;
      return this.parseUnary();
    }
    if (ch === '<' || ch === '>') {
      this.pos += 1;
      const value = this.parseUnary();
      return ch === '<' ? (value >> 8) & 0xff : value & 0xff;
    }
    const not = /^NOT(?![A-Za-z0-9_])/i.exec(this.text.slice(this.pos));
    if (not) {
      this.pos += not[0].length;
      return ~this.parseUnary() & 0xffff;
    }
    return this.parsePrimary();
  }

  parsePrimary() {
    this.skip();
    const rest = this.text.slice(this.pos);
    let m;
    if (rest[0] === '(') {
      this.pos += 1;
      const value = this.parseChain();
      this.skip();
      if (this.text[this.pos] !== ')') throw new Error('缺少右括号');
      this.pos += 1;
      return value;
    }
    if ((m = /^\$([0-9A-Fa-f]+)/.exec(rest))) {
      this.pos += m[0].length;
      return parseInt(m[1], 16);
    }
    if ((m = /^[0-9]+/.exec(rest))) {
      this.pos += m[0].length;
      return parseInt(m[0], 10);
    }
    if ((m = /^'([^']{1,2})'/.exec(rest))) {
      this.pos += m[0].length;
      return [...m[1]].reduce((acc, ch) => (acc << 8) | ch.charCodeAt(0), 0);
    }
    if (rest[0] === '@' && !/^@[A-Za-z0-9_]/.test(rest)) {
      this.pos += 1;
      return this.pc;
    }
    if ((m = /^[A-Za-z_@][A-Za-z0-9_@]*/.exec(rest))) {
      this.pos += m[0].length;
      const value = this.lookup(m[0]);
      if (value === undefined) {
        this.unresolved.push(m[0]);
        return 0;
      }
      return value;
    }
    throw new Error(`无法识别的表达式：${rest}`);
  }
}

/**
 * 汇编结果。
 * @typedef {object} GasmResult
 * @property {Map<number, number>} memory 存放地址到字节的映射
 * @property {Map<string, number>} symbols 符号表（键为小写名）
 * @property {Map<string, string>} symbolNames 小写名到源码里首次出现的写法
 * @property {Map<string, {file: string, line: number, name: string}>} undefinedSymbols 未定义符号及首次引用位置
 * @property {Map<string, {file: string, line: number, name: string}>} unknownOps 不认识的助记符或宏及首次出现位置
 * @property {Array<{file: string, line: number, name: string}>} missingIncludes 找不到的包含文件
 * @property {Map<string, {file: string, line: number, name: string}>} duplicates 值不一致的重复定义
 * @property {Array<{file: string, line: number, address: number, bytes: number[], kind: string}>} listing 逐条清单，kind 为 ins（指令）或 data（数据）
 */

/**
 * 汇编器。
 */
export class Gasm {
  /**
   * @param {object} options 选项
   * @param {string[]} [options.includeDirs] 额外的包含目录
   * @param {number} [options.dsFill] ds 保留空间的填充字节
   */
  constructor(options = {}) {
    this.includeDirs = options.includeDirs ?? [];
    this.dsFill = options.dsFill ?? 0;
  }

  /**
   * 汇编入口文件。
   * @param {string} entry 入口源文件路径
   * @returns {GasmResult} 汇编结果
   */
  assemble(entry) {
    const statements = [];
    this.duplicates = new Map();
    this.collect(entry, statements, new Set());
    const symbols = new Map();
    const symbolNames = new Map();
    const sizes = new Map();
    this.pass(statements, symbols, symbolNames, sizes, 1);
    return this.pass(statements, symbols, symbolNames, sizes, 2);
  }

  /**
   * 读取源文件并展开 include、跳过 comment 块，得到语句序列。
   * @param {string} file 源文件路径
   * @param {object[]} out 输出语句
   * @param {Set<string>} stack 正在包含的文件，防止循环
   */
  collect(file, out, stack) {
    if (stack.has(file)) throw new GasmError('循环包含', file, 0);
    stack.add(file);
    const lines = readFileSync(file, 'latin1').split(/\r?\n/);
    let commentDelimiter = '';
    lines.forEach((rawLine, index) => {
      if (commentDelimiter) {
        if (rawLine.includes(commentDelimiter)) commentDelimiter = '';
        return;
      }
      const block = /^\s*comment\s+(\S)(.*)$/i.exec(rawLine);
      if (block) {
        if (!block[2].includes(block[1])) commentDelimiter = block[1];
        return;
      }
      let parsed;
      try {
        parsed = parseLine(stripComment(rawLine));
      } catch (error) {
        throw new GasmError(error.message, file, index + 1);
      }
      if (parsed.op === 'INCLUDE') {
        const name = parsed.args.replace(/^["']|["']$/g, '').trim();
        const found = [dirname(file), ...this.includeDirs].map((dir) => join(dir, name)).find((p) => existsSync(p));
        if (!found) {
          out.push({ file, line: index + 1, label: parsed.label, op: 'MISSING_INCLUDE', args: name });
          return;
        }
        this.collect(found, out, stack);
        return;
      }
      if (parsed.label || parsed.op) out.push({ file, line: index + 1, ...parsed });
    });
    stack.delete(file);
  }

  /**
   * 执行一遍汇编。
   * @param {object[]} statements 语句序列
   * @param {Map<string, number>} symbols 符号表（第二遍读第一遍的结果）
   * @param {Map<string, string>} symbolNames 符号原写法
   * @param {Map<object, string>} sizes 第一遍确定的寻址方式，第二遍沿用
   * @param {number} passNo 第几遍
   * @returns {GasmResult} 本遍结果
   */
  pass(statements, symbols, symbolNames, sizes, passNo) {
    const memory = new Map();
    const listing = [];
    const undefinedSymbols = new Map();
    const unknownOps = new Map();
    const missingIncludes = [];
    let pc = 0;
    let realPc = 0;
    let ended = false;

    for (const st of statements) {
      if (ended) break;
      const where = { file: st.file, line: st.line };
      const fail = (message) => {
        throw new GasmError(message, st.file, st.line);
      };
      const lookup = (name) => symbols.get(name.toLowerCase());
      const evaluate = (text) => {
        const ev = new Evaluator(lookup, pc);
        let value;
        try {
          value = ev.evaluate(text);
        } catch (error) {
          fail(error.message);
        }
        if (passNo === 2) {
          for (const name of ev.unresolved) {
            if (!undefinedSymbols.has(name.toLowerCase())) undefinedSymbols.set(name.toLowerCase(), { ...where, name });
          }
        }
        return { value, unresolved: ev.unresolved.length > 0 };
      };
      const define = (name, value) => {
        const key = name.toLowerCase();
        if (passNo === 1 && symbols.has(key) && symbols.get(key) !== value && !this.duplicates.has(key)) {
          this.duplicates.set(key, { ...where, name });
        }
        symbols.set(key, value);
        if (!symbolNames.has(key)) symbolNames.set(key, name);
      };
      const emit = (bytes, kind = 'data') => {
        listing.push({ ...where, address: pc, bytes, kind });
        for (const byte of bytes) {
          memory.set(realPc, byte & 0xff);
          pc += 1;
          realPc += 1;
        }
      };

      if (st.op === 'MISSING_INCLUDE') {
        missingIncludes.push({ ...where, name: st.args });
        continue;
      }
      if (st.op === 'EQU' || st.op === '=') {
        if (!st.label) fail('equ 缺少名字');
        define(st.label, evaluate(st.args).value);
        continue;
      }
      if (st.label) define(st.label, pc);
      if (!st.op) continue;

      switch (st.op) {
        case 'ORG':
          pc = evaluate(st.args).value;
          realPc = pc;
          break;
        case 'ORGRUN':
          pc = evaluate(st.args).value;
          break;
        case 'ORGREAL':
          pc = realPc;
          break;
        case 'END':
          ended = true;
          break;
        case 'PAGE':
        case 'GLOBAL':
          break;
        case 'DB': {
          const bytes = [];
          for (const item of splitTopLevel(st.args)) {
            const quoted = /^"(.*)"$/.exec(item);
            if (quoted) {
              for (const ch of quoted[1]) bytes.push(ch.charCodeAt(0));
            } else if (item) {
              bytes.push(evaluate(item).value & 0xff);
            }
          }
          emit(bytes);
          break;
        }
        case 'DW': {
          const bytes = [];
          for (const item of splitTopLevel(st.args)) {
            if (!item) continue;
            const value = evaluate(item).value;
            bytes.push(value & 0xff, (value >> 8) & 0xff);
          }
          emit(bytes);
          break;
        }
        case 'DS': {
          const [countText, fillText] = splitTopLevel(st.args);
          const count = evaluate(countText).value;
          const fill = fillText ? evaluate(fillText).value : this.dsFill;
          emit(new Array(Math.max(0, count)).fill(fill));
          break;
        }
        default:
          if (MNEMONICS.has(st.op)) {
            this.emitInstruction(st, evaluate, (bytes) => emit(bytes, 'ins'), () => pc, sizes, fail);
          } else if (!unknownOps.has(st.op)) {
            unknownOps.set(st.op, { ...where, name: st.op });
          }
      }
    }

    return { memory, symbols, symbolNames, undefinedSymbols, unknownOps, missingIncludes, duplicates: this.duplicates, listing };
  }

  /**
   * 汇编一条 6502 指令。
   * @param {object} st 语句
   * @param {Function} evaluate 求值函数
   * @param {Function} emit 写字节函数
   * @param {Function} getPc 取当前地址
   * @param {Map<object, string>} sizes 第一遍确定的寻址方式
   * @param {Function} fail 报错函数
   */
  emitInstruction(st, evaluate, emit, getPc, sizes, fail) {
    const mnemonic = st.op;
    const args = st.args.trim();
    const pick = (mode) => {
      const info = OPCODE_BY_FORM.get(`${mnemonic} ${mode}`);
      if (!info) fail(`${mnemonic} 不支持 ${mode} 寻址`);
      return info;
    };
    const absOrZp = (text, baseMode) => {
      const { value, unresolved } = evaluate(text);
      const prior = sizes.get(st);
      let mode = baseMode;
      const zpMode = { abs: 'zp', abx: 'zpx', aby: 'zpy' }[baseMode];
      const wantZp = prior !== undefined ? prior === zpMode : !unresolved && value >= 0 && value < 0x100;
      if (wantZp && OPCODE_BY_FORM.has(`${mnemonic} ${zpMode}`)) mode = zpMode;
      sizes.set(st, mode);
      const info = pick(mode);
      if (info.length === 2) emit([info.code, value & 0xff]);
      else emit([info.code, value & 0xff, (value >> 8) & 0xff]);
    };

    if (mnemonic === 'BRK' && !args) {
      emit([0x00]);
      return;
    }
    if (!args || (SHIFT_MNEMONICS.has(mnemonic) && /^a$/i.test(args))) {
      emit([pick(SHIFT_MNEMONICS.has(mnemonic) ? 'acc' : 'imp').code]);
      return;
    }
    if (args[0] === '#') {
      emit([pick('imm').code, evaluate(args.slice(1)).value & 0xff]);
      return;
    }
    let m;
    if ((m = /^\((.*)\)\s*,\s*[yY]$/.exec(args))) {
      emit([pick('izy').code, evaluate(m[1]).value & 0xff]);
      return;
    }
    if ((m = /^\((.*)\s*,\s*[xX]\)$/.exec(args))) {
      emit([pick('izx').code, evaluate(m[1]).value & 0xff]);
      return;
    }
    if (mnemonic === 'JMP' && (m = /^\((.*)\)$/.exec(args))) {
      const value = evaluate(m[1]).value;
      emit([pick('ind').code, value & 0xff, (value >> 8) & 0xff]);
      return;
    }
    if (BRANCH_MNEMONICS.has(mnemonic)) {
      const { value: target, unresolved } = evaluate(args);
      const offset = target - (getPc() + 2);
      if (!unresolved && (offset < -128 || offset > 127)) fail(`分支超出范围：${offset}`);
      emit([pick('rel').code, offset & 0xff]);
      return;
    }
    if ((m = /^(.*),\s*[xX]$/.exec(args))) {
      absOrZp(m[1], 'abx');
      return;
    }
    if ((m = /^(.*),\s*[yY]$/.exec(args))) {
      absOrZp(m[1], 'aby');
      return;
    }
    if (mnemonic === 'JMP' || mnemonic === 'JSR') {
      const value = evaluate(args).value;
      emit([pick('abs').code, value & 0xff, (value >> 8) & 0xff]);
      return;
    }
    absOrZp(args, 'abs');
  }
}
