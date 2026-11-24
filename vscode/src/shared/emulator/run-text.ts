// ------------------------------------------------------------------------
// 名称：run-text.ts
// 说明：运行程序结果的说明文字：任务终端、命令通知与语言模型工具共用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：入参的形状与契约层的运行结果一致，共享层不引用契约，所以这里按结构声明。
// ------------------------------------------------------------------------

/** 运行程序的结果摘要。 */
export interface RunSummary {
  /** 启动的 Ember 的进程号。 */
  readonly pid: number | undefined;
  /** 载入内容的说明。 */
  readonly description: string;
  /** 是否调试运行。 */
  readonly debug: boolean;
  /** 调试运行用的端口。 */
  readonly port: number | undefined;
  /** 是整机（固件加磁盘镜像）运行。 */
  readonly firmwareDisk: boolean;
}

/**
 * 运行结果的说明：已启动的进程与载入的内容，调试运行时再说明停在什么状态、怎么继续。
 * @param result 运行结果。
 * @returns 多行文字，末尾没有换行。
 */
export function describeRunResult(result: RunSummary): string {
  const lines = [`已启动 Ember（进程 ${result.pid ?? '未知'}），加载 ${result.description}`];
  if (result.debug) {
    lines.push(
      result.firmwareDisk
        ? `调试模式：端口 ${result.port ?? '未知'}，固件已加载、磁盘已放入软驱、整机已开机，但没有运行；程序要在 SB-DOS 里输入程序名才会运行。继续操作用 emu_connect 连接，或命令“FC Dev：调用调试请求”。`
        : `调试模式：端口 ${result.port ?? '未知'}，程序已载入并上电，但没有运行。继续操作用 emu_connect 连接，或命令“FC Dev：调用调试请求”。`
    );
  }
  return lines.join('\n');
}
