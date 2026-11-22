// ------------------------------------------------------------------------
// 名称：build-service-comparer.ts
// 说明：用构建服务构建一个项目，并把产物与反汇编工程记录的原映像逐字节比较。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：原映像的路径取自 disasm.json，相对工程目录；构建的是用户或模型按生成的源码整理出来的
//       fcdev 项目，不是反汇编工程目录本身。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { BuildService } from '../../contracts/build/build-service';
import type { BuildComparer, BuildComparison } from '../../contracts/disassembly/build-comparer';
import type { FileSystem } from '../../contracts/files/file-system';
import { firstDifference } from '../../shared/disassembly/image-compare';
import { UserFacingError } from '../../shared/errors/user-facing-error';

/** 经构建服务构建并比较。 */
export class BuildServiceComparer implements BuildComparer {
  /**
   * 创建比较器。
   * @param fileSystem 文件系统。
   * @param buildService 构建服务。
   */
  constructor(
    private readonly fileSystem: FileSystem,
    private readonly buildService: BuildService
  ) {}

  /** @inheritdoc */
  async buildAndCompare(projectDir: string, buildTarget: string): Promise<BuildComparison> {
    const imageFile = this.readImagePath(projectDir);
    if (this.fileSystem.kindOf(imageFile) !== 'file') throw new UserFacingError(`读不到映像文件 ${imageFile}`);
    const image = this.fileSystem.readBytes(imageFile);
    const kind = this.fileSystem.kindOf(buildTarget);
    if (kind === undefined) throw new UserFacingError(`找不到要构建的 ${buildTarget}`);
    const outcome =
      kind === 'directory'
        ? await this.buildService.build({ cwd: buildTarget })
        : await this.buildService.build({ pathArg: path.basename(buildTarget), cwd: path.dirname(buildTarget) });
    const builtFile = outcome.result.outputFile;
    const built = this.fileSystem.readBytes(builtFile);
    const difference = firstDifference(image, built);
    return { builtFile, builtSize: built.length, imageSize: image.length, identical: difference === null, firstDifference: difference };
  }

  /**
   * 读工程文件里记录的映像路径。
   * @param projectDir 反汇编工程目录。
   * @returns 映像文件的绝对路径。
   * @throws UserFacingError 读不到工程文件，或里面没有映像路径。
   */
  private readImagePath(projectDir: string): string {
    const projectFile = path.join(projectDir, 'disasm.json');
    if (this.fileSystem.kindOf(projectFile) !== 'file') throw new UserFacingError(`读不到工程文件 ${projectFile}`);
    try {
      const parsed = JSON.parse(this.fileSystem.readText(projectFile)) as { image?: { path?: unknown } };
      if (typeof parsed.image?.path === 'string') return path.resolve(projectDir, parsed.image.path);
    } catch {
      // 解析失败按下面的统一说明处理
    }
    throw new UserFacingError('工程文件里没有映像路径，它可能已损坏');
  }
}
