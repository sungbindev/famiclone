// ------------------------------------------------------------------------
// 名称：run-command.mjs
// 说明：run 命令：构建程序后在模拟器里运行；--debug 时启动调试模式，由 debug 命令继续操作。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：运行方案由目标的 run 字段声明：rom 直接加载产物；firmware-disk 加载固件（fcdev.json 的 firmware），挂载磁盘镜像后复位。
//       firmware-disk 方案需要调试服务挂载镜像，挂载完成后用 live 命令转为自由运行（加 --debug 则保持调试模式）。
// ------------------------------------------------------------------------
import path from 'node:path';
import { buildDiskImage, buildPath } from '../../build/index.mjs';
import { DEFAULT_DEBUG_PORT, launchEmulator, locateEmulator } from '../../emulator/index.mjs';
import { FcdevError } from '../../shared/index.mjs';

/**
 * 执行 run 命令。
 * @param {{positionals: string[], options: {target?: string, debug: boolean, port?: number}, context: import('../index.mjs').CliContext}} input 参数与运行环境
 * @returns {Promise<number>} 退出码
 * @throws {FcdevError} 目标没有运行方案、找不到模拟器或构建失败
 */
export async function runRunCommand({ positionals, options, context }) {
  if (positionals.length > 1) {
    throw new FcdevError('run 只接受一个路径参数', { code: 'ARGUMENT_INVALID', hint: '用法：npx fcdev run [目录或文件]' });
  }
  const reporter = {
    log: (message) => context.stdout.write(`${message}\n`),
    write: (text) => context.stderr.write(text),
  };
  const { plan, result } = await buildPath(
    { pathArg: positionals[0], cwd: context.cwd, targetOverride: options.target, env: context.env },
    reporter,
  );
  const run = plan.target.run;
  if (!run) {
    throw new FcdevError(`目标 ${plan.target.name} 没有运行方案`, { code: 'TARGET_NO_RUN' });
  }
  const emulator = locateEmulator({ configured: plan.config.emulator, env: context.env });
  const port = options.port ?? DEFAULT_DEBUG_PORT;

  if (run.mode === 'rom') {
    const { pid, client } = await launchEmulator({ executable: emulator.executable, romFile: result.outputFile, debug: options.debug, port });
    client?.close();
    context.stdout.write(`已启动模拟器（进程 ${pid}），加载 ${result.outputFile}\n`);
  } else {
    if (!plan.config.firmware) {
      throw new FcdevError(`目标 ${plan.target.name} 的运行需要固件（BIOS）`, {
        code: 'FIRMWARE_MISSING',
        hint: '在 fcdev.json 里用 firmware 指定固件文件；fcdev 不提供任何 BIOS 或 ROM',
      });
    }
    const { imageFile } = buildDiskImage(plan, result);
    const { pid, client } = await launchEmulator({ executable: emulator.executable, romFile: plan.config.firmware, debug: true, port });
    try {
      for (const command of [`disk ${imageFile}`, 'reset', ...(options.debug ? [] : ['live'])]) {
        const reply = await client.send(command);
        if (!reply.ok) throw new FcdevError(`模拟器拒绝命令“${command}”：${reply.head}`, { code: 'EMULATOR_COMMAND_FAILED' });
      }
    } finally {
      client.close();
    }
    context.stdout.write(`已启动模拟器（进程 ${pid}），固件 ${plan.config.firmware}，磁盘镜像 ${imageFile}\n`);
    context.stdout.write(`等 DOS 启动后，在提示符下输入 ${path.basename(result.outputFile, path.extname(result.outputFile))} 运行程序\n`);
  }
  if (options.debug) {
    context.stdout.write(`调试模式：端口 ${port}，模拟器只在收到 run 命令时运行，例如 npx fcdev debug run 60\n`);
  }
  return 0;
}
