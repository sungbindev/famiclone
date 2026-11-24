// ------------------------------------------------------------------------
// 名称：assembly-intellisense-service.test.ts
// 说明：测试汇编智能感知服务：悬停、补全、转到定义、文档符号，以及 .include 的查找顺序与缓存。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用内存里的源文件代替磁盘；符号定义与调用写法取自 FC Dev 自带的 SB2000 与 NES 目标。
// ------------------------------------------------------------------------

import { describe, expect, it } from 'vitest';
import { AssemblySymbolResolver } from '../../src/logic/assembly/assembly-symbol-resolver';
import { ResolvingAssemblyIntelliSenseService } from '../../src/logic/assembly/resolving-assembly-intellisense-service';
import { FakeProjectInfoProvider, FakeSourceStore } from '../support/fakes';

const PROJECT = 'd:\\proj';
const INC_DIR = 'd:\\sdk\\asminc';

/** 符号定义文件：含分组标题、行尾说明与一个宏。 */
const SB2000_INC = [
  '; ---- 系统调用 ----',
  'SYSCALL             = $FFF0             ; 系统调用入口，JSR 到这里',
  'SYS_FUNCTION        = $063E             ; 功能号，调用前写入',
  '.macro call fn',
  '.endmacro'
].join('\n');

/** 运行库：带上方注释的标签。 */
const LIB_SOURCE = ['; void sb_newline(void)：换行', '_sb_newline:', '        rts'].join('\n');

/** 被编辑的程序。 */
const MAIN_TEXT = [
  '.include "sb2000.inc"',
  '.import _sb_newline',
  '_main:',
  '        lda #FN_X',
  '        sta SYS_FUNCTION',
  '        jsr SYSCALL',
  '@loop:  dex',
  '        bne @loop',
  '        jsr _sb_newline',
  '        rts'
].join('\n');

/** 创建服务与文件替身。 */
function create(withProject = true) {
  const store = new FakeSourceStore({
    [`${INC_DIR}\\sb2000.inc`]: SB2000_INC,
    [`${PROJECT}\\lib\\sb2000.s`]: LIB_SOURCE,
    [`${PROJECT}\\src\\main.s`]: MAIN_TEXT
  });
  const projects = new FakeProjectInfoProvider(
    withProject ? { root: PROJECT, targetName: 'sb2000', includeDirs: [], asmIncludeDirs: [INC_DIR] } : undefined
  );
  const service = new ResolvingAssemblyIntelliSenseService(new AssemblySymbolResolver(store, projects));
  const document = { path: `${PROJECT}\\src\\main.s`, text: MAIN_TEXT };
  return { store, service, document };
}

describe('悬停', () => {
  it('经 .include 引入的常量：显示值、说明、分组与来源', async () => {
    const { service, document } = create();
    const hover = await service.hover(document, { line: 4, character: 13 });
    expect(hover?.markdown).toContain('SYS_FUNCTION = $063E');
    expect(hover?.markdown).toContain('功能号，调用前写入');
    expect(hover?.markdown).toContain('分组：系统调用');
    expect(hover?.markdown).toContain('定义于 sb2000.inc 第 3 行');
    expect(hover?.range).toEqual({ start: { line: 4, character: 12 }, end: { line: 4, character: 24 } });
  });

  it('工作区其他文件里的标签：取上方注释作为说明', async () => {
    const { service, document } = create();
    const hover = await service.hover(document, { line: 8, character: 14 });
    expect(hover?.markdown).toContain('void sb\\_newline(void)：换行');
    expect(hover?.markdown).toContain('定义于 sb2000.s 第 2 行');
  });

  it('指令位置的助记符显示 6502 指令资料', async () => {
    const { service, document } = create();
    const hover = await service.hover(document, { line: 3, character: 9 });
    expect(hover?.markdown).toContain('**LDA**：加载累加器');
  });

  it('汇编指令显示写法与说明', async () => {
    const { service, document } = create();
    const hover = await service.hover(document, { line: 1, character: 2 });
    expect(hover?.markdown).toContain('**.import**');
  });

  it('当前文件的局部标签可以悬停，找不到的名字没有提示', async () => {
    const { service, document } = create();
    expect((await service.hover(document, { line: 7, character: 13 }))?.markdown).toContain('@loop:');
    expect(await service.hover(document, { line: 3, character: 17 })).toBeUndefined();
  });

  it('不在 FC Dev 项目里时没有引入目录，引入文件回落到工作区文件里查找', async () => {
    const { service, document } = create(false);
    expect((await service.hover(document, { line: 4, character: 13 }))?.markdown).toContain('定义于 sb2000.inc 第 3 行');
    expect((await service.hover(document, { line: 8, character: 14 }))?.markdown).toContain('_sb_newline:');
  });
});

