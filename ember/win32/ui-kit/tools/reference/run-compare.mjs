/*
 * ------------------------------------------------------------------------
 * 名称：run-compare.mjs
 * 说明：原生控件库与网页版对照：构建网页版参考页，用 Electron 对每页每个“主题 × 缩放比例”一次截下全部场景，再用 gallery 离屏绘制同样的场景，对照样本位置、非文字区域的像素与文字（图标）占据的范围。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用法：node tools/reference/run-compare.mjs [--pages button] [--scenes normal,hover] [--themes dark,light] [--scales 1,1.25] [--gallery gallery.exe路径] [--no-build]
 *       先用 npm run win32:test 构建 gallery（默认取测试构建目录里的 Debug 版）；页的大小、内部不参与对照的边距、做交互场景的样本序号都由 gallery --info 给出；
 *       报告与截图在 build/ember/win32-reference。
 *       判定：样本位置偏差 ≤ 1 个逻辑像素；排除文字区域后，通道差超过 6 的像素不超过 4%，且最大差 ≤ 125（圆角、焦点环与旋转箭头、对勾曲线上两种光栅器的抗锯齿差别；100% 缩放下展开的箭头有约 0.5 像素的偏差）；
 *       文字与图标的外框各边偏差 ≤ 1 个逻辑像素。
 * ------------------------------------------------------------------------
 */

import { execFileSync } from 'node:child_process';
import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { WIN32_REFERENCE_DIRECTORY, WIN32_TEST_BUILD_DIRECTORY } from '../../../../tools/build/build-paths.mjs';
import { compareLayout, comparePixels, contentBox, cropTo } from './compare-images.mjs';
import { decodePng } from './png.mjs';

const HERE = dirname(fileURLToPath(import.meta.url));
const EMBER_ROOT = resolve(HERE, '..', '..', '..', '..');
const PAGE_FILE = join(WIN32_REFERENCE_DIRECTORY, 'page', 'index.html');
const CAPTURE_MAIN = join(HERE, 'capture-main.cjs');

/** 样本位置的容差（逻辑像素）。 */
const LAYOUT_TOLERANCE = 1;
/** 像素对照：通道差超过它算不一致。 */
const PIXEL_TOLERANCE = 6;
/** 像素对照：允许不一致的像素占比（主要是圆角与焦点环曲线上，Skia 与 Direct2D 的抗锯齿不同）。 */
const MISMATCH_RATIO = 0.04;
/** 像素对照：允许的最大通道差。 */
const MAX_DIFF = 125;
/** 文字与图标外框的容差（逻辑像素）。 */
const CONTENT_TOLERANCE = 1;
/** 判定“有内容”的与背景色的差距。 */
const CONTENT_THRESHOLD = 40;
/** 单个子进程（截图或绘制）的超时时间，超时即视为卡住。 */
const TOOL_TIMEOUT_MILLISECONDS = 120000;
/** 默认要跑的场景（每页实际跑哪些由 gallery --info 的 scenes 决定）。 */
const DEFAULT_SCENES = ['normal', 'hover', 'pressed', 'focus', 'open', 'openkey', 'openhover', 'wheel', 'drag'];

/** 解析命令行。 */
function parseArguments(argv) {
  const options = {
    pages: null,
    scenes: DEFAULT_SCENES,
    themes: ['dark', 'light'],
    scales: [1, 1.25, 1.5, 2],
    gallery: join(WIN32_TEST_BUILD_DIRECTORY, 'ui-kit', 'Debug', 'rujian-ui-gallery.exe'),
    build: true
  };
  for (let i = 0; i < argv.length; i += 1) {
    const name = argv[i];
    if (name === '--no-build') options.build = false;
    else if (name === '--pages') options.pages = argv[++i].split(',');
    else if (name === '--scenes') options.scenes = argv[++i].split(',');
    else if (name === '--themes') options.themes = argv[++i].split(',');
    else if (name === '--scales') options.scales = argv[++i].split(',').map(Number);
    else if (name === '--gallery') options.gallery = resolve(argv[++i]);
    else throw new Error(`未知参数：${name}`);
  }
  return options;
}

