/*
 * ------------------------------------------------------------------------
 * 名称：feedback-page.tsx
 * 说明：反馈控件参考页：用 rujian-ui-kit 的真实 Callout、State、StatusLabel 画出与 gallery 的 feedback 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：文字必须与 gallery/pages.cpp 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { Button, Callout, Row, Stack, State, StatusLabel } from '@rujianjs/ui-kit';

/** 反馈控件页。 */
export function FeedbackPage() {
  return (
    <Stack className="reference-page">
      <Callout text="这是一段提示说明文字">
        <Button variant="link">选择目录</Button>
      </Callout>
      <Callout kind="error" text="这是一段错误提示文字" />
      <State text="没有符合条件的插件" />
      <State kind="error" text="读取数据失败" />
      <Row>
        <StatusLabel tone="success">已启用</StatusLabel>
        <StatusLabel tone="stopped">已停用</StatusLabel>
      </Row>
    </Stack>
  );
}