describe('转到定义', () => {
  it('跳到引入文件里符号名所在的位置', async () => {
    const { service, document } = create();
    const location = await service.definition(document, { line: 5, character: 13 });
    expect(location).toEqual({
      path: `${INC_DIR}\\sb2000.inc`,
      range: { start: { line: 1, character: 0 }, end: { line: 1, character: 7 } }
    });
  });

  it('局部标签跳到当前文件，汇编指令没有定义', async () => {
    const { service, document } = create();
    expect((await service.definition(document, { line: 7, character: 13 }))?.range.start.line).toBe(6);
    expect(await service.definition(document, { line: 1, character: 2 })).toBeUndefined();
  });
});

describe('补全', () => {
  it('指令位置给出助记符、汇编指令与宏', async () => {
    const { service, document } = create();
    const items = await service.complete(document, { line: 3, character: 10 });
    const labels = items.map((item) => item.label);
    expect(labels).toContain('lda');
    expect(labels).toContain('.segment');
    expect(labels).toContain('call');
    expect(labels).not.toContain('SYSCALL');
  });

  it('操作数位置给出符号，不含助记符', async () => {
    const { service, document } = create();
    const items = await service.complete(document, { line: 5, character: 13 });
    const labels = items.map((item) => item.label);
    expect(labels).toEqual(expect.arrayContaining(['SYSCALL', 'SYS_FUNCTION', '_sb_newline', '@loop']));
    expect(labels).not.toContain('lda');
    expect(items.find((item) => item.label === 'SYSCALL')).toMatchObject({ kind: 'constant', detail: '常量 $FFF0' });
  });

  it('以点开头只给汇编指令；注释里不补全', async () => {
    const { service } = create();
    const document = { path: `${PROJECT}\\src\\main.s`, text: '        .se\n        nop ; lo' };
    expect((await service.complete(document, { line: 0, character: 11 })).every((item) => item.kind === 'directive')).toBe(true);
    expect(await service.complete(document, { line: 1, character: 16 })).toEqual([]);
  });
});

describe('文档符号', () => {
  it('列出全局符号，不含局部标签', () => {
    const { service, document } = create();
    expect(service.documentSymbols(document).map((symbol) => symbol.name)).toEqual(['_main']);
  });
});

describe('缓存', () => {
  it('同一个文件只读取一次，失效后重新读取', async () => {
    const { service, document, store } = create();
    await service.hover(document, { line: 4, character: 13 });
    await service.hover(document, { line: 5, character: 13 });
    const includePath = `${INC_DIR}\\sb2000.inc`;
    expect(store.reads.filter((path) => path === includePath)).toHaveLength(1);
    store.files[includePath] = 'SYSCALL = $1234\nSYS_FUNCTION = $1\n';
    service.invalidate(includePath);
    const hover = await service.hover(document, { line: 5, character: 13 });
    expect(hover?.markdown).toContain('SYSCALL = $1234');
  });

  it('文本里的修改立即生效，不需要保存', async () => {
    const { service } = create();
    const document = { path: `${PROJECT}\\src\\main.s`, text: 'NEWCONST = 7 ; 新常量\n        lda NEWCONST' };
    const hover = await service.hover(document, { line: 1, character: 14 });
    expect(hover?.markdown).toContain('NEWCONST = 7');
    expect(hover?.markdown).toContain('新常量');
  });
});