/** 运行子进程；失败时把它的标准输出与错误输出一起放进异常。 */
function runTool(command, args, environment = process.env) {
  try {
    return execFileSync(command, args, { env: environment, stdio: 'pipe', timeout: TOOL_TIMEOUT_MILLISECONDS, encoding: 'utf8' });
  } catch (error) {
    throw new Error(`${command} 运行失败\n${String(error.stdout ?? '')}${String(error.stderr ?? '')}`, { cause: error });
  }
}

/** 构建网页版参考页。 */
async function buildReferencePage() {
  const { build } = await import(pathToFileURL(createRequire(join(EMBER_ROOT, 'package.json')).resolve('vite')).href);
  await build({ configFile: join(HERE, 'vite.config.mts') });
}

/** 一次截下一页在一组场景下的网页版截图。 */
function captureWeb(info, theme, scale, cases, prefix) {
  const electron = createRequire(join(EMBER_ROOT, 'package.json'))('electron');
  const environment = { ...process.env };
  delete environment.ELECTRON_RUN_AS_NODE;
  runTool(
    electron,
    [CAPTURE_MAIN, `--page=${PAGE_FILE}`, `--name=${info.name}`, `--theme=${theme}`, `--scale=${scale}`, `--width=${info.width}`, `--height=${info.height}`, `--cases=${cases.map((item) => item.scene + (item.target === null ? '' : `:${item.target}`)).join(',')}`, `--prefix=${prefix}`],
    environment
  );
}

/** 运行原生版绘制。 */
function renderNative(gallery, info, theme, scale, item, out, rects) {
  runTool(gallery, ['--render', out, '--rects', rects, '--page', info.name, '--theme', theme, '--scale', String(scale), '--scene', item.scene, '--target', String(item.target ?? 0)]);
}

/** 逻辑矩形转设备像素外框；inset 是四边各向内缩进的距离。 */
function deviceBox(rect, scale, inset) {
  return {
    left: Math.round((rect.x + inset.left) * scale),
    top: Math.round((rect.y + inset.top) * scale),
    right: Math.round((rect.x + rect.width - inset.right) * scale),
    bottom: Math.round((rect.y + rect.height - inset.bottom) * scale)
  };
}

