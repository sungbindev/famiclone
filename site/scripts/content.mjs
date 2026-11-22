/*
 * ------------------------------------------------------------------------
 * 名称：content.mjs
 * 说明：站点的内容层：读取 Ember 帮助文档与 fcdev 手册的源文件，渲染成页面的 HTML，整理导航、页内目录与上一页、下一页。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：源文件只读，不改 ember/help 的内容；页面之间的链接在构建时改写成相对路径，指向不存在的页面时直接报错。
 *       代码块用 Shiki 在构建时着色，亮色与深色两套颜色都写进页面，由主题类决定显示哪一套。
 * ------------------------------------------------------------------------
 */

import { dirname, join, posix, resolve } from 'node:path';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import MarkdownIt from 'markdown-it';
import { createHighlighter } from 'shiki';
import { DOC_TOPICS, groupTopics } from '../../ember/tools/build/import-fcdev-docs.mjs';

const here = dirname(fileURLToPath(import.meta.url));
/** 仓库根目录（famiclone）。 */
export const REPO_ROOT = resolve(here, '..', '..');
/** 站点输出目录名（相对仓库根目录）：其余页面与资源都放在这里。 */
export const SITE_DIR = 'site';
/** 帮助文档源目录。 */
export const HELP_ROOT = resolve(REPO_ROOT, 'ember', 'help');
/** 代码高亮用的语言；文档里没有登记的语言按纯文本显示。 */
const LANGS = ['c', 'cpp', 'asm', 'json', 'javascript', 'typescript', 'powershell', 'bash', 'ini', 'markdown', 'yaml', 'html', 'css', 'xml', 'diff'];
/** 文档里的语言名与高亮语言名的对应（只列需要改名的）。 */
const LANG_ALIASES = { js: 'javascript', ts: 'typescript', sh: 'bash', shell: 'bash', ps1: 'powershell', ca65: 'asm', txt: 'text', plaintext: 'text' };
/** 页内目录只列这两级标题。 */
const TOC_LEVELS = [2, 3];

/** 手写帮助页的侧边栏（与 ember/help/.vitepress/config.mts 的分组、顺序一致）。 */
const MANUAL_PAGES = [
  ['index', '简介'],
  ['start', '快速开始'],
  ['window', '窗口、菜单与快捷键'],
  ['machines', '机型与 ROM'],
  ['media', '卡带、软盘、磁带与打印机'],
  ['input', '键盘、手柄与鼠标'],
  ['display-sound', '画面与声音'],
  ['plugins', 'mapper 插件'],
  ['preferences', '首选项'],
  ['faq', '常见问题'],
  ['changelog', '更新记录']
];
const PLUGIN_PAGES = [
  ['index', '概述'],
  ['quickstart', '第一个插件'],
  ['package', '插件包与安装'],
  ['manifest', '清单：基本字段与匹配'],
  ['memory', '清单：存储区、窗口与名称表'],
  ['registers', '清单：寄存器与状态'],
  ['script', '脚本接口'],
  ['recipes', '常见写法'],
  ['debugging', '测试与排错']
];
const DEBUG_PAGES = [
  ['index', '概述'],
  ['protocol', '连接与报文'],
  ['control', '运行控制与典型流程'],
  ['clients', '客户端工具']
];

/**
 * 调试接口的模块页（api/ 下），顺序取自总览页里的模块列表。
 * @returns {[string, string][]} 文件名（不含扩展名）与侧边栏文字
 */
