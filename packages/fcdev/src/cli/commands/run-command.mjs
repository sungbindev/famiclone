// ------------------------------------------------------------------------
// 名称：run-command.mjs
// 说明：run 命令：构建程序后在 Ember 里运行；--debug 时启动调试模式，由 debug 命令继续操作。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：运行方案由目标的 run 字段声明：rom 直接加载产物；firmware-disk（学习机）加载 fcdev.json 的 firmware 指定的固件，
//       把程序放进磁盘镜像挂到软驱并开机，自由模式再在 SB-DOS 里输入程序名运行，调试模式停在开机状态。
// ------------------------------------------------------------------------
import path from 'node:path';
import { buildDiskImage, buildPath, createBuildPlan } from '../../build/index.mjs';
import { DEFAULT_DEBUG_PORT, launchEmulator, locateEmulator } from '../../emulator/index.mjs';
import { FcdevError } from '../../shared/index.mjs';

/**
 * 执行 run 命令。
 * @param {{positionals: string[], options: {target?: string, debug: boolean, port?: number}, context: import('../index.mjs').CliContext}} input 参数与运行环境
 * @returns {Promise<number>} 退出码
 * @throws {FcdevError} 目标没有运行方案、学习机没有配置固件、找不到 Ember 或构建失败
 */
export async function runRunCommand({ positionals, options, context }) {
  if (positionals.length > 1) {
    throw new FcdevError('run 只接受一个路径参数', { code: 'ARGUMENT_INVALID', hint: '用法：npx fcdev run [目录或文件]' });
  }
  const input = { pathArg: positionals[0], cwd: context.cwd, targetOverride: options.target, env: context.env };
  const { target, config } = createBuildPlan(input);
  if (!target.run) {
    throw new FcdevError(`目标 ${target.name} 没有运行方案`, { code: 'TARGET_NO_RUN' });
  }
  const withFirmware = target.run.mode === 'firmware-disk';
  if (withFirmware && !config.firmware) {
    throw new FcdevError(`${target.name} 目标运行需要固件，fcdev.json 里没有设置 firmware`, {
      code: 'FIRMWARE_MISSING',
      hint: '在 fcdev.json 里用 firmware 指向学习机的 BIOS 文件（iNES 格式），例如 "firmware": "SB2K_BIOS.NES"',
    });
  }
  const emulator = locateEmulator({ configured: config.emulator, env: context.env });
  const port = options.port ?? DEFAULT_DEBUG_PORT;

  const reporter = {
    log: (message) => context.stdout.write(`${message}\n`),
    write: (text) => context.stderr.write(text),
  };
  const { plan, result } = await buildPath(input, reporter);
  let launch;
  let description;
  if (withFirmware) {
    const { imageFile } = buildDiskImage(plan, result);
    const programName = path.basename(result.outputFile, path.extname(result.outputFile));
    launch = { sb2000: { firmware: config.firmware, diskImage: imageFile, programName } };
    description = `磁盘镜像 ${imageFile}（程序 ${programName}）`;
  } else {
    launch = { romFile: result.outputFile };
    description = result.outputFile;
  }
  const { pid, client } = await launchEmulator({ executable: emulator.executable, ...launch, debug: options.debug, port });
  client?.close();
  context.stdout.write(`已启动 Ember（进程 ${pid}），加载 ${description}\n`);
  if (options.debug) {
    context.stdout.write(withFirmware
      ? `调试模式：端口 ${port}，固件已加载、磁盘已放入软驱、整机已开机，但没有运行；程序要在 SB-DOS 里输入程序名才会运行，继续操作用 debug 命令，例如 npx fcdev debug call runtime.run frames=60 wait=true\n`
      : `调试模式：端口 ${port}，程序已载入并上电，但没有运行；继续操作用 debug 命令，例如 npx fcdev debug call runtime.run frames=60 wait=true\n`);
  }
  return 0;
}
