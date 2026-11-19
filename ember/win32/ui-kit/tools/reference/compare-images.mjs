/*
 * ------------------------------------------------------------------------
 * 名称：compare-images.mjs
 * 说明：截图对照用的纯函数：控件位置对照、像素对照（可排除文字区域）、内容外框（文字与图标占的范围）提取。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：像素是 RGBA，每像素 4 字节；矩形都是设备像素 { left, top, right, bottom }，右、下为开区间。
 * ------------------------------------------------------------------------
 */

/**
 * 对照两组控件位置（逻辑像素），返回每个控件各边的最大偏差。
 * @param {{ x: number, y: number, width: number, height: number }[]} expected 网页版的位置。
 * @param {{ x: number, y: number, width: number, height: number }[]} actual 原生版的位置。
 * @returns {{ maxDelta: number, deltas: { index: number, x: number, y: number, width: number, height: number }[] }} 偏差。
 */
export function compareLayout(expected, actual) {
  if (expected.length !== actual.length) throw new Error(`控件个数不同：网页版 ${expected.length} 个，原生版 ${actual.length} 个`);
  let maxDelta = 0;
  const deltas = expected.map((rect, index) => {
    const other = actual[index];
    const delta = { index, x: other.x - rect.x, y: other.y - rect.y, width: other.width - rect.width, height: other.height - rect.height };
    maxDelta = Math.max(maxDelta, Math.abs(delta.x), Math.abs(delta.y), Math.abs(delta.width), Math.abs(delta.height));
    return delta;
  });
  return { maxDelta, deltas };
}

/**
 * 把图裁成指定大小（保留左上角）：网页版窗口的像素大小会被系统取整，可能比逻辑大小乘缩放比例略大。
 * @param {{ width: number, height: number, data: Uint8Array }} image 原图。
 * @param {number} width 目标宽度，不能大于原图。
 * @param {number} height 目标高度，不能大于原图。
 * @returns {{ width: number, height: number, data: Uint8Array }} 裁出的图。
 */
export function cropTo(image, width, height) {
  if (width > image.width || height > image.height) throw new Error(`无法把 ${image.width}×${image.height} 的图裁成 ${width}×${height}`);
  const data = new Uint8Array(width * height * 4);
  for (let y = 0; y < height; y += 1) data.set(image.data.subarray(y * image.width * 4, (y * image.width + width) * 4), y * width * 4);
  return { width, height, data };
}

/**
 * 判断点是否落在任一排除矩形内。
 * @param {number} x 横坐标。
 * @param {number} y 纵坐标。
 * @param {{ left: number, top: number, right: number, bottom: number }[]} masks 排除矩形。
 * @returns {boolean} 是否被排除。
 */
function masked(x, y, masks) {
  return masks.some((mask) => x >= mask.left && x < mask.right && y >= mask.top && y < mask.bottom);
}

/**
 * 逐像素对照两张同样大小的图，排除指定的矩形。
 * @param {{ width: number, height: number, data: Uint8Array }} expected 网页版截图。
 * @param {{ width: number, height: number, data: Uint8Array }} actual 原生版截图。
 * @param {{ left: number, top: number, right: number, bottom: number }[]} masks 不参与对照的矩形。
 * @param {number} tolerance 单个通道的容差，超过算“不一致”。
 * @returns {{ total: number, mismatched: number, maxDiff: number, worst: { x: number, y: number } | null }} 结果。
 */
export function comparePixels(expected, actual, masks, tolerance) {
  if (expected.width !== actual.width || expected.height !== actual.height) throw new Error(`截图大小不同：网页版 ${expected.width}×${expected.height}，原生版 ${actual.width}×${actual.height}`);
  let total = 0;
  let mismatched = 0;
  let maxDiff = 0;
  let worst = null;
  for (let y = 0; y < expected.height; y += 1) {
    for (let x = 0; x < expected.width; x += 1) {
      if (masked(x, y, masks)) continue;
      total += 1;
      const offset = (y * expected.width + x) * 4;
      let diff = 0;
      for (let channel = 0; channel < 3; channel += 1) diff = Math.max(diff, Math.abs(expected.data[offset + channel] - actual.data[offset + channel]));
      if (diff > tolerance) mismatched += 1;
      if (diff > maxDiff) {
        maxDiff = diff;
        worst = { x, y };
      }
    }
  }
  return { total, mismatched, maxDiff, worst };
}

/**
 * 提取矩形内“和背景不同”的像素的外框，用来比较文字与图标占据的范围。
 * @param {{ width: number, height: number, data: Uint8Array }} image 截图。
 * @param {{ left: number, top: number, right: number, bottom: number }} area 搜索范围。
 * @param {number[]} background 背景色 [r, g, b]。
 * @param {number} threshold 与背景的通道差之和超过它才算内容。
 * @returns {{ left: number, top: number, right: number, bottom: number } | null} 外框；没有内容时为空。
 */
export function contentBox(image, area, background, threshold) {
  let left = Infinity;
  let top = Infinity;
  let right = -Infinity;
  let bottom = -Infinity;
  for (let y = area.top; y < area.bottom; y += 1) {
    for (let x = area.left; x < area.right; x += 1) {
      const offset = (y * image.width + x) * 4;
      const distance = Math.abs(image.data[offset] - background[0]) + Math.abs(image.data[offset + 1] - background[1]) + Math.abs(image.data[offset + 2] - background[2]);
      if (distance <= threshold) continue;
      left = Math.min(left, x);
      top = Math.min(top, y);
      right = Math.max(right, x + 1);
      bottom = Math.max(bottom, y + 1);
    }
  }
  return left === Infinity ? null : { left, top, right, bottom };
}