/** 对照一种组合，返回结果。insetOf 给出每个样本内部不参与对照的边距。 */
function compareCase(name, scale, insetOf, webImage, nativeImage, webRects, nativeRects) {
  const problems = [];
  const layout = compareLayout(webRects, nativeRects);
  if (layout.maxDelta > LAYOUT_TOLERANCE) problems.push(`样本位置最大偏差 ${layout.maxDelta.toFixed(2)}px`);

  const masks = webRects.map((rect, index) => deviceBox(rect, scale, insetOf(index)));
  const pixels = comparePixels(webImage, nativeImage, masks, PIXEL_TOLERANCE);
  const ratio = pixels.mismatched / pixels.total;
  if (ratio > MISMATCH_RATIO || pixels.maxDiff > MAX_DIFF) problems.push(`像素不一致 ${(ratio * 100).toFixed(3)}%，最大差 ${pixels.maxDiff}，位置 ${JSON.stringify(pixels.worst)}`);

  const contents = webRects.map((rect, index) => {
    const inset = insetOf(index);
    const area = deviceBox(rect, scale, { left: Math.max(0, inset.left), top: Math.max(0, inset.top), right: Math.max(0, inset.right), bottom: Math.max(0, inset.bottom) });
    if (area.right <= area.left || area.bottom <= area.top) return { index, web: null, native: null, delta: null };
    const probe = (area.top * webImage.width + area.left) * 4;
    const background = [webImage.data[probe], webImage.data[probe + 1], webImage.data[probe + 2]];
    const web = contentBox(webImage, area, background, CONTENT_THRESHOLD);
    const native = contentBox(nativeImage, area, background, CONTENT_THRESHOLD);
    if (!web || !native) {
      if (web !== native) problems.push(`样本 ${index} 文字：网页版${web ? '有' : '无'}、原生版${native ? '有' : '无'}`);
      return { index, web, native, delta: null };
    }
    const delta = { left: native.left - web.left, top: native.top - web.top, right: native.right - web.right, bottom: native.bottom - web.bottom };
    const limit = Math.ceil(CONTENT_TOLERANCE * scale);
    if (Math.max(...Object.values(delta).map(Math.abs)) > limit) problems.push(`样本 ${index} 文字外框偏差 ${JSON.stringify(delta)}（设备像素）`);
    return { index, web, native, delta };
  });
  return { name, layout, pixels: { ...pixels, ratio }, contents, problems };
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  if (!existsSync(options.gallery)) throw new Error(`没有找到 gallery：${options.gallery}，请先构建`);
  if (options.build || !existsSync(PAGE_FILE)) await buildReferencePage();
  mkdirSync(WIN32_REFERENCE_DIRECTORY, { recursive: true });

  const infos = JSON.parse(runTool(options.gallery, ['--info']));
  const results = [];
  for (const info of infos) {
    if (options.pages && !options.pages.includes(info.name)) continue;
    const cases = [];
    if (options.scenes.includes('normal')) cases.push({ scene: 'normal', target: null });
    for (const scene of info.scenes) if (options.scenes.includes(scene)) for (const target of info.targets) cases.push({ scene, target });
    const insetOf = (index) => {
      if (index >= info.sampleCount) return info.extraInset;
      return info.overrides.find((item) => item.index === index)?.inset ?? info.inset;
    };
    for (const theme of options.themes) {
      for (const scale of options.scales) {
        const base = `${info.name}-${theme}-${scale}`;
        const prefix = join(WIN32_REFERENCE_DIRECTORY, `${base}-web`);
        captureWeb(info, theme, scale, cases, prefix);
        for (const item of cases) {
          const caseName = item.scene === 'normal' ? 'normal' : `${item.scene}${item.target}`;
          const nativePng = join(WIN32_REFERENCE_DIRECTORY, `${base}-native-${caseName}.png`);
          const nativeJson = join(WIN32_REFERENCE_DIRECTORY, `${base}-native-${caseName}.json`);
          renderNative(options.gallery, info, theme, scale, item, nativePng, nativeJson);
          const nativeImage = decodePng(nativePng);
          const webImage = cropTo(decodePng(`${prefix}-${caseName}.png`), nativeImage.width, nativeImage.height);
          const result = compareCase(`${base}-${caseName}`, scale, insetOf, webImage, nativeImage, JSON.parse(readFileSync(`${prefix}-${caseName}.json`, 'utf8')), JSON.parse(readFileSync(nativeJson, 'utf8')));
          results.push(result);
          if (result.problems.length > 0) {
            console.log(`不一致  ${result.name}  位置偏差 ${result.layout.maxDelta.toFixed(2)}  像素不一致 ${(result.pixels.ratio * 100).toFixed(3)}%（最大差 ${result.pixels.maxDiff}）`);
            for (const problem of result.problems) console.log(`    ${problem}`);
          }
        }
        console.log(`${base}：${cases.length} 个场景已对照`);
      }
    }
  }
  writeFileSync(join(WIN32_REFERENCE_DIRECTORY, 'report.json'), JSON.stringify(results, null, 2) + '\n');
  const failed = results.filter((result) => result.problems.length > 0).length;
  console.log(`共 ${results.length} 个场景，${failed} 个不一致；报告与截图在 ${WIN32_REFERENCE_DIRECTORY}`);
  if (failed > 0) process.exit(1);
}

main().catch((error) => {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
});
