/*
 * ------------------------------------------------------------------------
 * 名称：png.mjs
 * 说明：最小的 PNG 解码器：只支持 8 位的 RGB 与 RGBA、不隔行扫描，够读 Electron 截图与 WIC 输出的 PNG。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：解码结果统一为 RGBA（每像素 4 字节）；遇到不支持的格式直接抛异常。
 * ------------------------------------------------------------------------
 */

import { readFileSync } from 'node:fs';
import { inflateSync } from 'node:zlib';

const SIGNATURE = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);

/**
 * Paeth 预测。
 * @param {number} left 左边的字节。
 * @param {number} up 上边的字节。
 * @param {number} upLeft 左上的字节。
 * @returns {number} 预测值。
 */
function paeth(left, up, upLeft) {
  const estimate = left + up - upLeft;
  const distanceLeft = Math.abs(estimate - left);
  const distanceUp = Math.abs(estimate - up);
  const distanceUpLeft = Math.abs(estimate - upLeft);
  if (distanceLeft <= distanceUp && distanceLeft <= distanceUpLeft) return left;
  return distanceUp <= distanceUpLeft ? up : upLeft;
}

/**
 * 解码 PNG 文件。
 * @param {string} file 文件路径。
 * @returns {{ width: number, height: number, data: Uint8Array }} 宽、高与 RGBA 像素。
 */
export function decodePng(file) {
  const buffer = readFileSync(file);
  if (!buffer.subarray(0, 8).equals(SIGNATURE)) throw new Error(`${file} 不是 PNG 文件`);
  let width = 0;
  let height = 0;
  let channels = 0;
  const parts = [];
  for (let offset = 8; offset < buffer.length; ) {
    const length = buffer.readUInt32BE(offset);
    const type = buffer.toString('ascii', offset + 4, offset + 8);
    const body = buffer.subarray(offset + 8, offset + 8 + length);
    if (type === 'IHDR') {
      width = body.readUInt32BE(0);
      height = body.readUInt32BE(4);
      const depth = body[8];
      const colorType = body[9];
      if (depth !== 8 || (colorType !== 2 && colorType !== 6) || body[12] !== 0) throw new Error(`${file} 的 PNG 格式不受支持（位深 ${depth}、颜色类型 ${colorType}、隔行 ${body[12]}）`);
      channels = colorType === 6 ? 4 : 3;
    } else if (type === 'IDAT') {
      parts.push(body);
    } else if (type === 'IEND') {
      break;
    }
    offset += 12 + length;
  }
  const raw = inflateSync(Buffer.concat(parts));
  const stride = width * channels;
  const rows = new Uint8Array(stride * height);
  for (let y = 0; y < height; y += 1) {
    const filter = raw[y * (stride + 1)];
    const source = y * (stride + 1) + 1;
    for (let x = 0; x < stride; x += 1) {
      const left = x >= channels ? rows[y * stride + x - channels] : 0;
      const up = y > 0 ? rows[(y - 1) * stride + x] : 0;
      const upLeft = y > 0 && x >= channels ? rows[(y - 1) * stride + x - channels] : 0;
      let value = raw[source + x];
      if (filter === 1) value += left;
      else if (filter === 2) value += up;
      else if (filter === 3) value += (left + up) >> 1;
      else if (filter === 4) value += paeth(left, up, upLeft);
      else if (filter !== 0) throw new Error(`${file} 有未知的滤波类型 ${filter}`);
      rows[y * stride + x] = value & 0xff;
    }
  }
  if (channels === 4) return { width, height, data: rows };
  const data = new Uint8Array(width * height * 4);
  for (let i = 0; i < width * height; i += 1) {
    data[i * 4] = rows[i * 3];
    data[i * 4 + 1] = rows[i * 3 + 1];
    data[i * 4 + 2] = rows[i * 3 + 2];
    data[i * 4 + 3] = 255;
  }
  return { width, height, data };
}
