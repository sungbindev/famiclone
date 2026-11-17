/*
 * ------------------------------------------------------------------------
 * 名称：publish-update.d.mts
 * 说明：升级文件发布脚本的类型声明，供测试引用。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-18
 * ------------------------------------------------------------------------
 */

/** RELEASES 里的一个条目。 */
export interface ReleaseEntry {
  readonly sha1: string;
  readonly file: string;
  readonly size: number;
}

/** 解析 RELEASES 的内容。 */
export function parseReleases(text: string): ReleaseEntry[];

/** 发布计划的输入：版本号、RELEASES 的内容、目录里各文件的大小与 SHA1，以及安装程序（.exe）的文件名（不给就不传 zip）。 */
export interface PublishInput {
  readonly version: string;
  readonly releases: string;
  readonly files: ReadonlyMap<string, { readonly size: number; readonly sha1: string }>;
  readonly installer?: string;
}

/** 要上传的一个文件：zipOf 是要打包的安装程序文件名，只有安装程序的 zip 有。 */
export interface UploadItem {
  readonly file: string;
  readonly key: string;
  readonly zipOf?: string;
}

/** 检查要发布的文件并排出上传顺序（.nupkg 在前，安装程序的 zip 在中间，RELEASES 在最后）。 */
export function planPublish(input: PublishInput): { readonly uploads: ReadonlyArray<UploadItem> };

/** 把安装程序打成只含这一个文件的 zip。 */
export function buildInstallerZip(installerPath: string, zipPath: string): void;
