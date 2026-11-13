#!/usr/bin/env bash
# ------------------------------------------------------------------------
# 名称：package-mac.sh
# 说明：在 macOS 上打包 arm64 版：生成 Schema 代码，编译原生模块，单独导出调试符号并去除原生模块中的符号，再用 Electron Forge 打包。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-13
# 备注：只能在苹果电脑上运行；调试符号保存在 core/build/Release/ember_core.node.dSYM，不随安装包分发。
# ------------------------------------------------------------------------

set -euo pipefail

cd "$(dirname "$0")/../.."

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "只能在 macOS 上运行" >&2
  exit 1
fi

npm run schema
node tools/build/build-core.mjs --arch arm64
dsymutil core/build/Release/ember_core.node -o core/build/Release/ember_core.node.dSYM
strip -x core/build/Release/ember_core.node

npx electron-forge package --platform darwin --arch arm64
