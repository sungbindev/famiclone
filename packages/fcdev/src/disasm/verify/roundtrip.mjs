// ------------------------------------------------------------------------
// 名称：roundtrip.mjs
// 说明：回读校验：把生成的源码工程写到目录里，用 ca65 汇编每个模块、ld65 链接，再把每个段的输出与映像里对应的字节逐字节比较。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：生成的文件按原样写入（UTF-8，没有 BOM，换行为 LF）；目标文件与链接输出放在 verify 子目录，每次先清空；
//       不一致时报告第一处差异所在的段、地址、文件偏移与两边的字节，以及它附近的符号。
// ------------------------------------------------------------------------
import { mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../../shared/index.mjs';
import { projectDigest } from '../model/index.mjs';
import { runTool } from '../../toolchain/index.mjs';

/**
 * @typedef {object} RoundtripResult
 * @property {boolean} ok 每个段的输出都与映像一致
 * @property {{segmentId: number, address: number, offset: number, expected: number|null, actual: number|null, symbol: string|null}|null} firstDifference 第一处差异；一致时为 null
 */

/**
 * 把生成的文件写进工程目录。
 * @param {string} projectDir 工程目录
 * @param {Map<string, string>} files 相对路径到内容
 */
export function writeGeneratedFiles(projectDir, files) {
  for (const [relative, content] of files) {
    const target = path.join(projectDir, relative);
    mkdirSync(path.dirname(target), { recursive: true });
    writeFileSync(target, content, 'utf8');
  }
}

/**
 * 运行一个工具，失败时抛出带输出的错误。
 * @param {import('../../toolchain/index.mjs').Toolchain} toolchain 工具链
 * @param {string} tool 工具名
 * @param {string[]} args 参数
 * @param {string} cwd 工作目录
 * @param {string} failure 失败说明
 */
async function runOrThrow(toolchain, tool, args, cwd, failure) {
  const result = await runTool(toolchain, tool, args, { cwd });
  if (result.code !== 0) throw new FcdevError(failure, { code: 'DISASM_ROUNDTRIP_FAILED', hint: `${result.stdout}${result.stderr}`.trim() });
}

/**
 * 回读校验。
 * @param {{project: import('../model/project.mjs').Project, bytes: Uint8Array, generated: import('../generate/index.mjs').GeneratedProject, projectDir: string, toolchain: import('../../toolchain/index.mjs').Toolchain}} input 工程、映像字节、生成结果、工程目录与工具链
 * @returns {Promise<RoundtripResult>} 校验结果
 * @throws {FcdevError} 汇编或链接失败
 */
export async function verifyRoundtrip({ project, bytes, generated, projectDir, toolchain }) {
  writeGeneratedFiles(projectDir, generated.files);
  const verifyDir = path.join(projectDir, 'verify');
  rmSync(verifyDir, { recursive: true, force: true });
  mkdirSync(verifyDir, { recursive: true });
  const objects = [];
  for (const module of generated.modules) {
    const object = path.join('verify', `${module}.o`);
    await runOrThrow(toolchain, 'ca65', ['-I', 'include', path.join('src', `${module}.s`), '-o', object], projectDir, `汇编 src/${module}.s 失败`);
    objects.push(`${module}.o`);
  }
  await runOrThrow(toolchain, 'ld65', ['-C', path.join('..', 'link.cfg'), '-o', 'unused.bin', ...objects], verifyDir, '链接失败');

  for (const segment of generated.segments) {
    const expected = bytes.subarray(segment.fileOffset, segment.fileOffset + segment.length);
    let actual;
    try {
      actual = readFileSync(path.join(verifyDir, segment.output));
    } catch {
      throw new FcdevError(`链接没有生成 ${segment.output}`, { code: 'DISASM_ROUNDTRIP_FAILED' });
    }
    const length = Math.max(expected.length, actual.length);
    for (let index = 0; index < length; index += 1) {
      if (expected[index] === actual[index]) continue;
      const layoutSegment = project.layout.segments.find((item) => item.id === segment.id);
      const address = layoutSegment.address + index;
      const offset = segment.fileOffset + index;
      const before = project.symbols.filter((symbol) => symbol.segmentId === segment.id && symbol.address <= address).sort((a, b) => b.address - a.address)[0];
      return { ok: false, firstDifference: { segmentId: segment.id, address, offset, expected: expected[index] ?? null, actual: actual[index] ?? null, symbol: before?.name ?? null } };
    }
  }
  return { ok: true, firstDifference: null };
}

/**
 * 把回读结果记进工程。
 * @param {import('../model/project.mjs').Project} project 工程，不会被修改
 * @param {RoundtripResult} result 回读结果
 * @param {Date} [now] 时间，默认当前
 * @returns {import('../model/project.mjs').Project} 记下结果的新工程（浅拷贝，除 verification 外与原工程共用数据）
 */
export function recordVerification(project, result, now = new Date()) {
  return { ...project, verification: { ok: result.ok, digest: projectDigest(project), at: now.toISOString(), ...(result.firstDifference === null ? {} : { firstDifference: result.firstDifference }) } };
}
