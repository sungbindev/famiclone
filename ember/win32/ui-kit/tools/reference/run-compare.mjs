/*
 * ------------------------------------------------------------------------
 * 名称：run-compare.mjs
 * 说明：原生控件库与网页版对照：构建网页版参考页，用 Electron 截图，再用 gallery 离屏绘制同样的页面，对照控件位置、非文字区域的像素与文字（图标）占据的范围。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：用法：node tools/reference/run-compare.mjs [--scenes normal,hover] [--themes dark,light] [--scales 1,1.25] [--gallery gallery.exe路径] [--no-build]
 *       先用 npm run win32:test 或同等命令构建 gallery（默认取测试构建目录里的 Debug 版）；报告与截图在 build/ember/win32-reference。
 *       判定：控件位置偏差 ≤ 1 个逻辑像素；排除文字区域后，通道差超过 3 的像素不超过 1.5%，且最大差 ≤ 64（圆角与焦点环曲线上两种光栅器的抗锯齿差别）；
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
/** 示例页的逻辑大小，与 gallery 的 kPageSize 一致。 */
const PAGE_SIZE = { width: 440, height: 60 };

/** 控件位置的容差（逻辑像素）。 */
const LAYOUT_TOLERANCE = 1;
/** 像素对照：通道差超过它算不一致。 */
const PIXEL_TOLERANCE = 3;
/** 像素对照：允许不一致的像素占比（主要是圆角与焦点环曲线上，Skia 与 Direct2D 的抗锯齿不同）。 */
const MISMATCH_RATIO = 0.015;
/** 像素对照：允许的最大通道差。 */
const MAX_DIFF = 64;
/** 文字与图标外框的容差（逻辑像素）。 */
const CONTENT_TOLERANCE = 1;
/** 内容区域相对按钮边缘的缩进（逻辑像素）：横向、纵向。 */
const CONTENT_INSET = { x: 6, y: 3 };
/** 判定“有内容”的与背景色的差距。 */
const CONTENT_THRESHOLD = 40;
/** 单个子进程（截图或绘制）的超时时间，超时即视为卡住。 */
const TOOL_TIMEOUT_MILLISECONDS = 60000;

/** 解析命令行。 */
function parseArguments(argv) {
  const options = {
    scenes: ['normal', 'hover', 'pressed', 'focus'],
    themes: ['dark', 'light'],
    scales: [1, 1.25, 1.5, 2],
    gallery: join(WIN32_TEST_BUILD_DIRECTORY, 'ui-kit', 'Debug', 'rujian-ui-gallery.exe'),
    build: true
  };
  for (let i = 0; i < argv.length; i += 1) {
    const name = argv[i];
    if (name === '--no-build') options.build = false;
    else if (name === '--scenes') options.scenes = argv[++i].split(',');
    else if (name === '--themes') options.themes = argv[++i].split(',');
    else if (name === '--scales') options.scales = argv[++i].split(',').map(Number);
    else if (name === '--gallery') options.gallery = resolve(argv[++i]);
    else throw new Error(`未知参数：${name}`);
  }
  return options;
}

/** 构建网页版参考页。 */
async function buildReferencePage() {
  const { build } = await import(pathToFileURL(createRequire(join(EMBER_ROOT, 'package.json')).resolve('vite')).href);
  await build({ configFile: join(HERE, 'vite.config.mts') });
}

/** 运行子进程；失败时把它的标准输出与错误输出一起放进异常。 */
function runTool(command, args, environment = process.env) {
  try {
    execFileSync(command, args, { env: environment, stdio: 'pipe', timeout: TOOL_TIMEOUT_MILLISECONDS });
  } catch (error) {
    throw new Error(`${command} 运行失败\n${String(error.stdout ?? '')}${String(error.stderr ?? '')}`, { cause: error });
  }
}

/** 运行网页版截图。 */
function captureWeb(theme, scale, scene, out, rects) {
  const electron = createRequire(join(EMBER_ROOT, 'package.json'))('electron');
  const environment = { ...process.env };
  delete environment.ELECTRON_RUN_AS_NODE;
  runTool(electron, [CAPTURE_MAIN, `--page=${PAGE_FILE}`, `--theme=${theme}`, `--scale=${scale}`, `--scene=${scene}`, `--width=${PAGE_SIZE.width}`, `--height=${PAGE_SIZE.height}`, `--out=${out}`, `--rects=${rects}`], environment);
}

/** 运行原生版绘制。 */
function renderNative(gallery, theme, scale, scene, out, rects) {
  runTool(gallery, ['--render', out, '--rects', rects, '--theme', theme, '--scale', String(scale), '--scene', scene]);
}

