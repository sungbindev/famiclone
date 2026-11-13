/*
 * ------------------------------------------------------------------------
 * 名称：latency-run.mjs
 * 说明：画面延迟与丢帧测量脚本：启动开发版，让主进程加载一个测试 ROM，运行一段时间后读取运行页渲染 Worker 的统计，打印核心帧率、收到帧率、渲染帧率、丢弃帧数，以及核心发布到收到、收到到画出的延迟（平均、p95、最大）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：用法：node tools/dev/latency-run.mjs [--rom=相对 core/test/data 或绝对路径] [--port=9303] [--seconds=30] [--warmup=3] [--no-build] [--screenshots=目录]
 *       统计在渲染 Worker 里，由开发期才有的 window.emberRuntimeDiagnostics 读出（见 src/ui/runtime/RuntimeScreen.tsx）；
 *       测量的口径见 docs/01-软件架构设计.md 第 7 节：“核心发布”是核心宿主发出帧的时刻，“画出”是 WebGL 绘制命令提交完成的时刻，不含合成器与显示器的延迟；
 *       跨进程的发布到收到用 Date.now()（毫秒，分辨率 1 毫秒）；只结束本脚本启动的进程树（按 PID），不按进程名结束任何进程。退出码 0 表示测量完成。
 * ------------------------------------------------------------------------
 */

import { spawnSync } from 'node:child_process';
import { mkdirSync } from 'node:fs';
import { isAbsolute, join } from 'node:path';
import { connectCdp, parseArguments, projectRoot, saveScreenshot, sleep, startDevApp, waitFor } from './dev-support.mjs';

/** 默认的测试 ROM：nestest。 */
const DEFAULT_ROM = 'nestest/nestest.nes';
/** 判断“满足 60 帧每秒”：渲染帧率不低于核心帧率的这个比例。 */
const FPS_RATIO_LIMIT = 0.98;
/** 判断“没有明显丢帧”：各种丢帧合计不超过收到帧数的这个比例。 */
const DROP_RATIO_LIMIT = 0.01;
/** 判断“延迟可接受”：核心发布到画出的 p95 不超过两帧的时间（毫秒）。 */
const LATENCY_LIMIT_MS = 2 * (1000 / 60.0988);

const options = parseArguments(process.argv.slice(2));
const port = Number(options.get('port') ?? 9303);
const seconds = Number(options.get('seconds') ?? 30);
const warmup = Number(options.get('warmup') ?? 3);
const romOption = options.get('rom') ?? DEFAULT_ROM;
const romPath = isAbsolute(romOption) ? romOption : join(projectRoot, 'core', 'test', 'data', romOption);
const screenshotDirectory = options.get('screenshots');
if (screenshotDirectory !== undefined) mkdirSync(screenshotDirectory, { recursive: true });

/** 毫秒数保留两位小数。 */
const ms = (value) => `${value.toFixed(2)} 毫秒`;

/** 打印一组延迟的摘要。 */
function printLatency(title, summary) {
  console.log(`${title}：平均 ${ms(summary.average)}，p95 ${ms(summary.p95)}，最大 ${ms(summary.max)}，最小 ${ms(summary.min)}（${summary.count} 帧）`);
}

let app;

