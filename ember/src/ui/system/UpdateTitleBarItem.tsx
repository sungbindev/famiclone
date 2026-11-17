/*
 * ------------------------------------------------------------------------
 * 名称：UpdateTitleBarItem.tsx
 * 说明：系统模块登记到标题栏右侧（系统窗口按钮左边）的“更新”按钮：新版本下载好时显示蓝色按钮，点击后重启并安装更新；其他状态不显示。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：显示与否取自升级控制器，状态变化后控制器通知外壳重新渲染；点击与“帮助→重启并安装更新”走同一条路径，失败用外壳通知给出说明。
 * ------------------------------------------------------------------------
 */

import { Button, iconRefresh } from '@rujianjs/ui-kit';
import { useShell } from '../shell/shell-context';
import type { UpdateController } from './update-controller';

/** 用户可见的文字。 */
const TEXT = { update: '更新' } as const;

/** 标题栏项的属性。 */
export interface UpdateTitleBarItemProps {
  readonly controller: UpdateController;
}

/** “更新”按钮；新版本还没有下载好时返回 null。 */
export function UpdateTitleBarItem({ controller }: UpdateTitleBarItemProps) {
  const shell = useShell();
  if (!controller.isReady()) return null;
  return (
    <Button variant="primary" icon={iconRefresh} title={controller.title()} onClick={() => controller.run(shell)}>
      {TEXT.update}
    </Button>
  );
}
