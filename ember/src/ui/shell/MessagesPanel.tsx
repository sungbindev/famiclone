/*
 * ------------------------------------------------------------------------
 * 名称：MessagesPanel.tsx
 * 说明：底部面板的“消息”页签：列出运行消息（时间、级别、文字），没有消息时显示淡色提示；标题行右侧有“清空”按钮。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * 备注：消息由各模块经外壳接口 addMessage 追加；本页签不是调试器，不显示内存与寄存器。
 * ------------------------------------------------------------------------
 */

import { Button, iconEraser, State } from '@rujianjs/ui-kit';
import { useShellMessages } from './shell-context';
import type { ShellMessageLevel } from './shell-registry';

const TEXT = {
  empty: '暂无消息',
  clear: '清空',
  levels: { info: '信息', warning: '警告', error: '错误' } satisfies Record<ShellMessageLevel, string>
} as const;

/** 补零成两位。 */
function pad(value: number): string {
  return String(value).padStart(2, '0');
}

/** 时间显示为“时:分:秒”。 */
function formatTime(time: Date): string {
  return `${pad(time.getHours())}:${pad(time.getMinutes())}:${pad(time.getSeconds())}`;
}

/** “消息”页签的内容。 */
export function MessagesPanel() {
  const { messages } = useShellMessages();
  if (messages.length === 0) return <State text={TEXT.empty} />;
  return (
    <ul className="ember-messages">
      {messages.map((message) => (
        <li key={message.id} className="ember-messages__item" data-level={message.level}>
          <time className="ember-messages__time" dateTime={message.time.toISOString()}>
            {formatTime(message.time)}
          </time>
          <span className="ember-messages__level">{TEXT.levels[message.level]}</span>
          <span className="ember-messages__text">{message.text}</span>
        </li>
      ))}
    </ul>
  );
}

/** “消息”页签标题行右侧的操作：清空。 */
export function MessagesActions() {
  const { messages, clear } = useShellMessages();
  return (
    <Button variant="ghost" iconOnly compact icon={iconEraser} title={TEXT.clear} disabled={messages.length === 0} onClick={clear}>
      {TEXT.clear}
    </Button>
  );
}
