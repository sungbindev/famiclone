// ------------------------------------------------------------------------
// 名称：docs-command.test.mjs
// 说明：测试 fcdev docs 命令与文档主题登记：列出主题、按主题名、文件名、关键词查询，以及登记与 docs 目录、架构文档清单一致。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：不启动子进程，直接调用 runCli；不需要 cc65 与模拟器。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync, readdirSync, readFileSync, statSync } from 'node:fs';
import path from 'node:path';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { runCli } from '../src/cli/index.mjs';
import { DOC_TOPICS, docsDirectory, queryDocs } from '../src/docs/index.mjs';
import { createTempDir } from './helpers.mjs';

/**
 * 列出目录下全部 Markdown 文件，路径相对 root，用 / 分隔。
 * @param {string} root 根目录
 * @param {string} [dir] 当前目录
 * @returns {string[]} 文件路径
 */
function listMarkdown(root, dir = root) {
  return readdirSync(dir).flatMap((name) => {
    const full = path.join(dir, name);
    if (statSync(full).isDirectory()) return listMarkdown(root, full);
    return name.endsWith('.md') ? [path.relative(root, full).split(path.sep).join('/')] : [];
  });
}

describe('fcdev docs 命令', () => {
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

  it('不带查询词列出全部主题，不需要 fcdev.json', async () => {
    assert.equal(await run(['docs']), 0);
    const text = stdout.join('');
    assert.match(text, new RegExp(`fcdev 使用文档：${DOC_TOPICS.length} 份`));
    for (const { topic } of DOC_TOPICS) assert.match(text, new RegExp(`\\b${topic}\\b`));
    assert.match(text, /fcdev docs <主题>/);
  });

  it('按主题名显示本机路径', async () => {
    assert.equal(await run(['docs', 'syscall']), 0);
    const text = stdout.join('');
    assert.match(text, /SB-DOS 系统调用手册（主题 syscall）/);
    const local = /本机：(.+)\n/.exec(text)[1];
    assert.equal(existsSync(local), true);
    assert.equal(local, path.join(docsDirectory(), 'sb2000', '02-系统调用手册.md'));
    assert.doesNotMatch(text, /在线|https?:\/\//);
  });

  it('--json 的结果不含在线地址', async () => {
    assert.equal(await run(['docs', 'build', '--json']), 0);
    const [doc] = JSON.parse(stdout.join(''));
    assert.equal(doc.file, '01-命令行与构建.md');
    assert.equal('onlineUrl' in doc, false);
  });

  it('api 主题指向目标目录里的接口参考', async () => {
    assert.equal(await run(['docs', 'api']), 0);
    const text = stdout.join('');
    const local = /本机：(.+)\n/.exec(text)[1];
    assert.equal(local, path.join(docsDirectory(), '..', 'targets', 'sb2000', 'api-reference.md'));
    assert.equal(existsSync(local), true);
  });

  it('--json 不带查询词输出全部文档，路径都存在', async () => {
    assert.equal(await run(['docs', '--json']), 0);
    const docs = JSON.parse(stdout.join(''));
    assert.equal(docs.length, DOC_TOPICS.length);
    for (const doc of docs) assert.equal(existsSync(doc.localPath), true, doc.localPath);
  });

  it('按文件名与关键词查询：主题名先于文件名先于关键词', () => {
    assert.equal(queryDocs('build').kind, 'topic');
    assert.deepEqual(queryDocs('01-命令行与构建').matches.map((doc) => doc.topic), ['build']);
    assert.deepEqual(queryDocs('系统调用手册').matches.map((doc) => doc.topic), ['syscall']);
    const file = queryDocs('反汇编');
    assert.equal(file.kind, 'file');
    assert.deepEqual(file.matches.map((doc) => doc.topic), ['disasm']);
    const text = queryDocs('运行');
    assert.equal(text.kind, 'text');
    assert.ok(['emulator', 'nes'].every((topic) => text.matches.some((doc) => doc.topic === topic)));
    assert.equal(queryDocs('zzzz_not_exist').kind, 'none');
  });

  it('关键词匹配到多份文档时列出摘要', async () => {
    assert.equal(await run(['docs', '运行']), 0);
    const text = stdout.join('');
    assert.match(text, /包含“运行”的文档：\d+ 份/);
    assert.match(text, /nes/);
    assert.match(text, /emulator/);
  });

  it('找不到时报错并提示', async () => {
    assert.equal(await run(['docs', 'zzzz_not_exist']), 1);
    assert.match(stderr.join(''), /没有找到“zzzz_not_exist”/);
    assert.match(stderr.join(''), /fcdev docs/);
  });
});

describe('文档主题登记', () => {
  it('登记的 docs 目录文件与目录里的 Markdown 文件一一对应，包内其他位置的文件都存在', () => {
    const registered = DOC_TOPICS.filter((entry) => !entry.inPackage).map((entry) => entry.file).sort();
    assert.deepEqual(registered, listMarkdown(docsDirectory()).sort());
    for (const entry of DOC_TOPICS.filter((item) => item.inPackage)) {
      assert.equal(existsSync(path.join(docsDirectory(), '..', ...entry.file.split('/'))), true, entry.file);
    }
  });

  it('主题名只含小写字母、数字与连字符，且不重复', () => {
    const topics = DOC_TOPICS.map((entry) => entry.topic);
    for (const topic of topics) assert.match(topic, /^[a-z][a-z0-9-]*$/);
    assert.equal(new Set(topics).size, topics.length);
  });

  it('架构文档（仓库根目录的 docs）的使用文档清单列出了每个主题与文件', () => {
    const text = readFileSync(path.join(docsDirectory(), '..', '..', '..', 'docs', '01-软件架构设计.md'), 'utf8');
    for (const { topic, file } of DOC_TOPICS) {
      assert.ok(text.includes(`| \`${topic}\` |`), `文档清单缺少主题 ${topic}`);
      const label = path.posix.basename(file, '.md');
      assert.ok(text.includes(label), `文档清单缺少 ${label}`);
    }
  });
});
