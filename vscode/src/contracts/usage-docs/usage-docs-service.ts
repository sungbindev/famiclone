// ------------------------------------------------------------------------
// 名称：usage-docs-service.ts
// 说明：使用文档的查询接口：列出全部主题，按主题名、文件名或关键词查找，读取文档正文。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：只读、不联网、不需要 fcdev.json；数据类型在 shared 里定义，这里重新导出。
// ------------------------------------------------------------------------

import type { DocInfo, DocQueryResult } from '../../shared/usage-docs/doc-topics';

export type { DocInfo, DocQueryKind, DocQueryResult } from '../../shared/usage-docs/doc-topics';

/** 使用文档服务。 */
export interface UsageDocsService {
  /**
   * 列出全部文档。
   * @returns 按登记顺序。
   * @throws FcdevError 登记表缺失或不合法。
   */
  list(): readonly DocInfo[];

  /**
   * 按词查找文档。
   * @param query 查询词。
   * @returns 匹配方式与结果。
   */
  query(query: string): DocQueryResult;

  /**
   * 读取一份文档的正文。
   * @param doc 文档。
   * @returns Markdown 正文。
   * @throws FcdevError 文档文件不存在。
   */
  read(doc: DocInfo): string;
}
