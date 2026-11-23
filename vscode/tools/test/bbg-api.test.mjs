// ------------------------------------------------------------------------
// 名称：bbg-api.test.mjs
// 说明：测试步步高汇编运行库的接口检查，并检查入库的库源码、符号文件与不封装清单满足规则。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-24
// 备注：规则见 vscode/tools/check/check-bbg-api.mjs：每个函数一个文件且有完整说明，bbg.inc 与源码一致，每个函数都有
//       测试程序用到，每个功能号被封装或写明不封装的原因，两者不能同时成立。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { describe, it } from 'node:test';
import { checkBbgApi, coverageOf, readFunctions, readKnownFunctions, readUnwrapped } from '../check/check-bbg-api.mjs';
import { API_DOC_FILE, renderApiReference } from '../gen/gen-bbg-api-doc.mjs';

describe('步步高运行库接口检查', () => {
  it('入库的库源码、符号文件与不封装清单没有问题', () => {
    const { problems } = checkBbgApi();
    assert.deepEqual(problems, []);
  });

  it('功能号表：DOS 17 个、文件调用 25 个、BIOS 1.0 实现的功能号都读到了', () => {
    const known = readKnownFunctions();
    assert.equal(new Set(known.dos.values()).size, 17);
    assert.equal(new Set(known.file.values()).size, 25);
    assert.ok(new Set(known.bios.values()).size >= 40);
  });

  it('封装的功能号覆盖：DOS 与文件调用除不封装清单外全部有函数', () => {
    const coverage = coverageOf(readFunctions());
    const unwrapped = readUnwrapped();
    for (const kind of ['dos', 'file']) {
      for (const number of new Set(readKnownFunctions()[kind].values())) {
        const wrapped = (coverage[kind].get(number) ?? []).length > 0;
        assert.ok(wrapped !== Boolean(unwrapped[kind]?.[String(number)]), `${kind} 功能 ${number}：封装与原因必须二选一`);
      }
    }
  });

  it('入库的接口参考文档与库源码注释生成的一致', () => {
    const stored = readFileSync(API_DOC_FILE, 'utf8').replace(/\r\n/g, '\n');
    assert.equal(stored, renderApiReference(), '库源码注释或不封装清单改过了：运行 node vscode/tools/gen/gen-bbg-api-doc.mjs 重新生成 api-reference.md');
  });
});
