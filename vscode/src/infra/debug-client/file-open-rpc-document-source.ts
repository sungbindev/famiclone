// ------------------------------------------------------------------------
// 名称：file-open-rpc-document-source.ts
// 说明：从扩展目录读出调试协议的接口描述文档（debug/openrpc.json），读一次后缓存。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：文档由 Ember 的 npm run schema 生成并放到扩展的 debug 目录；位置经扩展资源位置取得。
// ------------------------------------------------------------------------

import * as path from 'node:path';
import type { OpenRpcDocumentSource } from '../../contracts/debug-client/debug-connector';
import type { FileSystem } from '../../contracts/files/file-system';
import type { ExtensionResources } from '../../contracts/resources/extension-resources';
import type { OpenRpcDocument } from '../../shared/debug-client/method-catalog';

/** 文档相对扩展目录的路径。 */
const DOCUMENT_PATH = path.join('debug', 'openrpc.json');

/** 读扩展自带的接口描述文档。 */
export class FileOpenRpcDocumentSource implements OpenRpcDocumentSource {
  private cached?: OpenRpcDocument;

  /**
   * @param files 文件系统。
   * @param resources 扩展自带资源的位置。
   */
  constructor(
    private readonly files: FileSystem,
    private readonly resources: ExtensionResources
  ) {}

  /** @inheritdoc */
  load(): OpenRpcDocument {
    this.cached ??= JSON.parse(this.files.readText(path.join(this.resources.extensionDirectory, DOCUMENT_PATH))) as OpenRpcDocument;
    return this.cached;
  }
}
