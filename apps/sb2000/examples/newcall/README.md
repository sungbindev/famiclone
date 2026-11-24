# newcall

调用 BIOS 新增的系统调用 `$35`，显示它返回的扩展版本号；BIOS 不支持时显示 `NOT SUPPORTED`。

## 构建与运行

用 VS Code 的“FC Dev：构建”任务构建，产物在 `build/sb2000/NEWCALL.EXE`。

在 SB-DOS 的 `A:\>` 下输入：

```text
newcall
```
