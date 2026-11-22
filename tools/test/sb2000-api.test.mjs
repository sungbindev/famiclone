// ------------------------------------------------------------------------
// 名称：sb2000-api.test.mjs
// 说明：测试 SB2000 接口检查工具与 API 参考文档生成，并检查入库的头文件、库源码、不封装清单与参考文档满足规则。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：规则见 tools/check-sb2000-api.mjs：每个函数有说明、@param、@return，且有且只有一处实现，每个功能号被封装或写明不封装的原因；
//       参考文档 targets/sb2000/api-reference.md 由 tools/gen-sb2000-api-doc.mjs 生成，改了头文件注释或清单后要重新生成。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { describe, it } from 'node:test';
import { checkSb2000Api, coverageOf, KNOWN_FUNCTIONS, parseHeaderFunctions, readFunctions, readUnwrapped } from '../check-sb2000-api.mjs';
import { API_DOC_FILE, renderApiReference } from '../gen-sb2000-api-doc.mjs';

describe('SB2000 接口检查', () => {
  it('入库的头文件与库源码没有问题', () => {
    const { problems } = checkSb2000Api();
    assert.deepEqual(problems, []);
  });

  it('解析函数声明的文档标签与参数', () => {
    const [fn] = parseHeaderFunctions(
      '/**\n * 读一个字节。\n *\n * @syscall $24\n * @param page 页号。\n * @param offset 偏移。\n * @return 字节。\n */\nunsigned char sb_far_read(unsigned char page, unsigned int offset);\n',
      'x.h',
    );
    assert.equal(fn.name, 'sb_far_read');
    assert.deepEqual(fn.params, ['page', 'offset']);
    assert.deepEqual(fn.paramDocs, ['page', 'offset']);
    assert.equal(fn.returnType, 'unsigned char');
    assert.equal(fn.hasReturnDoc, true);
    assert.deepEqual(fn.syscalls, ['$24']);
    assert.equal(fn.summary, '读一个字节。');
  });

  it('宏的文档注释不会被当成后面函数的文档', () => {
    const functions = parseHeaderFunctions(
      '/** 宏。\n * @param a 参数。 */\n#define SB_X(a) (a)\n/** 函数。 */\nvoid sb_f(void);\n',
      'x.h',
    );
    assert.equal(functions.length, 1);
    assert.deepEqual(functions[0].paramDocs, []);
  });

  it('覆盖表：已封装与不封装的功能号互不重叠，不封装的都有原因', () => {
    const coverage = coverageOf(readFunctions());
    const unwrapped = readUnwrapped();
    for (const number of KNOWN_FUNCTIONS) {
      const wrapped = coverage.get(number).length > 0;
      const reason = unwrapped.functions[`$${number}`];
      assert.ok(wrapped !== Boolean(reason), `功能 $${number}：封装与原因必须二选一`);
      if (reason) assert.ok(reason.length >= 8, `功能 $${number} 的原因太短`);
    }
    for (const [key, reason] of Object.entries(unwrapped.partial)) {
      assert.ok(coverage.get(key.slice(1).toUpperCase()).length > 0, `${key} 写在 partial 里就必须有函数封装它`);
      assert.ok(reason.length >= 8);
    }
  });

  it('入库的 API 参考文档与头文件生成的一致', () => {
    const stored = readFileSync(API_DOC_FILE, 'utf8').replace(/\r\n/g, '\n');
    assert.equal(stored, renderApiReference(), '头文件注释或不封装清单改过了：运行 node tools/gen-sb2000-api-doc.mjs 重新生成 api-reference.md');
  });
});
