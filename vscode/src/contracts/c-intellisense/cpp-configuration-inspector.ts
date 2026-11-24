// ------------------------------------------------------------------------
// 名称：cpp-configuration-inspector.ts
// 说明：C/C++ 配置检查器的接口：判断项目的 c_cpp_properties.json 里是否还有 FC Dev 以外的配置。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：C/C++ 扩展用哪个配置只能由用户选，有其他配置时 FC Dev 的配置可能没在使用。
// ------------------------------------------------------------------------

/** C/C++ 配置检查器。 */
export interface CppConfigurationInspector {
  /**
   * 项目的 c_cpp_properties.json 里是否有名字不是 FC Dev 的配置。
   * @param root 项目根目录。
   * @returns 有其他配置为 true；文件不存在、解析失败或只有 FC Dev 配置时为 false。
   */
  hasOtherConfigurations(root: string): Promise<boolean>;
}
