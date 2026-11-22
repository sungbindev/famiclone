// ------------------------------------------------------------------------
// 名称：extension-resources.ts
// 说明：扩展自带资源的位置：目标定义、工具链、项目模板、使用文档与扩展版本。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：资源在扩展目录下，路径随扩展版本变化；读取资源的代码一律经这里取路径，不直接用 __dirname，
//       测试时注入临时目录。
// ------------------------------------------------------------------------

/** 扩展自带资源的位置。 */
export interface ExtensionResources {
  /** 扩展版本号。 */
  readonly version: string;
  /** 扩展目录（含 package.json）。 */
  readonly extensionDirectory: string;
  /** 目标定义所在的目录（里面是 nes、sb2000 等目标目录）。 */
  readonly targetsDirectory: string;
  /** 工具链根目录（里面有 bin、include、asminc、cfg、lib）。 */
  readonly toolchainDirectory: string;
  /** 项目模板所在的目录（里面是 nes-c 等模板目录与 _common）。 */
  readonly templatesDirectory: string;
  /** 使用文档所在的目录（里面有 topics.json）。 */
  readonly docsDirectory: string;
}
