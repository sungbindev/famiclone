// ------------------------------------------------------------------------
// 名称：chat-contributions.test.ts
// 说明：检查扩展声明的聊天指令、技能与自定义智能体文件：路径存在、前置信息完整、
//       智能体引用的工具与清单里的语言模型工具一一对应，技能文字只提到存在的工具。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：只读文本，不加载 VS Code；工具引用的写法（扩展标识加斜杠加工具名）
//       尚未在真实 VS Code 里核对。
// ------------------------------------------------------------------------

import { existsSync, readdirSync, readFileSync, statSync } from 'node:fs';
import * as path from 'node:path';
import { describe, expect, it } from 'vitest';

const ROOT = path.resolve(__dirname, '..');
const EXTENSION_ID = 'sungbin.fcdev';
/** 智能体可以直接使用的 VS Code 内置工具。 */
const BUILT_IN_TOOLS = ['read', 'search', 'todo'];

interface Manifest {
  contributes: {
    chatInstructions: Array<{ path: string }>;
    chatSkills: Array<{ path: string }>;
    chatAgents: Array<{ path: string }>;
    languageModelTools: Array<{ toolReferenceName: string }>;
  };
}

const manifest = JSON.parse(readFileSync(path.join(ROOT, 'package.json'), 'utf8')) as Manifest;

/** 读取清单里声明的文件，路径相对扩展根目录。 */
function readDeclared(declared: string): string {
  return readFileSync(path.join(ROOT, declared), 'utf8');
}

/** 取出 Markdown 前置信息中的单行字段与 tools 列表。 */
function frontmatterOf(text: string): { fields: Record<string, string>; tools: string[] } {
  const match = /^---\r?\n([\s\S]*?)\r?\n---/.exec(text);
  const lines = (match?.[1] ?? '').split(/\r?\n/);
  const fields: Record<string, string> = {};
  const tools: string[] = [];
  let inTools = false;
  for (const line of lines) {
    const item = /^\s+-\s+(.+)$/.exec(line);
    if (inTools && item) {
      tools.push((item[1] ?? '').trim());
      continue;
    }
    const field = /^([\w-]+):\s*(.*)$/.exec(line);
    inTools = field?.[1] === 'tools';
    if (field) fields[field[1] ?? ''] = (field[2] ?? '').replace(/^'(.*)'$/, '$1');
  }
  return { fields, tools };
}

/** 递归读取目录下全部 Markdown 文件的文字。 */
function readMarkdown(directory: string): string {
  return readdirSync(directory)
    .map((name) => {
      const full = path.join(directory, name);
      if (statSync(full).isDirectory()) return readMarkdown(full);
      return name.endsWith('.md') ? readFileSync(full, 'utf8') : '';
    })
    .join('\n');
}

describe('聊天指令、技能与智能体的声明', () => {
  const toolNames = manifest.contributes.languageModelTools.map((tool) => tool.toolReferenceName);

  it('声明的文件都存在，且打进扩展包（不在忽略清单里）', () => {
    const declared = [...manifest.contributes.chatInstructions, ...manifest.contributes.chatSkills, ...manifest.contributes.chatAgents];
    expect(declared.length).toBe(3);
    for (const item of declared) expect(existsSync(path.join(ROOT, item.path)), item.path).toBe(true);
    expect(readFileSync(path.join(ROOT, '.vscodeignore'), 'utf8')).not.toMatch(/^chat/m);
  });

  it('指令文件有名称、说明与适用范围', () => {
    for (const item of manifest.contributes.chatInstructions) {
      const { fields } = frontmatterOf(readDeclared(item.path));
      expect(fields.name, item.path).toBeTruthy();
      expect(fields.description, item.path).toBeTruthy();
      expect(fields.applyTo, item.path).toBeTruthy();
    }
  });

  it('技能所在目录名与技能名一致，引用的文件存在', () => {
    for (const item of manifest.contributes.chatSkills) {
      const text = readDeclared(item.path);
      const { fields } = frontmatterOf(text);
      expect(fields.name).toBe(path.basename(path.dirname(path.join(ROOT, item.path))));
      expect(fields.description).toBeTruthy();
      for (const link of text.matchAll(/\]\((references\/[^)#]+)\)/g)) {
        expect(existsSync(path.join(path.dirname(path.join(ROOT, item.path)), link[1] ?? '')), link[1]).toBe(true);
      }
    }
  });

  it('智能体的工具是内置工具加上扩展的全部语言模型工具，没有多也没有少', () => {
    for (const item of manifest.contributes.chatAgents) {
      const { fields, tools } = frontmatterOf(readDeclared(item.path));
      expect(fields.name, item.path).toBeTruthy();
      expect(fields.description, item.path).toBeTruthy();
      const own = tools.filter((tool) => !BUILT_IN_TOOLS.includes(tool));
      expect(tools.filter((tool) => BUILT_IN_TOOLS.includes(tool)).toSorted()).toEqual(BUILT_IN_TOOLS.toSorted());
      expect(own.toSorted()).toEqual(toolNames.map((name) => `${EXTENSION_ID}/${name}`).toSorted());
    }
  });

  it('技能与指令里提到的工具名都存在', () => {
    const text = [
      ...manifest.contributes.chatSkills.map((item) => readMarkdown(path.dirname(path.join(ROOT, item.path)))),
      ...manifest.contributes.chatInstructions.map((item) => readDeclared(item.path))
    ].join('\n');
    const mentioned = new Set([...text.matchAll(/\b((?:disasm|emu)_[a-z_]+)\b/g)].map((match) => match[1] ?? ''));
    expect(mentioned.size).toBeGreaterThan(0);
    expect([...mentioned].filter((name) => !toolNames.includes(name))).toEqual([]);
  });
});
