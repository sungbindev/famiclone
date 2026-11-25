/*
 * ------------------------------------------------------------------------
 * 名称：format-html.mjs
 * 说明：把站点生成的 HTML 排成多行并按层级缩进，只在块级标签前后换行，不改动行内内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：<pre>、<script>、<style>、<textarea> 的内容原样保留；行内标签之间不加空白，渲染结果不变。
 * ------------------------------------------------------------------------
 */

/** 块级标签：开标签与闭标签各自单独成行，内容比标签缩进一级。 */
const BLOCK_TAGS = new Set([
  'html', 'head', 'body', 'div', 'section', 'header', 'footer', 'nav', 'main', 'aside', 'article',
  'p', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'ul', 'ol', 'li', 'table', 'thead', 'tbody', 'tfoot',
  'tr', 'td', 'th', 'dl', 'dt', 'dd', 'figure', 'figcaption', 'details', 'summary', 'form', 'blockquote'
]);
/** 没有闭标签的块级标签：单独成行，不改变缩进层级。 */
const LINE_TAGS = new Set(['meta', 'link', 'hr']);
/** 内容需要原样保留的标签。 */
const PROTECTED = /<(pre|script|style|textarea)\b[\s\S]*?<\/\1\s*>/gi;
/** 占位符的分隔字符，源码里不会出现。 */
const MARK = '\u0000';

/**
 * 排版 HTML 文本。
 * @param {string} html 单行或已有换行的 HTML
 * @returns {string} 排版后的 HTML，末尾带换行
 */
export function formatHtml(html) {
  const kept = [];
  const masked = html.replace(PROTECTED, (block) => {
    kept.push(block);
    return `${MARK}${kept.length - 1}${MARK}`;
  });
  const restore = (text) => text.replace(new RegExp(`${MARK}(\\d+)${MARK}`, 'g'), (_, index) => kept[Number(index)]);
  const whole = new RegExp(`^${MARK}(\\d+)${MARK}$`);

  let depth = 0;
  let out = '';
  const line = (text) => `\n${'  '.repeat(depth)}${text}`;
  for (const token of masked.split(/(<[^>]*>)/)) {
    if (token === '') continue;
    const tag = /^<(\/?)([a-zA-Z][\w-]*)[^>]*>$/.exec(token);
    if (tag === null) {
      out += whole.test(token) ? line(restore(token)) : restore(token);
      continue;
    }
    const closing = tag[1] === '/';
    const name = tag[2].toLowerCase();
    if (LINE_TAGS.has(name)) {
      out += line(token);
    } else if (BLOCK_TAGS.has(name)) {
      if (closing) depth = Math.max(0, depth - 1);
      out += line(token);
      if (!closing && !token.endsWith('/>')) depth += 1;
    } else {
      out += token;
    }
  }
  return `${out.trim()}\n`;
}
