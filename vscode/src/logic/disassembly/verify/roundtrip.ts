// ------------------------------------------------------------------------
// 名称：roundtrip.ts
// 说明：回读校验：把生成的源码工程写到目录里，用 ca65 汇编每个模块、ld65 链接，再把每个段的输出与映
//       像里对应的字节逐字节比较。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：生成的文件按原样写入（UTF-8，没有 BOM，换行为 LF）；目标文件与链接输出放在 verify 子目录，
//       每次先清空；不一致时报告第一处差异所在的段、地址、文件偏移与两边的字节，以及它附近的符号。
// ------------------------------------------------------------------------
import * as path from 'node:path';
import type { FileSystem } from '../../../contracts/files/file-system';
import type { ToolName, ToolRunner, Toolchain } from '../../../contracts/toolchain/toolchain';
import { FcdevError } from '../../../shared/errors/fcdev-error';
import { expectSegment, projectDigest } from '../../../shared/disassembly/model/index';
import type { Project } from '../../../shared/disassembly/model/project';
import type { GeneratedProject } from '../../../shared/disassembly/generate/index';

export interface RoundtripResult {
  /** 每个段的输出都与映像一致 */
  ok: boolean;
  /** 第一处差异；一致时为 null */
  firstDifference: {segmentId: number, address: number, offset: number, expected: number|null, actual: number|null, symbol: string|null}|null;
}

/**
 * 把生成的文件写进工程目录。
 * @param files 文件系统
 * @param projectDir 工程目录
 * @param generated 相对路径到内容
 */
export function writeGeneratedFiles(files: FileSystem, projectDir: string, generated: Map<string, string>) {
  for (const [relative, content] of generated) {
    const target = path.join(projectDir, relative);
    files.ensureDirectory(path.dirname(target));
    files.writeText(target, content);
  }
}

/**
 * 运行一个工具，失败时抛出带输出的错误。
 * @param tools 工具运行器
 * @param toolchain 工具链
 * @param tool 工具名
 * @param args 参数
 * @param cwd 工作目录
 * @param failure 失败说明
 */
async function runOrThrow(tools: ToolRunner, toolchain: Toolchain, tool: ToolName, args: string[], cwd: string, failure: string) {
  const result = await tools.run(toolchain, tool, args, cwd);
  if (result.code !== 0) throw new FcdevError(failure, { code: 'DISASM_ROUNDTRIP_FAILED', hint: `${result.stdout}${result.stderr}`.trim() });
}

/**
 * 回读校验。
 * @param input 工程、映像字节、生成结果、工程目录与工具链
 * @returns 校验结果
 * @throws FcdevError 汇编或链接失败
 */
export async function verifyRoundtrip({ files, tools, project, bytes, generated, projectDir, toolchain }: {files: FileSystem, tools: ToolRunner, project: Project, bytes: Uint8Array, generated: GeneratedProject, projectDir: string, toolchain: Toolchain}): Promise<RoundtripResult> {
  writeGeneratedFiles(files, projectDir, generated.files);
  const verifyDir = path.join(projectDir, 'verify');
  files.removeDirectory(verifyDir);
  files.ensureDirectory(verifyDir);
  const objects: string[] = [];
  for (const module of generated.modules) {
    const object = path.join('verify', `${module}.o`);
    await runOrThrow(tools, toolchain, 'ca65', ['-I', 'include', path.join('src', `${module}.s`), '-o', object], projectDir, `汇编 src/${module}.s 失败`);
    objects.push(`${module}.o`);
  }
  await runOrThrow(tools, toolchain, 'ld65', ['-C', path.join('..', 'link.cfg'), '-o', 'unused.bin', ...objects], verifyDir, '链接失败');

  for (const segment of generated.segments) {
    const expected = bytes.subarray(segment.fileOffset, segment.fileOffset + segment.length);
    let actual: Uint8Array;
    try {
      actual = files.readBytes(path.join(verifyDir, segment.output));
    } catch {
      throw new FcdevError(`链接没有生成 ${segment.output}`, { code: 'DISASM_ROUNDTRIP_FAILED' });
    }
    const length = Math.max(expected.length, actual.length);
    for (let index = 0; index < length; index += 1) {
      if (expected[index] === actual[index]) continue;
      const layoutSegment = expectSegment(project.layout, segment.id);
      const address = layoutSegment.address + index;
      const offset = segment.fileOffset + index;
      const before = project.symbols.filter((symbol) => symbol.segmentId === segment.id && symbol.address <= address).toSorted((a, b) => b.address - a.address)[0];
      return { ok: false, firstDifference: { segmentId: segment.id, address, offset, expected: expected[index] ?? null, actual: actual[index] ?? null, symbol: before?.name ?? null } };
    }
  }
  return { ok: true, firstDifference: null };
}

/**
 * 把回读结果记进工程。
 * @param project 工程，不会被修改
 * @param result 回读结果
 * @param [now] 时间，默认当前
 * @returns 记下结果的新工程（浅拷贝，除 verification 外与原工程共用数据）
 */
export function recordVerification(project: Project, result: RoundtripResult, now: Date = new Date()): Project {
  return { ...project, verification: { ok: result.ok, digest: projectDigest(project), at: now.toISOString(), ...(result.firstDifference === null ? {} : { firstDifference: result.firstDifference }) } };
}