/** 逻辑矩形转设备像素外框。 */
function deviceBox(rect, scale, inset) {
  return {
    left: Math.round((rect.x + inset.x) * scale),
    top: Math.round((rect.y + inset.y) * scale),
    right: Math.round((rect.x + rect.width - inset.x) * scale),
    bottom: Math.round((rect.y + rect.height - inset.y) * scale)
  };
}

/** 对照一种组合，返回结果。 */
function compareCase(name, scale, webImage, nativeImage, webRects, nativeRects) {
  const problems = [];
  const layout = compareLayout(webRects, nativeRects);
  if (layout.maxDelta > LAYOUT_TOLERANCE) problems.push(`控件位置最大偏差 ${layout.maxDelta.toFixed(2)}px`);

  const masks = webRects.map((rect) => deviceBox(rect, scale, CONTENT_INSET));
  const pixels = comparePixels(webImage, nativeImage, masks, PIXEL_TOLERANCE);
  const ratio = pixels.mismatched / pixels.total;
  if (ratio > MISMATCH_RATIO || pixels.maxDiff > MAX_DIFF) problems.push(`像素不一致 ${(ratio * 100).toFixed(3)}%，最大差 ${pixels.maxDiff}，位置 ${JSON.stringify(pixels.worst)}`);

  const contents = webRects.map((rect, index) => {
    const area = deviceBox(rect, scale, CONTENT_INSET);
    const probe = (Math.round((rect.y + rect.height / 2) * scale) * webImage.width + Math.round((rect.x + 2) * scale)) * 4;
    const background = [webImage.data[probe], webImage.data[probe + 1], webImage.data[probe + 2]];
    const web = contentBox(webImage, area, background, CONTENT_THRESHOLD);
    const native = contentBox(nativeImage, area, background, CONTENT_THRESHOLD);
    if (!web || !native) {
      if (web !== native) problems.push(`按钮 ${index} 文字：网页版${web ? '有' : '无'}、原生版${native ? '有' : '无'}`);
      return { index, web, native, delta: null };
    }
    const delta = { left: native.left - web.left, top: native.top - web.top, right: native.right - web.right, bottom: native.bottom - web.bottom };
    const limit = CONTENT_TOLERANCE * scale;
    if (Math.max(...Object.values(delta).map(Math.abs)) > limit) problems.push(`按钮 ${index} 文字外框偏差 ${JSON.stringify(delta)}（设备像素）`);
    return { index, web, native, delta };
  });
  return { name, layout, pixels: { ...pixels, ratio }, contents, problems };
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  if (!existsSync(options.gallery)) throw new Error(`没有找到 gallery：${options.gallery}，请先构建`);
  if (options.build || !existsSync(PAGE_FILE)) await buildReferencePage();
  mkdirSync(WIN32_REFERENCE_DIRECTORY, { recursive: true });

  const results = [];
  for (const theme of options.themes) {
    for (const scale of options.scales) {
      for (const scene of options.scenes) {
        const name = `${theme}-${scale}-${scene}`;
        const files = { webPng: join(WIN32_REFERENCE_DIRECTORY, `${name}-web.png`), webJson: join(WIN32_REFERENCE_DIRECTORY, `${name}-web.json`), nativePng: join(WIN32_REFERENCE_DIRECTORY, `${name}-native.png`), nativeJson: join(WIN32_REFERENCE_DIRECTORY, `${name}-native.json`) };
        captureWeb(theme, scale, scene, files.webPng, files.webJson);
        renderNative(options.gallery, theme, scale, scene, files.nativePng, files.nativeJson);
        const nativeImage = decodePng(files.nativePng);
        const result = compareCase(name, scale, cropTo(decodePng(files.webPng), nativeImage.width, nativeImage.height), nativeImage, JSON.parse(readFileSync(files.webJson, 'utf8')), JSON.parse(readFileSync(files.nativeJson, 'utf8')));
        results.push(result);
        const status = result.problems.length === 0 ? '通过' : '不一致';
        console.log(`${status}  ${name}  位置偏差 ${result.layout.maxDelta.toFixed(2)}  像素不一致 ${(result.pixels.ratio * 100).toFixed(3)}%（最大差 ${result.pixels.maxDiff}）`);
        for (const problem of result.problems) console.log(`    ${problem}`);
      }
    }
  }
  writeFileSync(join(WIN32_REFERENCE_DIRECTORY, 'report.json'), JSON.stringify(results, null, 2) + '\n');
  const failed = results.filter((result) => result.problems.length > 0).length;
  console.log(`共 ${results.length} 种组合，${failed} 种不一致；报告与截图在 ${WIN32_REFERENCE_DIRECTORY}`);
  if (failed > 0) process.exit(1);
}

main().catch((error) => {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
});
