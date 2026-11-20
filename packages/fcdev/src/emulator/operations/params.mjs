// ------------------------------------------------------------------------
// 名称：params.mjs
// 说明：模拟器操作参数的读取与检查：错误统一成 EMULATOR_PARAM_INVALID。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-21
// 备注：通用的检查函数在 shared/param-checks.mjs，这里只指定错误码；地址可以写成整数或 "$C000" 这样的十六进制文字。
// ------------------------------------------------------------------------
import { createParamChecks } from '../../shared/index.mjs';

export const {
  invalidParam, requireString, optionalString, optionalInteger, optionalBoolean, optionalEnum, toAddress, optionalAddress, optionalArray,
} = createParamChecks('EMULATOR_PARAM_INVALID');
