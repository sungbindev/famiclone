/*
 * ------------------------------------------------------------------------
 * 名称：dialog-page.tsx
 * 说明：对话框参考页：用 rujian-ui-kit 的对话框标题栏、正文区域与按钮区的真实样式，画出与 gallery 的 dialog 页一样的内容（独立窗口对话框的内容）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：组件库没有单独导出“对话框内容”，这里按 DialogPopupView 的结构用同样的类名拼出来；样本依次是最大化、关闭按钮与按钮区的两个按钮；
 *       样本顺序与内容必须与 gallery/pages.cpp 的 buildDialogPage 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Button, ScrollArea } from '@rujianjs/ui-kit';
import { DialogTitleBar } from '@kit-source/components/dialog/DialogTitleBar';

const noop = () => undefined;

/** 对话框页。 */
export function DialogPage() {
  return (
    <div style={{ height: '100vh', background: 'var(--bg)' }}>
      <div className="ui-dialog-popup__frame ui-is-active">
        <DialogTitleBar titleId="dialog-title" title="对话框标题" maximizable maximized={false} closable onToggleMaximize={noop} onClose={noop} />
        <div className="ui-dialog">
          <ScrollArea className="ui-dialog__scroll" viewportProps={{ className: 'ui-dialog__body ui-select-scope' }}>
            <p style={{ margin: 0 }}>这是对话框的正文内容。</p>
          </ScrollArea>
          <div className="ui-dialog__footer">
            <Button>取消</Button>
            <Button variant="primary">确定</Button>
          </div>
        </div>
      </div>
    </div>
  );
}
