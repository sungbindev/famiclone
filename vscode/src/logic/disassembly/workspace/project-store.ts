// ------------------------------------------------------------------------
// 名称：project-store.ts
// 说明：反汇编工程在磁盘上的读写：工程目录里的 disasm.json、映像文件与符号集文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：工程里的映像与符号集路径相对工程目录，用 / 分隔，跨盘符时才写绝对路径；工程文件只由这里读
//       写，其余代码只处理内存里的工程。
// ------------------------------------------------------------------------
import * as path from 'node:path';
import type { FileSystem } from '../../../contracts/files/file-system';
import { FcdevError } from '../../../shared/errors/fcdev-error';
import { isImageStale, parseProject, serializeProject } from '../../../shared/disassembly/model/index';
import type { Project } from '../../../shared/disassembly/model/project';

/** 工程文件名 */
export const PROJECT_FILE = 'disasm.json';

/**
 * 把文件路径写成相对工程目录的形式，保存进工程。
 * @param projectDir 工程目录
 * @param file 文件的绝对路径
 * @returns 相对路径（/ 分隔）；跨盘符时为原路径
 */
export function toProjectPath(projectDir: string, file: string): string {
  const relative = path.relative(projectDir, file);
  return path.isAbsolute(relative) ? file : relative.split(path.sep).join('/');
}

/**
 * 把工程里保存的路径还原成绝对路径。
 * @param projectDir 工程目录
 * @param stored 工程里保存的路径
 * @returns 绝对路径
 */
export function fromProjectPath(projectDir: string, stored: string): string {
  return path.resolve(projectDir, stored);
}

/**
 * 把字节序列包成 Buffer（不复制），供需要 Buffer 方法的解析器使用。
 * @param bytes 字节序列
 * @returns 同一块内存的 Buffer
 */
export function toBuffer(bytes: Uint8Array): Buffer {
  return Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength);
}

/**
 * 保存工程文件，目录不存在时创建。
 * @param files 文件系统
 * @param projectDir 工程目录
 * @param project 工程
 * @returns 工程文件的路径
 */
export function writeProject(files: FileSystem, projectDir: string, project: Project): string {
  files.ensureDirectory(projectDir);
  const file = path.join(projectDir, PROJECT_FILE);
  files.writeText(file, serializeProject(project));
  return file;
}

/**
 * 读取工程文件。
 * @param files 文件系统
 * @param projectDir 工程目录
 * @returns 工程
 * @throws FcdevError 目录里没有工程文件，或工程文件不合法
 */
export function readProject(files: FileSystem, projectDir: string): Project {
  const file = path.join(projectDir, PROJECT_FILE);
  let text: string;
  try {
    text = files.readText(file);
  } catch (error) {
    throw new FcdevError(`读不到工程文件 ${file}`, { code: 'DISASM_PROJECT_MISSING', hint: '先用 disasm_open 创建工程，或运行命令“反汇编映像文件”', cause: error });
  }
  return parseProject(text);
}

/**
 * 读取工程的映像，并检查它与工程记录的一致。
 * @param files 文件系统
 * @param projectDir 工程目录
 * @param project 工程
 * @returns 映像字节
 * @throws FcdevError 读不到映像，或映像与工程记录的摘要不符
 */
export function readProjectImage(files: FileSystem, projectDir: string, project: Project): Buffer {
  const file = fromProjectPath(projectDir, project.image.path);
  let bytes: Buffer;
  try {
    bytes = toBuffer(files.readBytes(file));
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
 * @param files 文件系统
 * @param projectDir 工程目录
 * @param project 工程
 * @returns 符号集文件名到文本
 * @throws FcdevError 读不到某个符号集文件
 */
export function readProjectSymbolSets(files: FileSystem, projectDir: string, project: Project): Map<string, string> {
  const texts = new Map<string, string>();
  for (const set of project.symbolSets) {
    const file = fromProjectPath(projectDir, set.path);
    try {
      texts.set(set.name, files.readText(file));
    } catch (error) {
      throw new FcdevError(`读不到符号集 ${set.name}：${file}`, { code: 'DISASM_SYMBOL_SET_MISSING', hint: '工程里记录的路径相对工程目录', cause: error });
    }
  }
  return texts;
}
