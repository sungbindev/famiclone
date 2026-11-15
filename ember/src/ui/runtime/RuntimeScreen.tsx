/*
 * ------------------------------------------------------------------------
 * 名称：RuntimeScreen.tsx
 * 说明：运行页的画面组件：一个可聚焦的画面区，里面的画布交给渲染 Worker（OffscreenCanvas + WebGL）；接上画面端口，采集键盘、鼠标、游戏手柄的原始事件送给核心。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：画布由本组件在效果里创建（transferControlToOffscreen 对同一个画布只能调用一次，严格模式会重复执行效果）；
 *       画面区跟随容器与设备像素比调整画布的设备像素尺寸，画面比例与缩放方式由属性决定，画面在 Worker 里按比例居中、周围黑底。
 *       键盘事件只在画面区获得焦点时采集；窗口失焦、页面隐藏与卸载时松开全部按键。Windows 上画面区持有焦点时独占键盘（见 use-keyboard-exclusive.ts），按键由主进程以物理键位交来，页面收不到键盘事件；其他平台用页面内的键盘事件。鼠标事件发生在画面区上，坐标换算成标准帧的像素；游戏手柄每一帧读一次。
 *       界面不做任何键位映射，原始事件交给核心：键盘与手柄由核心按手柄绑定变成手柄口的按键，其余键盘与鼠标事件交给机器（卡带的 mapper 脚本）处理。
 *       开发期（import.meta.env.DEV）把诊断读取函数挂到 window.emberRuntimeDiagnostics，供 tools/dev 的脚本经 CDP 使用，正式包没有。
 * ------------------------------------------------------------------------
 */

import { useNotify } from '@rujianjs/ui-kit';
import { useCallback, useEffect, useLayoutEffect, useMemo, useRef, type RefObject } from 'react';
import type { EmberBridge } from '../../contracts/shell/shell-bridge';
import { DiagnosticsClient } from './diagnostics-client';
import { GamepadCapture } from './gamepad-capture';
import { KeyboardCapture } from './key-capture';
import { frameUnitsPerPixel, MouseCapture, toFramePoint } from './pointer-capture';
import { cropLinesFor, DEFAULT_SCREEN_ASPECT, DEFAULT_SCREEN_SCALING, type ScreenAspect, type ScreenScaling } from './screen-layout';
import { useStreamPorts } from './use-stream-ports';
import { useKeyboardExclusive } from './use-keyboard-exclusive';
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

/** 像素模式的滚轮：多少像素算一格。 */
const WHEEL_PIXELS_PER_NOTCH = 100;

/** 捕获鼠标期间，两次右键按下的间隔不超过这个毫秒数就算“双击右键”，释放捕获。 */
const DOUBLE_RIGHT_CLICK_MS = 400;

