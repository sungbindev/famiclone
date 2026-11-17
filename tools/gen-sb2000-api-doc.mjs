// ------------------------------------------------------------------------
// 名称：gen-sb2000-api-doc.mjs
// 说明：由 SB2000 目标的头文件文档注释生成 C 接口参考文档 api-reference.md，并附系统调用覆盖表与不封装清单。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：用法 node tools/gen-sb2000-api-doc.mjs；改了头文件注释或 api-unwrapped.json 后重新运行；test/sb2000-api.test.mjs 核对入库的文档与生成结果一致。
//       文档来源只有头文件注释（fcdev api 命令读的也是同一份），所以这里不手写任何函数说明。
// ------------------------------------------------------------------------
import { writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { groupByModule } from '../packages/fcdev/src/api/index.mjs';
import { coverageOf, KNOWN_FUNCTIONS, readFunctions, readUnwrapped } from './check-sb2000-api.mjs';

/** 仓库根目录 */
const REPO_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

/** SB2000 目标目录 */
const TARGET_DIR = path.join(REPO_ROOT, 'packages', 'fcdev', 'targets', 'sb2000');

/** 生成的文档路径 */
export const API_DOC_FILE = path.join(TARGET_DIR, 'api-reference.md');

/** 模块的中文名 */
const MODULE_TITLES = {
  sb2000: '总头文件与通用调用', system: '系统与中断', memory: '内存页与远指针', text: '文字输出', input: '键盘、手柄与鼠标', clock: '日期与时间',
  file: '文件与目录', disk: '软驱扇区', math: '数学', sound: '声音与语音', printer: '打印机', ime: '中文输入法', display: '显示、窗口与图片',
  dma: '通用 DMA', ppu: '图形端口', timer: '硬件定时器',
};

/**
 * 生成 C 接口参考文档的全文。
 * @param {string} [targetDir] 目标目录
 * @returns {string} Markdown 文本（LF 换行）
 */
export function renderApiReference(targetDir = TARGET_DIR) {
  const functions = readFunctions(targetDir);
  const groups = groupByModule(functions);
  const unwrapped = readUnwrapped(targetDir);
  const lines = [
    '# SB2000 C 接口参考',
    '',
    '本文由运行库头文件 `sb2000-*.h` 的文档注释生成。',
    `共 ${functions.length} 个函数，${groups.size} 个模块。随时查询用 \`npx fcdev api\`：\`npx fcdev api <函数名>\` 看完整文档，\`npx fcdev api <模块名>\` 看模块的函数列表，\`npx fcdev api '$7B'\` 看用到某个系统调用的函数，\`npx fcdev api <词>\` 按名称与说明搜索。`,
    '',
    '约定：',
    '',
    '- 所有函数用 `#include <sb2000.h>` 引入；返回 DOS 出错码或软驱状态码的函数，0 表示成功，结果经指针参数带回。',
    '- 字符串、缓冲区、结构体指针必须在程序窗口 `$8000-$BFFF` 里（系统服务换页后程序窗口不可见的，函数内部已经处理好，调用者不用管）。',
    '- 程序最大 16K（窗口 `$8000-$BFFF`，C 软件栈占顶端 1K）。',
    '- 汇编程序调用这些函数时，函数名前加下划线（如 `_sb_puts`），参数按 cc65 的 fastcall 约定传递；系统调用号、地址与寄存器的符号见 [SB2000 符号参考](symbols-reference.md)。',
    '',
    '## 目录',
    '',
  ];
  for (const [module, items] of groups) lines.push(`- [${MODULE_TITLES[module] ?? module}（${items[0].header}）](#${module})`);
  lines.push('- [系统调用覆盖表](#系统调用覆盖表)', '');

  for (const [module, items] of groups) {
    lines.push(`<a id="${module}"></a>`, `## ${MODULE_TITLES[module] ?? module}（${items[0].header}）`, '');
    lines.push('| 函数 | 说明 | 系统调用 |', '| --- | --- | --- |');
    for (const fn of items) lines.push(`| [\`${fn.name}\`](#${fn.name}) | ${fn.summary.replace(/\|/g, '\\|')} | ${fn.syscalls.join('；').replace(/\|/g, '\\|')} |`);
    lines.push('');
    for (const fn of items) {
      lines.push(`<a id="${fn.name}"></a>`, `### ${fn.name}`, '', '```c', `${fn.signature};`, '```', '', fn.description, '');
      if (fn.syscalls.length > 0) lines.push(`系统调用：${fn.syscalls.join('；')}`, '');
      if (fn.params.length > 0) {
        lines.push('参数：', '');
        for (const param of fn.params) lines.push(`- \`${param}\`：${fn.paramText[param] ?? ''}`);
        lines.push('');
      }
      if (fn.hasReturnDoc) lines.push(`返回：${fn.returnText}`, '');
    }
  }

  const coverage = coverageOf(functions);
  lines.push('## 系统调用覆盖表', '', 'BIOS 里有处理例程的每个功能号：被哪些函数封装，或者为什么不封装。需要没有封装的部分时，用 `sb_syscall` 加 `sb2000-symbols.h` 里的常量直接调用。', '');
  lines.push('| 功能号 | 封装函数 | 没有封装的部分 |', '| --- | --- | --- |');
  for (const number of KNOWN_FUNCTIONS) {
    const wrappers = coverage.get(number);
    const key = `$${number}`;
    if (wrappers.length > 0) {
      const shown = wrappers.length > 4 ? `${wrappers.slice(0, 4).join('、')} 等 ${wrappers.length} 个` : wrappers.join('、');
      lines.push(`| ${key} | ${shown} | ${(unwrapped.partial[key] ?? '').replace(/\|/g, '\\|')} |`);
    } else {
      lines.push(`| ${key} | （不封装） | ${(unwrapped.functions[key] ?? '').replace(/\|/g, '\\|')} |`);
    }
  }
  lines.push('');
  return lines.join('\n');
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const text = renderApiReference();
  writeFileSync(API_DOC_FILE, text);
  console.log(`已生成 ${path.relative(REPO_ROOT, API_DOC_FILE)}（${text.split('\n').length} 行）`);
}