async function main() {
  if (!options.has('no-build')) {
    for (const script of ['schema', 'core:build']) {
      const run = spawnSync('npm', ['run', script], { cwd: projectRoot, shell: true, stdio: 'inherit' });
      if (run.status !== 0) throw new Error(`npm run ${script} 失败`);
    }
  }
  console.log(`启动开发版：ROM=${romPath}，调试端口=${port}，预热 ${warmup} 秒，测量 ${seconds} 秒`);
  app = startDevApp({ port, romPath });
  const target = await app.waitForPage();
  const page = await connectCdp(target.webSocketDebuggerUrl);
  await page.send('Runtime.enable');

  const ready = await waitFor(
    () => page.evaluate('typeof window.emberRuntimeDiagnostics === "object" && typeof window.ember === "object"').catch(() => false),
    (value) => value === true,
    60_000
  );
  if (ready !== true) throw new Error('运行页没有暴露诊断（需要开发版）');
  const running = await waitFor(
    () => page.evaluate('window.ember.core.runtime.status({})').catch(() => undefined),
    (status) => status !== undefined && status.power === 'on' && status.mode === 'running',
    60_000
  );
  if (running?.mode !== 'running') throw new Error('卡带没有运行起来');

  await sleep(warmup * 1000);
  const start = await page.evaluate('window.emberRuntimeDiagnostics.read(true)');
  if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'latency-start.png'));
  const environment = await page.evaluate(`({ devicePixelRatio: window.devicePixelRatio, userAgent: navigator.userAgent, visible: document.visibilityState, focus: document.hasFocus() })`);
  await sleep(seconds * 1000);
  const result = await page.evaluate('window.emberRuntimeDiagnostics.read()');
  const status = await page.evaluate('window.ember.core.runtime.status({})');
  if (screenshotDirectory !== undefined) await saveScreenshot(page, join(screenshotDirectory, 'latency-end.png'));

  const lost = result.dropped + result.hostDropped + result.coreMissed + result.invalid;
  const lostRatio = result.received + result.hostDropped + result.coreMissed === 0 ? 0 : lost / (result.received + result.hostDropped + result.coreMissed);
  const totalP95 = result.publishToReceive.p95 + result.receiveToRender.p95;

  console.log('\n延迟与丢帧测量报告');
  console.log(`测量时长：${(result.spanMs / 1000).toFixed(1)} 秒（预热 ${warmup} 秒后开始，开始时画布 ${start.canvasWidth}x${start.canvasHeight}，设备像素比 ${environment.devicePixelRatio}，页面${environment.visible === 'visible' ? '可见' : '不可见'}）`);
  console.log(`运行状态：帧号 ${status.frame}，电源 ${status.power}，模式 ${status.mode}`);
  console.log(`核心帧率：${result.coreFps.toFixed(2)} 帧/秒（目标 60.10）`);
  console.log(`收到帧率：${result.receivedFps.toFixed(2)} 帧/秒`);
  console.log(`渲染帧率：${result.renderedFps.toFixed(2)} 帧/秒（浏览器的画面节拍 requestAnimationFrame 约 ${result.tickFps.toFixed(1)} 次/秒）`);
  console.log(`收到 ${result.received} 帧，渲染 ${result.rendered} 帧；渲染 Worker 丢弃（被新帧顶掉）${result.dropped} 帧，核心宿主背压丢弃 ${result.hostDropped} 帧，帧号缺口 ${result.coreMissed} 帧，无效 ${result.invalid} 帧`);
  printLatency('核心发布到收到的延迟', result.publishToReceive);
  printLatency('收到到画出的延迟', result.receiveToRender);
  console.log(`两段之和（p95 相加，保守估计）：${ms(totalP95)}`);
  console.log(`画面：${result.frameWidth}x${result.frameHeight}，非黑像素 ${(result.lastFrameNonBlackRatio * 100).toFixed(1)}%`);

  console.log('\n结论');
  console.log(`满足 60 帧每秒：${result.renderedFps >= result.coreFps * FPS_RATIO_LIMIT ? '是' : '否'}（渲染帧率 / 核心帧率 = ${((result.renderedFps / result.coreFps) * 100).toFixed(1)}%）`);
  console.log(`没有明显丢帧：${lostRatio <= DROP_RATIO_LIMIT ? '是' : '否'}（丢失合计 ${lost} 帧，占 ${(lostRatio * 100).toFixed(2)}%）`);
  console.log(`延迟可接受（核心发布到画出的 p95 不超过两帧 ${LATENCY_LIMIT_MS.toFixed(1)} 毫秒）：${totalP95 <= LATENCY_LIMIT_MS ? '是' : '否'}`);
  console.log(`\nJSON：${JSON.stringify(result)}`);
  page.close();
}

let failure;
try {
  await main();
} catch (error) {
  failure = error;
  console.error(`测量脚本出错：${error instanceof Error ? error.message : String(error)}`);
  if (app?.output) console.error(`开发版最近的输出：\n${app.output}`);
} finally {
  app?.stop();
}
process.exit(failure === undefined ? 0 : 1);
