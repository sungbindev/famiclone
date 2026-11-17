/*
 * ------------------------------------------------------------------------
 * 名称：PageScrollbar.ts
 * 说明：帮助文档窗口的页面滚动条：隐藏系统自带的页面滚动条，在标题栏与导航栏下面画一条覆盖在内容上的滚动条，外观与行为同主窗口里的滚动区域。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：系统自带的页面滚动条是一整条贯穿窗口高度的槽，会把固定在顶部的标题栏与导航栏挤窄；这里改成覆盖式的，起点取固定在顶部的各条（标题栏、导航栏、局部导航）的下沿；
 *       滑块颜色与粗细取组件库令牌（样式在 style.css）；鼠标移入、滚动、拖动时显示，其余时候渐隐；只在页面有可滚动内容时出现。
 * ------------------------------------------------------------------------
 */

import { defineComponent, h, onBeforeUnmount, onMounted, ref } from 'vue';

/** 滑块的最小高度（像素）。 */
const MIN_THUMB_HEIGHT = 24;
/** 滚动停下后滚动条保持显示的时间（毫秒）。 */
const VISIBLE_AFTER_SCROLL_MS = 500;
/** 点轨道翻页时占一屏高度的比例。 */
const PAGE_RATIO = 0.9;
/** 顶部各条的选择器：标题栏、导航栏、局部导航。 */
const HEADER_SELECTORS = ['.ui-title-bar', '.VPNav', '.VPLocalNav'];

/** 页面滚动条。 */
export const PageScrollbar = defineComponent({
  name: 'PageScrollbar',
  setup() {
    const top = ref(0);
    const thumbTop = ref(0);
    const thumbHeight = ref(0);
    const scrollable = ref(false);
    const visible = ref(false);
    let hideTimer = 0;
    let dragging: { startY: number; startScroll: number; ratio: number } | null = null;
    let hovering = false;
    let resize: ResizeObserver | undefined;

    /** 顶部各条的最低下沿（取在窗口内的）。 */
    const headerBottom = (): number => {
      let bottom = 0;
      for (const selector of HEADER_SELECTORS) {
        const element = document.querySelector(selector);
        if (element === null) continue;
        const rect = element.getBoundingClientRect();
        if (rect.height > 0) bottom = Math.max(bottom, Math.min(rect.bottom, window.innerHeight));
      }
      return Math.max(0, bottom);
    };

    const update = (): void => {
      const root = document.documentElement;
      const range = root.scrollHeight - window.innerHeight;
      top.value = headerBottom();
      const track = window.innerHeight - top.value;
      scrollable.value = range > 1 && track > MIN_THUMB_HEIGHT;
      if (!scrollable.value) return;
      const height = Math.max(MIN_THUMB_HEIGHT, (track * window.innerHeight) / root.scrollHeight);
      thumbHeight.value = Math.min(height, track);
      thumbTop.value = (window.scrollY / range) * (track - thumbHeight.value);
    };

    const show = (): void => {
      visible.value = true;
      window.clearTimeout(hideTimer);
      if (hovering || dragging !== null) return;
      hideTimer = window.setTimeout(() => {
        visible.value = false;
      }, VISIBLE_AFTER_SCROLL_MS);
    };

    const onScroll = (): void => {
      update();
      show();
    };

    const onPointerDownThumb = (event: PointerEvent): void => {
      const root = document.documentElement;
      const range = root.scrollHeight - window.innerHeight;
      const free = window.innerHeight - top.value - thumbHeight.value;
      if (free <= 0) return;
      dragging = { startY: event.clientY, startScroll: window.scrollY, ratio: range / free };
      (event.currentTarget as HTMLElement).setPointerCapture(event.pointerId);
      event.preventDefault();
      event.stopPropagation();
      show();
    };

    const onPointerMoveThumb = (event: PointerEvent): void => {
      if (dragging === null) return;
      window.scrollTo(0, dragging.startScroll + (event.clientY - dragging.startY) * dragging.ratio);
    };

    const onPointerUpThumb = (event: PointerEvent): void => {
      dragging = null;
      (event.currentTarget as HTMLElement).releasePointerCapture(event.pointerId);
      show();
    };

    const onPointerDownTrack = (event: PointerEvent): void => {
      const direction = event.clientY < top.value + thumbTop.value ? -1 : 1;
      window.scrollBy({ top: direction * window.innerHeight * PAGE_RATIO });
    };

    onMounted(() => {
      update();
      window.addEventListener('scroll', onScroll, { passive: true });
      window.addEventListener('resize', onScroll);
      resize = new ResizeObserver(update);
      resize.observe(document.documentElement);
      resize.observe(document.body);
    });

    onBeforeUnmount(() => {
      window.clearTimeout(hideTimer);
      window.removeEventListener('scroll', onScroll);
      window.removeEventListener('resize', onScroll);
      resize?.disconnect();
    });

    return () =>
      scrollable.value
        ? h(
            'div',
            {
              class: ['page-scrollbar', visible.value && 'page-scrollbar--visible'],
              style: { top: `${top.value}px` },
              'aria-hidden': 'true',
              onPointerenter: () => {
                hovering = true;
                show();
              },
              onPointerleave: () => {
                hovering = false;
                show();
              },
              onPointerdown: onPointerDownTrack
            },
            [
              h('div', {
                class: 'page-scrollbar__thumb',
                style: { top: `${thumbTop.value}px`, height: `${thumbHeight.value}px` },
                onPointerdown: onPointerDownThumb,
                onPointermove: onPointerMoveThumb,
                onPointerup: onPointerUpThumb,
                onPointercancel: onPointerUpThumb
              })
            ]
          )
        : null;
  }
});
