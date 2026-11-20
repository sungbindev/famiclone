// ------------------------------------------------------------------------
// 名称：project-store.mjs
// 说明：反汇编工程在磁盘上的读写：工程目录里的 disasm.json、映像文件与符号集文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：工程里的映像与符号集路径相对工程目录，用 / 分隔，跨盘符时才写绝对路径；工程文件只由这里读写，其余代码只处理内存里的工程。
// ------------------------------------------------------------------------
import { mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../../shared/index.mjs';
import { isImageStale, parseProject, serializeProject } from '../model/index.mjs';

/** 工程文件名 */
export const PROJECT_FILE = 'disasm.json';

/**
 * 把文件路径写成相对工程目录的形式，保存进工程。
 * @param {string} projectDir 工程目录
 * @param {string} file 文件的绝对路径
 * @returns {string} 相对路径（/ 分隔）；跨盘符时为原路径
 */
export function toProjectPath(projectDir, file) {
  const relative = path.relative(projectDir, file);
  return path.isAbsolute(relative) ? file : relative.split(path.sep).join('/');
}

/**
 * 把工程里保存的路径还原成绝对路径。
 * @param {string} projectDir 工程目录
 * @param {string} stored 工程里保存的路径
 * @returns {string} 绝对路径
 */
export function fromProjectPath(projectDir, stored) {
  return path.resolve(projectDir, stored);
}

/**
 * 保存工程文件，目录不存在时创建。
 * @param {string} projectDir 工程目录
 * @param {import('../model/project.mjs').Project} project 工程
 * @returns {string} 工程文件的路径
 */
export function writeProject(projectDir, project) {
  mkdirSync(projectDir, { recursive: true });
  const file = path.join(projectDir, PROJECT_FILE);
  writeFileSync(file, serializeProject(project), 'utf8');
  return file;
}

/**
 * 读取工程文件。
 * @param {string} projectDir 工程目录
 * @returns {import('../model/project.mjs').Project} 工程
 * @throws {FcdevError} 目录里没有工程文件，或工程文件不合法
 */
export function readProject(projectDir) {
  const file = path.join(projectDir, PROJECT_FILE);
  let text;
  try {
    text = readFileSync(file, 'utf8');
  } catch (error) {
    throw new FcdevError(`读不到工程文件 ${file}`, { code: 'DISASM_PROJECT_MISSING', hint: '先用 fcdev disasm 生成工程', cause: error });
  }
  return parseProject(text);
}

/**
 * 读取工程的映像，并检查它与工程记录的一致。
 * @param {string} projectDir 工程目录
 * @param {import('../model/project.mjs').Project} project 工程
 * @returns {Buffer} 映像字节
 * @throws {FcdevError} 读不到映像，或映像与工程记录的摘要不符
 */
export function readProjectImage(projectDir, project) {
  const file = fromProjectPath(projectDir, project.image.path);
  let bytes;
  try {
    bytes = readFileSync(file);
  } catch (error) {
    throw new FcdevError(`读不到映像文件 ${file}`, { code: 'DISASM_IMAGE_MISSING', hint: '映像被移动或删除；工程里记录的路径相对工程目录', cause: error });
  }
  if (isImageStale(project, bytes)) {
    throw new FcdevError(`映像文件 ${file} 与工程记录的不一致`, { code: 'DISASM_IMAGE_MISMATCH', hint: '映像文件变了，工程已经过期' });
  }
  return bytes;
}

/**
 * 读取工程登记的符号集内容。
 * @param {string} projectDir 工程目录
 * @param {import('../model/project.mjs').Project} project 工程
 * @returns {Map<string, string>} 符号集文件名到文本
 * @throws {FcdevError} 读不到某个符号集文件
 */
export function readProjectSymbolSets(projectDir, project) {
  const texts = new Map();
  for (const set of project.symbolSets) {
    const file = fromProjectPath(projectDir, set.path);
    try {
      texts.set(set.name, readFileSync(file, 'utf8'));
    } catch (error) {
      throw new FcdevError(`读不到符号集 ${set.name}：${file}`, { code: 'DISASM_SYMBOL_SET_MISSING', hint: '工程里记录的路径相对工程目录', cause: error });
    }
  }
  return texts;
}
