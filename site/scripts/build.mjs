/*
 * ------------------------------------------------------------------------
 * 名称：build.mjs
 * 说明：站点的构建入口（npm run build）：先刷新帮助文档的生成内容，再打包样式，用 Vite 的 SSR 把每一页渲染成静态 HTML，
 *       首页写到仓库根目录的 index.html，其余页面与资源写到 site/ 下。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-23
 * 备注：首页是根目录的 index.html，其余页面在 site/<分区>/ 下；页面之间与资源都用相对路径，直接打开或部署到 GitHub Pages 都能用。
 *       组件库从 UI_KIT_SOURCE 指定的目录（默认 ../../rujian-ui-kit，与仓库并列）直接引用源码。
 *       build/site 是临时目录（已忽略）；不加 --skip-refresh 时会重新生成 help/api 与 help/fcdev（它们不提交）。
 * ------------------------------------------------------------------------
 */

import { execFileSync } from 'node:child_process';
import { copyFileSync, existsSync, mkdirSync, readdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { build as viteBuild, createServer } from 'vite';
import react from '@vitejs/plugin-react';
import { REPO_ROOT, SITE_DIR, HELP_ROOT, hrefFrom, loadContent, navigationFor } from './content.mjs';
import { EMBER_SHOTS, FCDEV_SHOTS, HERO_SHOT } from './shots.mjs';
import { importFcdevDocs } from '../../ember/tools/build/import-fcdev-docs.mjs';

const here = dirname(fileURLToPath(import.meta.url));
/** 站点项目目录（famiclone/site）。 */
const SITE_ROOT = resolve(here, '..');
/** 组件库源码目录。 */
const UI_KIT_SOURCE = resolve(SITE_ROOT, process.env.UI_KIT_SOURCE ?? '../../rujian-ui-kit');
/** 临时构建目录（仓库的 build 下，已忽略）。 */
const BUILD_DIR = join(REPO_ROOT, 'build', 'site');
/** 站点资源目录（样式、字体、图片）相对仓库根目录的路径。 */
const ASSETS_DIR = `${SITE_DIR}/assets`;
/** 样式入口的产物文件名。 */
const STYLE_FILE = 'site.css';
/** 站点的 GitHub 仓库。 */
const REPOSITORY_URL = 'https://github.com/sungbindev/famiclone';
/** 各分区的首页（用于清理旧页面）。 */
const SECTION_DIRECTORIES = ['manual', 'plugin', 'debug', 'api', 'fcdev'];
/** fcdev 的参考文档生成脚本（先于手册导入运行）。 */
const FCDEV_GENERATORS = ['gen-sb2000-api-doc.mjs', 'gen-sb2000-symbols-doc.mjs', 'gen-sb2000-examples-doc.mjs'];
/** 首页与页面要链接的 fcdev 手册页。 */
const FCDEV_LINKS = { start: 'fcdev/install', build: 'fcdev/build', emulator: 'fcdev/emulator', nes: 'fcdev/nes', sb2000: 'fcdev/sb2000', syscall: 'fcdev/syscall' };

/**
 * 刷新帮助文档的生成内容：调试接口模块页、fcdev 参考文档与手册导入（与 npm run help:build 的步骤一致，只是不再构建 VitePress）。
 * @returns {void}
 */
function refreshHelpSources() {
  const extension = join(REPO_ROOT, 'vscode');
  if (!existsSync(join(extension, 'debug', 'openrpc.json'))) throw new Error('扩展里没有 debug/openrpc.json：先在 ember 目录运行 npm run schema');
  if (!existsSync(join(extension, 'out', 'shared', 'debug-client', 'api-reference.js'))) throw new Error('找不到扩展的编译产物：先在 vscode 目录运行 npm run build');
  execFileSync(process.execPath, [join(extension, 'tools', 'gen', 'gen-debug-api-docs.mjs'), join(HELP_ROOT, 'api')], { cwd: extension, stdio: 'inherit' });
  for (const script of FCDEV_GENERATORS) execFileSync(process.execPath, [join(extension, 'tools', 'gen', script)], { cwd: REPO_ROOT, stdio: 'inherit' });
  const { pages, written } = importFcdevDocs();
  console.log(`fcdev 手册：${pages} 页，写入 ${written} 页`);
}

/**
 * Vite 的共用配置：组件库的别名，只保留一份 React。
 * @returns {import('vite').InlineConfig} 配置
 */
function sharedConfig() {
  return {
    configFile: false,
    root: SITE_ROOT,
    logLevel: 'warn',
    plugins: [react()],
    resolve: {
      alias: [{ find: /^@rujianjs\/ui-kit$/, replacement: join(UI_KIT_SOURCE, 'src', 'index.ts') }],
      dedupe: ['react', 'react-dom']
    }
  };
}

/**
 * 打包样式：组件库与站点的样式合成一个文件，字体等资源与它放在同一目录，复制到 site/assets。
 * @returns {Promise<void>}
 */
async function buildStyles() {
  const outDir = join(BUILD_DIR, 'css');
  rmSync(outDir, { recursive: true, force: true });
  await viteBuild({
    ...sharedConfig(),
    build: {
      outDir,
      emptyOutDir: true,
      cssCodeSplit: false,
      // 组件库的组件模块只有 CSS 有副作用，不关闭摇树会把没有引用的组件样式一起丢掉
      rollupOptions: {
        input: join(SITE_ROOT, 'src', 'app', 'site.ts'),
        treeshake: false,
        output: {
          entryFileNames: '[name].js',
          assetFileNames: (info) => (String(info.name ?? '').endsWith('.css') ? STYLE_FILE : '[name][extname]')
        }
      }
    }
  });
  const target = join(REPO_ROOT, ...ASSETS_DIR.split('/'));
  mkdirSync(target, { recursive: true });
  for (const name of readdirSync(outDir)) {
    if (name.endsWith('.js')) continue;
    copyFileSync(join(outDir, name), join(target, name));
  }
  const styleFile = join(target, STYLE_FILE);
  if (!existsSync(styleFile)) throw new Error(`样式没有生成：${STYLE_FILE}`);
  // 压缩器会把 CSS 转义（如 \25B8）还原成字面字符，文件没有编码声明时会乱码，所以统一转回转义
  const ascii = readFileSync(styleFile, 'utf8').replace(/[^\x00-\x7f]/gu, (ch) => `\\${ch.codePointAt(0).toString(16)} `);
  writeFileSync(styleFile, ascii, 'utf8');
}

/**
 * 启动 SSR 环境并加载页面渲染函数。
 * @returns {Promise<{render: (view: object) => string, close: () => Promise<void>}>} 渲染器
 */
async function createRenderer() {
  const server = await createServer({
    ...sharedConfig(),
    appType: 'custom',
    server: { middlewareMode: true, hmr: false, watch: null, fs: { allow: [REPO_ROOT, SITE_ROOT, UI_KIT_SOURCE] } },
    optimizeDeps: { noDiscovery: true, include: [] }
  });
  const module = await server.ssrLoadModule('/src/app/render.tsx');
  return { render: module.renderPage, close: () => server.close() };
}

/**
 * 检查截图都在，缺了直接报错（页面上的图片不能是断的）。
 * @param {{file: string}[]} shots 截图清单
 * @returns {void}
 */
function checkShots(shots) {
  for (const shot of shots) {
    const file = join(REPO_ROOT, ...ASSETS_DIR.split('/'), 'shots', shot.file);
    if (!existsSync(file)) throw new Error(`找不到截图：${file}`);
  }
}

/**
 * 每一页共用的部分：样式、图标、标题栏的导航与仓库链接。
 * @param {string} out 页面输出路径
 * @param {object} content 站点内容
 * @param {string} title 页面标题
 * @returns {object} 共用视图
 */
function commonView(out, content, title, description) {
  const pageOut = (id) => content.pages.find((page) => page.id === id)?.out ?? (() => { throw new Error(`站点没有页面：${id}`); })();
  const href = (id) => hrefFrom(out, pageOut(id));
  return {
    title,
    description,
    css: hrefFrom(out, `${ASSETS_DIR}/${STYLE_FILE}`),
    icon: hrefFrom(out, `${ASSETS_DIR}/fcdev-icon.png`),
    logo: hrefFrom(out, `${ASSETS_DIR}/ember-logo.png`),
    home: hrefFrom(out, 'index.html'),
    repository: REPOSITORY_URL,
    topLinks: [
      { label: 'Ember 模拟器', href: href('manual/start') },
      { label: 'fcdev 扩展', href: href('fcdev/index') },
      { label: '帮助文档', href: href('manual/index') }
    ],
    nav: null,
    href
  };
}

/**
 * 首页的视图。
 * @param {object} content 站点内容
 * @returns {object} 视图
 */
function homeView(content) {
  const common = commonView('index.html', content, 'Ember 与 fcdev', 'Ember 模拟器与 fcdev 开发工具：NES 与学习机的运行、调试，以及 VS Code 扩展与帮助文档。');
  const shot = (item) => ({ src: hrefFrom('index.html', `${ASSETS_DIR}/shots/${item.file}`), alt: item.alt, caption: item.caption });
  const sectionCount = (id) => content.sections.find((section) => section.id === id).groups.reduce((sum, group) => sum + group.pages.length, 0);
  return {
    ...common,
    kind: 'home',
    nav: navigationFor(content, { id: 'home', out: 'index.html', section: { id: '' } }),
    landing: {
      hero: shot(HERO_SHOT),
      emberShots: EMBER_SHOTS.map(shot),
      fcdevShots: FCDEV_SHOTS.map(shot),
      counts: {
        manual: sectionCount('manual'),
        plugin: sectionCount('plugin'),
        debug: sectionCount('debug'),
        fcdev: sectionCount('fcdev')
      },
      links: {
        manualStart: common.href('manual/start'),
        manualIndex: common.href('manual/index'),
        pluginIndex: common.href('plugin/index'),
        debugIndex: common.href('debug/index'),
        fcdevIndex: common.href('fcdev/index'),
        fcdevStart: common.href(FCDEV_LINKS.start),
        fcdevBuild: common.href(FCDEV_LINKS.build),
        fcdevEmulator: common.href(FCDEV_LINKS.emulator),
        fcdevNes: common.href(FCDEV_LINKS.nes),
        fcdevSb2000: common.href(FCDEV_LINKS.sb2000),
        fcdevSyscall: common.href(FCDEV_LINKS.syscall)
      }
    }
  };
}

/**
 * 帮助页的视图：侧边栏、标题、正文、页内目录与上一页、下一页。
 * @param {object} content 站点内容
 * @param {object} page 页面
 * @returns {object} 视图
 */
function docView(content, page) {
  const common = commonView(page.out, content, `${page.label} · ${page.section.title} · Ember 与 fcdev`, `${page.section.title}：${page.label}`);
  const index = content.pages.indexOf(page);
  const pager = (target) => (target ? { title: target.label, href: hrefFrom(page.out, target.out), section: target.section.title } : undefined);
  return {
    ...common,
    kind: 'doc',
    nav: navigationFor(content, page),
    doc: {
      sectionTitle: page.section.title,
      sectionHref: hrefFrom(page.out, page.section.landing),
      label: page.label,
      heading: page.heading,
      bodyHtml: page.bodyHtml,
      toc: page.toc,
      prev: pager(content.pages[index - 1]),
      next: pager(content.pages[index + 1])
    }
  };
}

/**
 * 写出一个页面，目录不存在时创建。
 * @param {string} out 相对仓库根目录的输出路径
 * @param {string} html 页面文本
 * @returns {void}
 */
function writePage(out, html) {
  const file = join(REPO_ROOT, ...out.split('/'));
  mkdirSync(dirname(file), { recursive: true });
  writeFileSync(file, `<!doctype html>\n${html}`, 'utf8');
}

/**
 * 构建入口。
 * @returns {Promise<void>}
 */
async function main() {
  if (!process.argv.includes('--skip-refresh')) refreshHelpSources();
  checkShots([HERO_SHOT, ...EMBER_SHOTS, ...FCDEV_SHOTS]);

  const assetsTarget = join(REPO_ROOT, ...ASSETS_DIR.split('/'));
  mkdirSync(assetsTarget, { recursive: true });
  copyFileSync(join(REPO_ROOT, 'ember', 'resources', 'ember-logo.png'), join(assetsTarget, 'ember-logo.png'));
  copyFileSync(join(REPO_ROOT, 'vscode', 'resources', 'icon.png'), join(assetsTarget, 'fcdev-icon.png'));
  await buildStyles();

  const content = await loadContent();
  for (const directory of SECTION_DIRECTORIES) rmSync(join(REPO_ROOT, SITE_DIR, directory), { recursive: true, force: true });

  const renderer = await createRenderer();
  try {
    writePage('index.html', renderer.render(homeView(content)));
    for (const page of content.pages) writePage(page.out, renderer.render(docView(content, page)));
  } finally {
    await renderer.close();
  }
  console.log(`站点：首页 index.html，帮助页 ${content.pages.length} 页，写入 ${SITE_DIR}/`);
}

try {
  await main();
} catch (error) {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
}
