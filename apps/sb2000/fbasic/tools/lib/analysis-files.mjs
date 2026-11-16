// ------------------------------------------------------------------------
// 名称：analysis-files.mjs
// 说明：读取 analysis 目录里的人工文件：地址列表与区段列表。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：# 开头的行与行内 # 之后是注释；地址是十六进制，可带 $ 前缀；区段写作 起-止（含止）。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';

/**
 * 去掉注释与空行。
 * @param {string} file 文件路径
 * @returns {string[]} 有内容的行
 */
function contentLines(file) {
  return readFileSync(file, 'utf8')
    .split(/\r?\n/)
    .map((line) => line.replace(/#.*$/, '').trim())
    .filter((line) => line !== '');
}

/**
 * 读取地址列表。
 * @param {string} file 文件路径
 * @returns {number[]} 地址；每行取第一个十六进制数
 */
export function readAddressList(file) {
  return contentLines(file).map((line) => Number.parseInt(line.split(/\s+/)[0].replace('$', ''), 16));
}

/**
 * 读取区段列表。
 * @param {string} file 文件路径
 * @returns {{start: number, end: number, label: string}[]} 区段与其后的说明
 */
export function readRangeList(file) {
  return contentLines(file).map((line) => {
    const match = /^\$?([0-9A-Fa-f]{4})\s*-\s*\$?([0-9A-Fa-f]{4})\s*(.*)$/.exec(line);
    if (!match) throw new Error(`${file}：区段格式不对：${line}`);
    return { start: Number.parseInt(match[1], 16), end: Number.parseInt(match[2], 16), label: match[3] };
  });
}
