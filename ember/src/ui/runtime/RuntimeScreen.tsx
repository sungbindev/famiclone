/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeScreen.tsx
 * 说明：运行页的画面组件：一个可聚焦的画面区，里面的画布交给渲染 Worker（OffscreenCanvas + WebGL）；接上画面端口，采集键盘送给核心。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：画布由本组件在效果里创建（transferControlToOffscreen 对同一个画布只能调用一次，严格模式会重复执行效果）；
 *       画面区跟随容器与设备像素比调整画布的设备像素尺寸，画面比例与缩放方式由属性决定，画面在 Worker 里按比例居中、周围黑底。
 *       键盘事件只在画面区获得焦点时采集；窗口失焦、页面隐藏与卸载时松开全部按键。
 *       开发期（import.meta.env.DEV）把诊断读取函数挂到 window.emberRuntimeDiagnostics，供 tools/dev 的脚本经 CDP 使用，正式包没有。
 * ------------------------------------------------------------------------
 */

import { useNotify } from '@rujianjs/ui-kit';
import { useCallback, useEffect, useLayoutEffect, useMemo, useRef, type RefObject } from 'react';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { DiagnosticsClient } from './diagnostics-client';
import { KeyboardCapture } from './key-capture';
import { DEFAULT_SCREEN_ASPECT, DEFAULT_SCREEN_SCALING, type ScreenAspect, type ScreenScaling } from './screen-layout';
import { useStreamPorts } from './use-stream-ports';
import { parseWorkerReply, type WorkerDiagnostics, type WorkerRequest } from './worker-messages';
import './ember-run.css';

declare global {
  interface Window {
    /** 开发期的诊断读取：reset 为 true 时先清空统计。 */
    emberRuntimeDiagnostics?: { read(reset?: boolean): Promise<WorkerDiagnostics> };
  }
}

const TEXT = {
  label: '模拟器画面',
  workerFailed: '无法启动画面渲染'
} as const;

/** 画面组件的属性。 */
export interface RuntimeScreenProps {
  /** 预加载脚本暴露的接口。 */
  readonly bridge: EmberBridge;
  /** 画面比例，默认 4:3。 */
  readonly aspect?: ScreenAspect;
  /** 缩放方式，默认适应窗口。 */
  readonly scaling?: ScreenScaling;
  /** 画面区元素的引用：页面用它在操作后把焦点还给画面。 */
  readonly screenRef: RefObject<HTMLDivElement | null>;
}

/** 容器的设备像素尺寸：优先取浏览器给的设备像素内容框，没有时按设备像素比换算。 */
function measureDevicePixels(container: HTMLElement, entry: ResizeObserverEntry | undefined): { width: number; height: number } {
  const box = entry?.devicePixelContentBoxSize?.[0];
  if (box !== undefined) return { width: Math.round(box.inlineSize), height: Math.round(box.blockSize) };
  const rect = container.getBoundingClientRect();
  const ratio = window.devicePixelRatio || 1;
  return { width: Math.round(rect.width * ratio), height: Math.round(rect.height * ratio) };
}

/**
 * 画面组件。
 * @param props 属性。
 */