/** 画面组件的属性。 */
export interface RuntimeScreenProps {
  /** 预加载脚本暴露的接口。 */
  readonly bridge: EmberBridge;
  /** 画面比例，默认 8:7。 */
  readonly aspect?: ScreenAspect;
  /** 缩放方式，默认适应窗口。 */
  readonly scaling?: ScreenScaling;
  /** 显示全部 240 行，默认显示；关闭时上下各裁掉 8 行。 */
  readonly showAllLines?: boolean;
  /** 点击画面时捕获鼠标（指针锁定），默认关闭；捕获期间鼠标事件只有相对位移，双击右键退出（没有键盘独占的平台上按 Esc 也会退出）。 */
  readonly captureMouse?: boolean;
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
export function RuntimeScreen({ bridge, aspect = DEFAULT_SCREEN_ASPECT, scaling = DEFAULT_SCREEN_SCALING, showAllLines = true, captureMouse = false, screenRef }: RuntimeScreenProps) {
  const notify = useNotify();
  const workerRef = useRef<Worker | undefined>(undefined);
  const sizeRef = useRef({ width: 0, height: 0 });
  const optionsRef = useRef({ aspect, scaling, cropLines: cropLinesFor(showAllLines) });
  /** 捕获期间上一次右键按下的时刻（用来判断双击右键）。 */
  const lastRightDownRef = useRef(Number.NEGATIVE_INFINITY);

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
    optionsRef.current = { aspect, scaling, cropLines: cropLinesFor(showAllLines) };
    postView();
  }, [aspect, scaling, showAllLines, postView]);

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
  const keyboard = useMemo(() => new KeyboardCapture((message) => input.send(message)), [input]);
  const mouse = useMemo(() => new MouseCapture((message) => input.send(message)), [input]);
  const gamepads = useMemo(() => new GamepadCapture((message) => input.send(message)), [input]);
  // 画面区持有焦点时独占键盘（Windows）：按键经主进程交来，不再经页面的键盘事件。
  useKeyboardExclusive(bridge.shell, screenRef, keyboard);

  /** 指针在标准帧上的位置；还没有画布时为 undefined。 */
  const framePointOf = useCallback(
    (event: { clientX: number; clientY: number }) => {
      const element = screenRef.current;
      if (element === null) return undefined;
      return toFramePoint(event.clientX, event.clientY, element.getBoundingClientRect(), { canvasWidth: sizeRef.current.width, canvasHeight: sizeRef.current.height, ...optionsRef.current });
    },
    [screenRef]
  );

  /** 鼠标现在是否被画面区捕获（指针锁定）。 */
  const isCaptured = useCallback(() => screenRef.current !== null && document.pointerLockElement === screenRef.current, [screenRef]);

  // 退出捕获（按 Esc、切换窗口）时松开鼠标按键；关掉“点击画面捕获鼠标”时把已有的捕获退掉。
  useEffect(() => {
    const onLockChange = (): void => {
      if (!isCaptured()) mouse.leave(0);
    };
    document.addEventListener('pointerlockchange', onLockChange);
    if (!captureMouse && isCaptured()) document.exitPointerLock?.();
    return () => {
      document.removeEventListener('pointerlockchange', onLockChange);
      if (isCaptured()) document.exitPointerLock?.();
    };
  }, [captureMouse, isCaptured, mouse]);

  // 页面显示时自动聚焦；窗口失焦、页面隐藏与卸载时松开全部按键。
  // 用 useLayoutEffect：卸载时它的清理先于普通效果，松开的消息先发出，输入端口随后才关闭。
  useLayoutEffect(() => {
    screenRef.current?.focus({ preventScroll: true });
    const release = (): void => keyboard.releaseAll();
    const onVisibility = (): void => {
      if (document.visibilityState === 'hidden') keyboard.releaseAll();
    };
    window.addEventListener('blur', release);
    document.addEventListener('visibilitychange', onVisibility);
    return () => {
      window.removeEventListener('blur', release);
      document.removeEventListener('visibilitychange', onVisibility);
      keyboard.releaseAll();
    };
  }, [keyboard, screenRef]);

  // 游戏手柄没有事件可监听，每一帧读一次，变化时上报；卸载时上报断开（同样用 useLayoutEffect，在输入端口关闭之前）。
  useLayoutEffect(() => {
    if (typeof navigator.getGamepads !== 'function') return undefined;
    let frame = 0;
    const tick = (): void => {
      gamepads.poll(navigator.getGamepads());
      frame = window.requestAnimationFrame(tick);
    };
    frame = window.requestAnimationFrame(tick);
    return () => {
      window.cancelAnimationFrame(frame);
      gamepads.disconnectAll();
    };
  }, [gamepads]);

  return (
    <div
      ref={screenRef}
      className="ember-run-screen"
      tabIndex={0}
      role="application"
      aria-label={TEXT.label}
      onKeyDown={(event) => {
        if (keyboard.keyDown(event.code, event.repeat, event.target, event)) event.preventDefault();
      }}
      onKeyUp={(event) => {
        if (keyboard.keyUp(event.code)) event.preventDefault();
      }}
      onBlur={() => keyboard.releaseAll()}
      onPointerMove={(event) => {
        if (isCaptured()) {
          const element = screenRef.current;
          const scale = element === null ? undefined : frameUnitsPerPixel(element.getBoundingClientRect(), { canvasWidth: sizeRef.current.width, canvasHeight: sizeRef.current.height, ...optionsRef.current });
          if (scale !== undefined) mouse.moveBy('move', event.movementX * scale.x, event.movementY * scale.y, event.buttons);
          return;
        }
        // 开启了鼠标捕获的机器，没捕获时鼠标不进画面，里面的鼠标不跟着动。
        if (captureMouse) return;
        const point = framePointOf(event);
        if (point !== undefined) mouse.report('move', point, event.buttons);
      }}
      onPointerDown={(event) => {
        if (isCaptured()) {
          if (event.button === 2) {
            const doubled = event.timeStamp - lastRightDownRef.current <= DOUBLE_RIGHT_CLICK_MS;
            lastRightDownRef.current = doubled ? Number.NEGATIVE_INFINITY : event.timeStamp;
            if (doubled) {
              document.exitPointerLock?.();
              return;
            }
          }
          mouse.moveBy('down', 0, 0, event.buttons);
          return;
        }
        if (captureMouse) {
          // 第一次点击只用来捕获，不把这次按下送给机器。
          screenRef.current?.requestPointerLock?.();
          return;
        }
        const point = framePointOf(event);
        // 按住拖到画面之外再松开，松开事件也要到这里。
        event.currentTarget.setPointerCapture?.(event.pointerId);
        if (point !== undefined) mouse.report('down', point, event.buttons);
      }}
      onPointerUp={(event) => {
        if (isCaptured()) {
          mouse.moveBy('up', 0, 0, event.buttons);
          return;
        }
        if (captureMouse) return;
        const point = framePointOf(event);
        if (point !== undefined) mouse.report('up', point, event.buttons);
      }}
      onPointerLeave={(event) => {
        if (!isCaptured() && !captureMouse) mouse.leave(event.buttons);
      }}
      onWheel={(event) => {
        const amount = event.deltaMode === 1 ? event.deltaY : event.deltaY / WHEEL_PIXELS_PER_NOTCH;
        if (isCaptured()) {
          mouse.moveBy('wheel', 0, 0, event.buttons, amount);
          return;
        }
        if (captureMouse) return;
        const point = framePointOf(event);
        if (point !== undefined) mouse.report('wheel', point, event.buttons, amount);
      }}
      onContextMenu={(event) => event.preventDefault()}
    />
  );
}
