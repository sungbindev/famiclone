// ------------------------------------------------------------------------
// 名称：file-usage-docs-service.ts
// 说明：使用文档服务的实现：读取扩展目录里的 docs/topics.json 与各文档。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：登记表每次读取都重新解析，文档很少，不做缓存。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ExtensionResources } from '../../contracts/resources/extension-resources';
import type { DocInfo, DocQueryResult, UsageDocsService } from '../../contracts/usage-docs/usage-docs-service';
import { FcdevError } from '../../shared/errors/fcdev-error';
import { describeDoc, parseTopics, queryDocs } from '../../shared/usage-docs/doc-topics';

/** 登记表的文件名。 */
const TOPICS_FILE = 'topics.json';

/** 读取随扩展发布的使用文档。 */
export class FileUsageDocsService implements UsageDocsService {
  /**
   * 创建服务。
   * @param fileSystem 文件系统。
   * @param resources 扩展资源位置。
   */
  constructor(
    private readonly fileSystem: FileSystem,
    private readonly resources: ExtensionResources
  ) {}

  /** @inheritdoc */
  list(): readonly DocInfo[] {
    const file = path.join(this.resources.docsDirectory, TOPICS_FILE);
    if (this.fileSystem.kindOf(file) !== 'file') {
      throw new FcdevError(`找不到文档登记表：${file}`, { code: 'DOCS_MISSING', hint: '重新安装扩展' });
    }
    return parseTopics(this.fileSystem.readText(file), file).map((entry) =>
      describeDoc(entry, this.resources.docsDirectory, this.resources.extensionDirectory)
    );
  }

  /** @inheritdoc */
  query(query: string): DocQueryResult {
    return queryDocs(this.list(), query);
  }

  /** @inheritdoc */
  read(doc: DocInfo): string {
    if (this.fileSystem.kindOf(doc.localPath) !== 'file') {
      throw new FcdevError(`找不到文档文件：${doc.localPath}`, { code: 'DOCS_MISSING', hint: '重新安装扩展' });
    }
    return this.fileSystem.readText(doc.localPath);
  }
}