export function RuntimeScreen({ bridge, aspect = DEFAULT_SCREEN_ASPECT, scaling = DEFAULT_SCREEN_SCALING, screenRef }: RuntimeScreenProps) {
  const notify = useNotify();
  const workerRef = useRef<Worker | undefined>(undefined);
  const sizeRef = useRef({ width: 0, height: 0 });
  const optionsRef = useRef({ aspect, scaling });

  /** 把当前的画布尺寸与显示选项发给 Worker。 */
  const postView = useCallback(() => {
    const request: WorkerRequest = { type: 'view', view: { ...sizeRef.current, ...optionsRef.current } };
    workerRef.current?.postMessage(request);
  }, []);

  // 创建画布与 Worker，跟随容器尺寸与设备像素比；先于端口的效果声明，保证端口到达时 Worker 已存在。
  useEffect(() => {
    const container = screenRef.current;
    if (container === null) return undefined;
    const canvas = document.createElement('canvas');
    canvas.className = 'ember-run-screen__canvas';
    container.append(canvas);
    let worker: Worker;
    try {
      const offscreen = canvas.transferControlToOffscreen();
      worker = new Worker(new URL('./render-worker.ts', import.meta.url), { type: 'module' });
      const init: WorkerRequest = { type: 'init', canvas: offscreen };
      worker.postMessage(init, [offscreen]);
    } catch (error) {
      canvas.remove();
      notify(`${TEXT.workerFailed}：${error instanceof Error ? error.message : String(error)}`, 'error');
      return undefined;
    }
    workerRef.current = worker;
    const diagnostics = new DiagnosticsClient((request) => worker.postMessage(request));
    worker.addEventListener('message', (event) => {
      const reply = parseWorkerReply(event.data);
      if (reply?.type === 'failed') notify(reply.message, 'error');
      else if (reply?.type === 'diagnostics') diagnostics.handleReply(reply.id, reply.data);
    });
    if (import.meta.env.DEV) window.emberRuntimeDiagnostics = { read: (reset) => diagnostics.read(reset) };

    const observer = new ResizeObserver((entries) => {
      sizeRef.current = measureDevicePixels(container, entries[entries.length - 1]);
      postView();
    });
    try {
      observer.observe(container, { box: 'device-pixel-content-box' });
    } catch {
      observer.observe(container);
    }

    // 设备像素比变化（缩放页面、换显示器）时重新量一次；每次变化后重新监听新的比值。
    let ratioQuery: MediaQueryList | undefined;
    const onRatioChange = (): void => {
      sizeRef.current = measureDevicePixels(container, undefined);
      postView();
      listenRatio();
    };
    function listenRatio(): void {
      ratioQuery = window.matchMedia(`(resolution: ${window.devicePixelRatio}dppx)`);
      ratioQuery.addEventListener('change', onRatioChange, { once: true });
    }
    listenRatio();

    return () => {
      ratioQuery?.removeEventListener('change', onRatioChange);
      observer.disconnect();
      diagnostics.dispose();
      if (import.meta.env.DEV) delete window.emberRuntimeDiagnostics;
      worker.terminate();
      workerRef.current = undefined;
      canvas.remove();
    };
  }, [notify, postView, screenRef]);

  // 画面比例或缩放方式变了：重新发版面。
  useEffect(() => {
    optionsRef.current = { aspect, scaling };
    postView();
  }, [aspect, scaling, postView]);

  // 画面端口交给 Worker；Worker 不存在时关闭端口。
  const onFramePort = useCallback((port: MessagePort) => {
    const worker = workerRef.current;
    if (worker === undefined) {
      port.close();
      return;
    }
    const request: WorkerRequest = { type: 'attach', port };
    worker.postMessage(request, [port]);
  }, []);
  const input = useStreamPorts(bridge, onFramePort);
  const capture = useMemo(() => new KeyboardCapture((port, buttons) => input.sendButtons(port, buttons)), [input]);

  // 页面显示时自动聚焦；窗口失焦、页面隐藏与卸载时松开全部按键。
  // 用 useLayoutEffect：卸载时它的清理先于普通效果，松开的消息先发出，输入端口随后才关闭。
  useLayoutEffect(() => {
    screenRef.current?.focus({ preventScroll: true });
    const release = (): void => capture.releaseAll();
    const onVisibility = (): void => {
      if (document.visibilityState === 'hidden') capture.releaseAll();
    };
    window.addEventListener('blur', release);
    document.addEventListener('visibilitychange', onVisibility);
    return () => {
      window.removeEventListener('blur', release);
      document.removeEventListener('visibilitychange', onVisibility);
      capture.releaseAll();
    };
  }, [capture, screenRef]);

  return (
    <div
      ref={screenRef}
      className="ember-run-screen"
      tabIndex={0}
      role="application"
      aria-label={TEXT.label}
      onKeyDown={(event) => {
        if (capture.keyDown(event.code, event.repeat, event.target)) event.preventDefault();
      }}
      onKeyUp={(event) => {
        if (capture.keyUp(event.code, event.target)) event.preventDefault();
      }}
      onBlur={() => capture.releaseAll()}
    />
  );
}
