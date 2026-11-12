// ------------------------------------------------------------------------
// 名称：prompts.mjs
// 说明：命令行交互：让用户在几个选项里选一个，输入序号或名称都可以。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用 node:readline，不引入依赖；整个创建流程共用一个输入迭代器，避免多个问题争抢同一个输入流；输入不合法时重新询问，最多 3 次。
// ------------------------------------------------------------------------
import { createInterface } from 'node:readline';
import { CreateError } from './errors.mjs';

/** 同一个问题最多询问的次数 */
const MAX_ATTEMPTS = 3;

/**
 * 创建提问器：从输入流逐行读取回答。
 * @param {{input: NodeJS.ReadableStream, output: {write: (text: string) => unknown}}} streams 输入流与输出流
 * @returns {{askChoice: (request: {question: string, choices: {value: string, label: string}[]}) => Promise<string>, close: () => void}} 提问器；用完要 close
 */
export function createPrompter({ input, output }) {
  const readline = createInterface({ input });
  const lines = readline[Symbol.asyncIterator]();

  /** 读下一行回答，输入结束时报错 */
  async function readAnswer() {
    output.write('请输入序号或名称：');
    const { value, done } = await lines.next();
    if (done) throw new CreateError('没有收到输入', { code: 'PROMPT_CLOSED' });
    return value.trim();
  }

  return {
    /**
     * 询问用户在若干选项里选一个。
     * @param {{question: string, choices: {value: string, label: string}[]}} request 问题与选项
     * @returns {Promise<string>} 选中的 value
     * @throws {CreateError} 连续输入不合法，或输入流结束
     */
    async askChoice({ question, choices }) {
      const list = choices.map((choice, index) => `  ${index + 1}) ${choice.label}`).join('\n');
      for (let attempt = 0; attempt < MAX_ATTEMPTS; attempt += 1) {
        output.write(`${question}\n${list}\n`);
        const answer = await readAnswer();
        const byNumber = Number(answer);
        const chosen = Number.isInteger(byNumber) && byNumber >= 1 && byNumber <= choices.length ? choices[byNumber - 1] : choices.find((choice) => choice.value === answer);
        if (chosen) return chosen.value;
        output.write(`“${answer}”不是可选的项\n`);
      }
      throw new CreateError(`连续 ${MAX_ATTEMPTS} 次输入不合法`, { code: 'PROMPT_INVALID' });
    },
    close() {
      readline.close();
    },
  };
}