function apiModulePages() {
  const overview = readFileSync(join(HELP_ROOT, 'api', 'index.md'), 'utf8');
  const modules = [...overview.matchAll(/^### \[([a-z0-9-]+)\]\(\1\.md\)$/gm)].map((match) => match[1]);
  if (modules.length === 0) throw new Error('api/index.md 里找不到模块列表：先运行 help 生成');
  return [['index', '总览'], ...modules.map((name) => [name, name])];
}

/**
 * 站点的分区（顺序即侧边栏顺序）。页面 id 是源文件相对 ember/help 的路径，去掉 .md。
 * @returns {Section[]} 分区清单
 */
function buildSections() {
  const pagesOf = (dir, list) => list.map(([name, label]) => ({ id: `${dir}/${name}`, src: `${dir}/${name}.md`, label }));
  const fcdev = groupTopics(DOC_TOPICS).map((group) => ({ title: group.title, pages: group.topics.map((topic) => ({ id: `fcdev/${topic.topic}`, src: `fcdev/${topic.topic}.md`, label: topic.title })) }));
  return [
    { id: 'manual', title: '使用说明', summary: '打开卡带、添加机型与 ROM、键位与手柄、画面声音，以及常见问题。', groups: [{ title: null, pages: pagesOf('manual', MANUAL_PAGES) }] },
    { id: 'plugin', title: '插件开发手册', summary: '为 NES 卡带编写 mapper 插件：清单、存储区、寄存器、脚本接口与测试排错。', groups: [{ title: null, pages: pagesOf('plugin', PLUGIN_PAGES) }] },
    {
      id: 'debug',
      title: '调试接口手册',
      summary: '通过 WebSocket JSON-RPC 控制 Ember 的运行、读写内存、断点与截图，附完整的请求与事件说明。',
      groups: [
        { title: '调试服务', pages: pagesOf('debug', DEBUG_PAGES) },
        { title: '请求与事件', pages: pagesOf('api', apiModulePages()) }
      ]
    },
    {
      id: 'fcdev',
      title: 'fcdev 开发手册',
      summary: '用 C 或 ca65 汇编开发 NES 与 SB2000 程序：构建、运行、调试、反汇编，以及系统调用与硬件参考。',
      groups: [{ title: null, pages: [{ id: 'fcdev/index', src: 'fcdev/index.md', label: '概述' }] }, ...fcdev]
    }
  ];
}

/**
 * 两个页面输出路径之间的相对链接。
 * @param {string} fromOut 当前页面的输出路径（相对仓库根目录，用 / 分隔）
 * @param {string} toOut 目标页面的输出路径
 * @returns {string} 相对链接
 */
export function hrefFrom(fromOut, toOut) {
  const relativePath = posix.relative(posix.dirname(fromOut), toOut);
  return relativePath === '' ? posix.basename(toOut) : relativePath;
}

/**
 * 页面的输出路径：分区首页为分区目录下的 index.html，其余为同名的 html。
 * @param {string} id 页面 id
 * @returns {string} 输出路径
 */
function outputOf(id) {
  return `${SITE_DIR}/${id}.html`;
}

/**
 * 按 Shiki 语言名取高亮语言，认不出的按纯文本。
 * @param {string} info 代码块的语言标记
 * @returns {string} 高亮语言名
 */
function languageOf(info) {
  const name = LANG_ALIASES[info] ?? info;
  return LANGS.includes(name) ? name : 'text';
}

/**
 * 标题的纯文字（去掉标记与行内元素）。
 * @param {import('markdown-it').Token | undefined} inline 行内内容的 token
 * @returns {string} 纯文字
 */
function plainText(inline) {
  if (!inline || inline.type !== 'inline') return '';
  return inline.children
    .filter((child) => child.type === 'text' || child.type === 'code_inline')
    .map((child) => child.content)
    .join('');
}

/**
 * 标题的锚点：小写、空白变短横线、去掉标点，中文保留。
 * @param {string} text 标题文字
 * @returns {string} 锚点
 */
function slugOf(text) {
  return text.trim().toLowerCase().replace(/\s+/g, '-').replace(/[^\p{L}\p{N}_-]/gu, '') || 'section';
}

/**
 * 同一页里重复的标题加序号，保证锚点唯一。
 * @param {Map<string, number>} seen 已出现的锚点与次数
 * @param {string} text 标题文字
 * @returns {string} 唯一的锚点
 */
function uniqueSlug(seen, text) {
  const base = slugOf(text);
  const count = seen.get(base) ?? 0;
  seen.set(base, count + 1);
  return count === 0 ? base : `${base}-${count}`;
}

/**
 * 建立一个页面的 Markdown 渲染器：标题加锚点、代码块高亮、链接改写、表格加横向滚动容器。
 * @param {import('shiki').Highlighter} highlighter 高亮器
 * @returns {MarkdownIt} 渲染器
 */
function createMarkdown(highlighter) {
  const md = new MarkdownIt({ html: true, linkify: false, typographer: false });

  md.renderer.rules.fence = (tokens, index) => {
    const token = tokens[index];
    const lang = languageOf(token.info.trim().split(/\s+/)[0] ?? '');
    const code = highlighter.codeToHtml(token.content, { lang, themes: { light: 'github-light', dark: 'github-dark' } });
    return `<div class="code">${code}</div>\n`;
  };

  md.renderer.rules.heading_open = (tokens, index, options, env, self) => {
    const token = tokens[index];
    const text = plainText(tokens[index + 1]);
    const id = uniqueSlug(env.slugs, text);
    token.attrSet('id', id);
    if (TOC_LEVELS.includes(Number(token.tag.slice(1)))) env.headings.push({ level: Number(token.tag.slice(1)), id, text });
    return self.renderToken(tokens, index, options);
  };

  md.renderer.rules.link_open = (tokens, index, options, env, self) => {
    const token = tokens[index];
    const resolved = env.resolveLink(token.attrGet('href') ?? '');
    token.attrSet('href', resolved.href);
    if (resolved.external) {
      token.attrSet('target', '_blank');
      token.attrSet('rel', 'noopener');
    }
    return self.renderToken(tokens, index, options);
  };

  md.renderer.rules.table_open = () => '<div class="table-wrap"><table>\n';
  md.renderer.rules.table_close = () => '</table></div>\n';
  return md;
}

/**
 * 解析一个链接：外部链接原样保留；页面之间的链接改写成相对路径；找不到目标页面时报错。
 * @param {string} href 原始链接
 * @param {Page} page 当前页面
 * @param {Map<string, Page>} pagesByFile 源文件绝对路径到页面
 * @returns {{href: string, external: boolean}} 改写后的链接
 */
function resolveLink(href, page, pagesByFile) {
  if (/^[a-z][a-z0-9+.-]*:/i.test(href)) return { href, external: true };
  if (href.startsWith('#')) return { href, external: false };
  const at = href.indexOf('#');
  const rawPath = at < 0 ? href : href.slice(0, at);
  const hash = at < 0 ? '' : href.slice(at);
  const path = decodeURI(rawPath);
  const base = path.startsWith('/') ? join(HELP_ROOT, path) : resolve(dirname(page.src), path);
  const candidates = [];
  if (rawPath.endsWith('/')) candidates.push(join(base, 'index.md'));
  else if (base.endsWith('.md')) candidates.push(base);
  else candidates.push(`${base}.md`, join(base, 'index.md'));
  const target = candidates.map((file) => pagesByFile.get(resolve(file))).find((found) => found !== undefined);
  if (target === undefined) throw new Error(`帮助页面的链接找不到目标：${href}（${page.src}）`);
  return { href: `${hrefFrom(page.out, target.out)}${hash}`, external: false };
}

/**
 * 去掉 HTML 标签，取纯文字。
 * @param {string} html 片段
 * @returns {string} 纯文字
 */
function stripTags(html) {
  return html.replace(/<[^>]+>/g, '').replace(/&lt;/g, '<').replace(/&gt;/g, '>').replace(/&amp;/g, '&').trim();
}

/**
 * 读取全部分区与页面，渲染正文。
 * @returns {Promise<Content>} 站点内容
 */
export async function loadContent() {
  const highlighter = await createHighlighter({ themes: ['github-light', 'github-dark'], langs: LANGS });
  const sections = buildSections().map((section) => ({ ...section, landing: outputOf(section.groups[0].pages[0].id) }));
  const pages = [];
  for (const section of sections) {
    for (const group of section.groups) {
      for (const entry of group.pages) {
        pages.push({
          id: entry.id,
          src: join(HELP_ROOT, entry.src),
          out: outputOf(entry.id),
          label: entry.label,
          section,
          group: group.title,
          heading: '',
          bodyHtml: '',
          toc: []
        });
      }
    }
  }
  const pagesByFile = new Map(pages.map((page) => [resolve(page.src), page]));
  const markdown = createMarkdown(highlighter);
  for (const page of pages) {
    const text = readFileSync(page.src, 'utf8');
    const env = { slugs: new Map(), headings: [], resolveLink: (href) => resolveLink(href, page, pagesByFile) };
    const html = markdown.render(text, env);
    const match = /<h1 id="[^"]*">([\s\S]*?)<\/h1>\n?/.exec(html);
    if (match === null) throw new Error(`页面没有一级标题：${page.src}`);
    page.heading = stripTags(match[1]);
    page.bodyHtml = html.slice(0, match.index) + html.slice(match.index + match[0].length);
    page.toc = env.headings;
  }
  highlighter.dispose();
  return { sections, pages };
}

/**
 * 某一页的侧边栏：每个分区一组，分区内按分组列出页面；当前页与所在分区标为选中。
 * @param {Content} content 站点内容
 * @param {Page} page 当前页面
 * @returns {NavSection[]} 侧边栏
 */
export function navigationFor(content, page) {
  return content.sections.map((section) => ({
    title: section.title,
    href: hrefFrom(page.out, section.landing),
    open: section.id === page.section.id,
    groups: section.groups.map((group) => ({
      title: group.title,
      items: group.pages.map((entry) => {
        const target = content.pages.find((candidate) => candidate.id === entry.id);
        return { label: entry.label, href: hrefFrom(page.out, target.out), active: entry.id === page.id };
      })
    }))
  }));
}

/**
 * @typedef {{id: string, title: string, summary: string, landing: string, groups: {title: string | null, pages: {id: string, src: string, label: string}[]}[]}} Section
 * @typedef {{id: string, src: string, out: string, label: string, section: Section, group: string | null, heading: string, bodyHtml: string, toc: {level: number, id: string, text: string}[]}} Page
 * @typedef {{sections: Section[], pages: Page[]}} Content
 * @typedef {{title: string, href: string, open: boolean, groups: {title: string | null, items: {label: string, href: string, active: boolean}[]}[]}} NavSection
 */
