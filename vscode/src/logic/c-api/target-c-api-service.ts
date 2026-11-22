// ------------------------------------------------------------------------
// 名称：target-c-api-service.ts
// 说明：目标 C 接口文档的读取：解析目标 include 目录下头文件里带文档注释的函数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：没有 include 目录或头文件里没有带文档注释的函数的目标，视为没有 C 接口文档。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { CApiService, TargetApi } from '../../contracts/c-api/c-api-service';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ProjectConfigReader } from '../../contracts/project/project-config-reader';
import type { TargetRepository, Target } from '../../contracts/targets/target-repository';
import { parseHeaderFunctions, type ApiFunction } from '../../shared/c-api/api-functions';
import { FcdevError } from '../../shared/errors/fcdev-error';

/** 经文件系统读取目标头文件的 C 接口服务。 */
export class TargetCApiService implements CApiService {
  /**
   * 创建服务。
   * @param fileSystem 文件系统。
   * @param configReader 项目配置读取。
   * @param targets 目标定义读取。
   */
  constructor(
    private readonly fileSystem: FileSystem,
    private readonly configReader: ProjectConfigReader,
    private readonly targets: TargetRepository
  ) {}

  /** @inheritdoc */
  describe(input: { readonly cwd: string; readonly target?: string | undefined }): TargetApi {
    const targetName = input.target ?? this.configReader.read(input.cwd, input.cwd).target;
    if (targetName === undefined) {
      throw new FcdevError('没有指定目标', { code: 'TARGET_MISSING', hint: '在 fcdev.json 里写 "target"，或指定 target' });
    }
    const target = this.targets.load(targetName);
    const functions = this.loadFunctions(target);
    if (functions.length === 0) {
      throw new FcdevError(`目标 ${target.name} 没有 C 接口文档`, {
        code: 'API_MISSING',
        hint: '目前只有 sb2000 目标提供运行库函数的文档'
      });
    }
    return { targetName: target.name, targetTitle: target.title, functions };
  }

  /**
   * 读出目标全部带文档注释的函数。
   * @param target 目标。
   * @returns 函数列表，按头文件名与声明顺序。
   */
  private loadFunctions(target: Target): ApiFunction[] {
    const functions: ApiFunction[] = [];
    for (const includeDir of target.include) {
      const directory = path.join(target.dir, includeDir);
      if (this.fileSystem.kindOf(directory) !== 'directory') continue;
      const headers = this.fileSystem
        .list(directory)
        .filter((entry) => !entry.isDirectory && entry.name.endsWith('.h'))
        .map((entry) => entry.name)
        .toSorted();
      for (const name of headers) {
        const text = this.fileSystem.readText(path.join(directory, name)).replace(/\r\n/g, '\n');
        functions.push(...parseHeaderFunctions(text, name));
      }
    }
    return functions;
  }
}
