/*
 * ------------------------------------------------------------------------
 * 名称：tooltip-page.tsx
 * 说明：悬停提示参考页：用 rujian-ui-kit 的真实悬停提示（带 title 的按钮与 setTooltip 版式提示）画出与 gallery 的 tooltip 页一样的内容。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：样本是三个按钮；鼠标停在按钮上等提示弹出后，提示元素排在所有样本之后（与 gallery 输出的位置一致）；
 *       样本顺序与内容必须与 gallery/pages.cpp 的 buildTooltipPage 一致，改一边要同步另一边。
 * ------------------------------------------------------------------------
 */

import { useEffect, useRef } from 'react';
import { Button, iconEye, iconInfoCircle, Row, setTooltip } from '@rujianjs/ui-kit';

/** 悬停提示页。 */
export function TooltipPage() {
  const rich = useRef<HTMLButtonElement>(null);
  useEffect(() => {
    if (rich.current === null) return;
    setTooltip(rich.current, {
      title: '标题',
      icon: iconInfoCircle,
      items: [{ icon: iconEye, title: '小标题', text: '正文说明文字，稍微长一点用来换行验证宽度' }, { text: '第二条说明' }]
    });
  }, []);
  return (
    <Row className="reference-page">
      <Button data-sample icon={false} title="这是一条短提示">
        提示一
      </Button>
      <Button
        data-sample
        icon={false}
        title="这是一段比较长的提示文字，用来验证提示在最大宽度处换行并保持角标位置。这是一段比较长的提示文字，用来验证提示在最大宽度处换行并保持角标位置。"
      >
        提示二
      </Button>
      <Button ref={rich} data-sample icon={false}>
        提示三
      </Button>
    </Row>
  );
}
