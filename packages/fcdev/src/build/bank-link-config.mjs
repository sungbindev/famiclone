// ------------------------------------------------------------------------
// 名称：bank-link-config.mjs
// 说明：多页（换页）程序的链接配置：按库页个数把目标的链接配置模板展开成完整的链接配置，并给出这种程序用的打包选项。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：模板里写固定的部分与库页 generatedFrom 之前的库页，%BANK_MEMORY% 与 %BANK_SEGMENTS% 两行分别展开成
//       库页 generatedFrom 到 N 的内存区与段；定义见 docs/03-目标规范.md 第 2 节的 banked。
// ------------------------------------------------------------------------
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { FcdevError } from '../shared/index.mjs';
import { resolveTargetPath } from '../targets/index.mjs';

/** 模板里内存区的展开位置 */
const MEMORY_MARKER = '%BANK_MEMORY%';

/** 模板里段的展开位置 */
const SEGMENT_MARKER = '%BANK_SEGMENTS%';

/**
 * 取程序的库页个数。
 * @param {import('./build-plan.mjs').BuildPlan} plan 构建计划
 * @returns {number} 库页个数；目标不支持多页或选项为 0 时为 0
 */
export function getBankCount(plan) {
  const { banked } = plan.target;
  return banked ? plan.targetOptions[banked.option] : 0;
}

/**
 * 把模板展开成完整的链接配置。
 * @param {string} template 链接配置模板
 * @param {import('../targets/index.mjs').BankedSpec} banked 目标的多页定义
 * @param {number} count 库页个数
 * @returns {string} 链接配置
 * @throws {FcdevError} 模板里缺少展开位置
 */
export function expandBankedLinkConfig(template, banked, count) {
  const lines = template.split(/\r?\n/);
  const generate = (pattern) => {
    const generated = [];
    for (let bank = banked.generatedFrom; bank <= count; bank += 1) generated.push(`    ${pattern.replaceAll('{n}', String(bank))}`);
    return generated;
  };
  const expand = (marker, pattern) => {
    const index = lines.findIndex((line) => line.trim() === marker);
    if (index < 0) throw new FcdevError(`链接配置模板里缺少 ${marker} 一行`, { code: 'TARGET_INVALID' });
    lines.splice(index, 1, ...generate(pattern));
  };
  expand(MEMORY_MARKER, banked.memory);
  expand(SEGMENT_MARKER, banked.segment);
  return lines.join('\n');
}

/**
 * 确定链接用的配置文件与打包选项：多页程序展开模板并写到中间文件目录，普通程序直接用目标的链接配置。
 * @param {import('./build-plan.mjs').BuildPlan} plan 构建计划
 * @param {string} toolchainHome 工具链根目录
 * @returns {{linkConfig: string, packagerOptions: Record<string, unknown>}} 链接配置的路径与打包选项
 */
export function resolveLinkSetup(plan, toolchainHome) {
  const { target } = plan;
  const count = getBankCount(plan);
  if (count === 0) {
    return { linkConfig: resolveTargetPath(target, target.linkConfig, toolchainHome), packagerOptions: target.packagerOptions };
  }
  const template = readFileSync(resolveTargetPath(target, target.banked.linkConfig, toolchainHome), 'utf8');
  const linkConfig = path.join(plan.objectDir, `${plan.programName}.cfg`);
  writeFileSync(linkConfig, expandBankedLinkConfig(template, target.banked, count));
  // 多页程序的大小由库页个数决定，不再用单页程序的大小上限检查
  const { maxSize: _unused, ...packagerOptions } = target.packagerOptions;
  return { linkConfig, packagerOptions };
}
