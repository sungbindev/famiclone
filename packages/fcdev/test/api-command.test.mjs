// ------------------------------------------------------------------------
// 名称：api-command.test.mjs
// 说明：测试 fcdev api 命令与头文件文档解析：列出模块、按函数名、模块名、系统调用号、关键词查询，以及错误提示。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：不启动子进程，直接调用 runCli；查询的是内置 sb2000 目标的头文件，不需要 cc65 与模拟器。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { parseHeaderFunctions, queryApi, syscallMatches } from '../src/api/index.mjs';
import { runCli } from '../src/cli/index.mjs';
import { createTempDir } from './helpers.mjs';

describe('fcdev api 命令', () => {
  let temp;
  let stdout;
  let stderr;

  /** 运行命令行并返回退出码 */
  function run(argv) {
    return runCli(argv, {
      cwd: temp.dir,
      env: process.env,
      stdout: { write: (text) => stdout.push(text) },
      stderr: { write: (text) => stderr.push(text) },
    });
  }

  beforeEach(() => {
    temp = createTempDir();
    stdout = [];
    stderr = [];
  });

  afterEach(() => temp.cleanup());

  it('不带查询词列出全部模块与函数个数', async () => {
    assert.equal(await run(['api', '--target', 'sb2000']), 0);
    const text = stdout.join('');
    assert.match(text, /个函数，\d+ 个模块/);
    for (const name of ['file', 'disk', 'display', 'math', 'sound', 'printer', 'ime']) assert.match(text, new RegExp(`\\b${name}\\b`));
    assert.match(text, /fcdev api <函数名>/);
  });

  it('按函数名显示完整文档：签名、说明、参数、返回、系统调用', async () => {
    assert.equal(await run(['api', '--target', 'sb2000', 'sb_file_open']), 0);
    const text = stdout.join('');
    assert.match(text, /unsigned char sb_file_open\(const char \*path, unsigned char mode, unsigned char \*handle\);/);
    assert.match(text, /系统调用：\$7B\/\$09/);
    assert.match(text, /参数：/);
    assert.match(text, /handle {2}返回句柄号/);
    assert.match(text, /返回：0 成功/);
  });

  it('按模块名与头文件名列出函数，按系统调用号查函数', async () => {
    assert.equal(await run(['api', '--target', 'sb2000', 'disk']), 0);
    assert.match(stdout.join(''), /模块 disk：\d+ 个函数/);
    assert.match(stdout.join(''), /sb_disk_read_sector/);
    stdout.length = 0;
    assert.equal(await run(['api', '--target', 'sb2000', 'sb2000-disk.h']), 0);
    assert.match(stdout.join(''), /sb_disk_format_track/);
    stdout.length = 0;
    assert.equal(await run(['api', '--target', 'sb2000', '$80/$02']), 0);
    assert.match(stdout.join(''), /系统调用 \$80\/\$02：1 个函数/);
    assert.match(stdout.join(''), /sb_disk_read_sector/);
    stdout.length = 0;
    assert.equal(await run(['api', '--target', 'sb2000', '$7b']), 0);
    assert.match(stdout.join(''), /sb_file_open/);
  });

  it('按关键词搜索名称与说明', async () => {
    assert.equal(await run(['api', '--target', 'sb2000', '格式化']), 0);
    assert.match(stdout.join(''), /sb_disk_format_track/);
  });

  it('--json 输出机器可读的结果', async () => {
    assert.equal(await run(['api', '--target', 'sb2000', '--json', 'sb_clock_get_time']), 0);
    const [fn] = JSON.parse(stdout.join(''));
    assert.equal(fn.name, 'sb_clock_get_time');
    assert.deepEqual(fn.params, ['time']);
    assert.deepEqual(fn.syscalls, ['$7F/$04']);
  });

  it('找不到时报错并提示；没有指定目标、目标没有文档时也报错', async () => {
    assert.equal(await run(['api', '--target', 'sb2000', 'zzzz_not_exist']), 1);
    assert.match(stderr.join(''), /没有找到“zzzz_not_exist”/);
    stderr.length = 0;
    assert.equal(await run(['api']), 1);
    assert.match(stderr.join(''), /没有指定目标/);
    stderr.length = 0;
    assert.equal(await run(['api', '--target', 'nes']), 1);
    assert.match(stderr.join(''), /没有 C 接口文档/);
  });

  it('解析文档注释：说明全文、参数与返回值的续行', () => {
    const [fn] = parseHeaderFunctions(
      '/**\n * 第一段。\n * 第二行。\n *\n * @syscall $10\n * @param a 参数 a，\n *        续行。\n * @return 返回值。\n */\nunsigned char sb_x(unsigned char a);\n',
      'sb2000-x.h',
    );
    assert.equal(fn.description, '第一段。\n第二行。');
    assert.equal(fn.paramText.a, '参数 a， 续行。');
    assert.equal(fn.returnText, '返回值。');
    assert.equal(fn.signature, 'unsigned char sb_x(unsigned char a)');
  });

  it('系统调用标签的匹配规则：单个、范围、多个并列', () => {
    assert.equal(syscallMatches('$7B/$09', '7b'), true);
    assert.equal(syscallMatches('$7B/$09', '7b', '09'), true);
    assert.equal(syscallMatches('$7B/$09', '7b', '0a'), false);
    assert.equal(syscallMatches('$30/$04-$07', '30', '06'), true);
    assert.equal(syscallMatches('$30/$04-$07', '30', '08'), false);
    assert.equal(syscallMatches('$3C、$3D、$3E、$3F', '3e'), true);
    assert.equal(syscallMatches('$7B/$01、$7B/$06', '7b', '06'), true);
    assert.equal(syscallMatches('直接写 $0696-$0699', '06'), false);
  });

  it('查询优先级：函数名先于模块名先于系统调用号先于关键词', () => {
    const functions = parseHeaderFunctions(
      '/**\n * 读。\n * @syscall $20\n */\nvoid sb_a_read(void);\n/**\n * 写。\n * @syscall $21\n */\nvoid sb_a_write(void);\n',
      'sb2000-a.h',
    );
    assert.equal(queryApi(functions, 'sb_a_read').kind, 'function');
    assert.equal(queryApi(functions, 'a').kind, 'module');
    assert.equal(queryApi(functions, '$21').kind, 'syscall');
    assert.equal(queryApi(functions, '写').kind, 'text');
    assert.equal(queryApi(functions, '没有').kind, 'none');
  });
});
