// ------------------------------------------------------------------------
// 名称：fs-utils.mjs
// 说明：文件系统小工具：读 JSON、取修改时间、递归创建目录。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：无
// ------------------------------------------------------------------------
import { mkdirSync, readFileSync, statSync } from 'node:fs';
import { FcdevError } from './errors.mjs';

/**
 * 读取并解析 JSON 文件。
 * @param {string} file 文件路径
 * @returns {unknown} 解析后的值
 * @throws {FcdevError} 文件不存在或内容不是合法的 JSON
 */
export function readJsonFile(file) {
  let text;
  try {
    text = readFileSync(file, 'utf8');
  } catch (error) {
    throw new FcdevError(`无法读取文件 ${file}`, { code: 'FILE_READ_FAILED', cause: error });
  }
  try {
    return JSON.parse(text);
  } catch (error) {
    throw new FcdevError(`${file} 不是合法的 JSON`, { code: 'JSON_INVALID', cause: error });
  }
}

/**
 * 取文件的修改时间。
 * @param {string} file 文件路径
 * @returns {number|null} 毫秒时间戳；文件不存在时为 null
 */
export function getModifiedTime(file) {
  try {
    return statSync(file).mtimeMs;
  } catch (error) {
    if (error.code === 'ENOENT') return null;
    throw error;
  }
}

/**
 * 判断路径是否是已存在的目录。
 * @param {string} target 路径
 * @returns {boolean} 是目录时为 true；不存在时为 false
 */
export function isDirectory(target) {
  try {
    return statSync(target).isDirectory();
  } catch (error) {
    if (error.code === 'ENOENT') return false;
    throw error;
  }
}

/**
 * 递归创建目录，已存在时不报错。
 * @param {string} dir 目录路径
 */
export function ensureDirectory(dir) {
  mkdirSync(dir, { recursive: true });
}
