// ------------------------------------------------------------------------
// 名称：tool-spec.ts
// 说明：一个反汇编工具在代码里的描述：对应的 fcdev 操作、是否需要用户确认、
//       调用与确认时显示的文字、需要检查的路径参数。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：工具的名字、说明与参数结构只写在 package.json 的 languageModelTools 里；
//       代码按名字取它的结构来检查输入，不重复定义。
// ------------------------------------------------------------------------

/** 工具的输入：模型给出的参数。 */
export type ToolInput = Record<string, unknown>;

/** 一个反汇编工具。 */
export interface ToolSpec {
  /** 工具名，与 package.json 里声明的一致。 */
  readonly name: string;
  /** 对应的 fcdev 操作名；用 fcdev build 构建并比较的工具写 build。 */
  readonly operation: string;
  /** 会写工程目录、运行汇编链接工具或构建，调用前要用户确认；
   *  有的工具只在某些参数下才写，可以按输入判断。 */
  readonly writes: boolean | ((input: ToolInput) => boolean);
  /** 调用时在聊天里显示的文字。 */
  describe(input: ToolInput): string;
  /** 需要确认时，对这次调用要做什么的说明；不给时用默认说明。 */
  confirm?(input: ToolInput): string;
  /** 取值是文件或目录路径、要按工作区规则检查的参数名；数组参数里的每一项都检查。 */
  readonly paths?: readonly string[];
  /** 路径参数里，允许指向工作区之外、但要在确认时说明的参数名（如要反汇编的映像）。 */
  readonly outsideAllowed?: readonly string[];
}
