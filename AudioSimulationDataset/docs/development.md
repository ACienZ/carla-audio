# 开发与验证指南

本文基于当前服务器和仓库整理，不是一次全新机器安装教程。环境可能变化，执行前先核对。进度见 [TODO](../TODO.md)，协作要求见 [AGENTS](../AGENTS.md)。

## 环境与目录

截至 2026-10-19 的环境：Ubuntu 22.04、CARLA 0.9.16、UE4.26、Conda `carla` 环境中的 Python 3.11。CARLA 仓库位于 `/home/test/carla-0.9.16`，UE4 位于 `/home/test/UnrealEngine_4.26`。

构建 PythonAPI 与运行客户端使用同一 Conda 环境。先在已初始化 Conda 的终端执行：

```bash
conda activate carla
cd /home/test/carla-0.9.16
which python
which python3
python --version
python3 --version
```

两个解释器都应来自选定环境。当前路径不适用于其他机器时替换为实际安装位置，不通过安装另一个官方 CARLA 包来掩盖自定义绑定缺失。

```bash
export UE4_ROOT=/home/test/UnrealEngine_4.26
test -f "$UE4_ROOT/GenerateProjectFiles.sh"
```

这个 export 只对当前终端有效。后续业务开发在 `AudioSimulationDataset/`，构建命令仍从 CARLA 仓库根目录执行。

## Issue 分支与同步

当前流程是任务分支 → `7-audio-gen-demo/main` → `main`。下面的 #10 命令是本次创建记录，分支已经存在时不要重复创建；后续新任务替换编号和名称。开始前确认工作区干净，已有修改先妥善保存。

```bash
git fetch origin --prune
gh issue develop 10 --repo ACienZ/carla-audio \
  --base 7-audio-gen-demo/main --name 10-python-client-binding --checkout
gh issue develop 10 --repo ACienZ/carla-audio --list
git status --short --branch
git rev-parse HEAD origin/7-audio-gen-demo/main
```

创建时两条提交 ID 应相同，Issue Development 应显示关联分支，本地应检出 `10-python-client-binding` 并跟踪同名远端分支。后续创建 PR 时仍显式核对目标为 `7-audio-gen-demo/main`，并确认 PR 出现在 Issue 的 Development 中。

### Squash 合并后的收尾

参考 [Issue #3](https://github.com/ACienZ/carla-audio/issues/3#issuecomment-4759475264)，采用用户最新约定：远端任务分支删除，本地任务分支改名归档。先确认 PR 已 squash 合并、交付验收完成，以及任务分支没有合并后新增的待交付提交；再核对 Issue 已关闭、Project 为 Done。当前集成分支不是仓库默认分支，不能依赖关闭关键词自动完成收尾。

以下以尚未完成的 #10 为未来操作示例，现在不要执行：

```bash
git switch 7-audio-gen-demo/main
git pull --ff-only origin 7-audio-gen-demo/main
# 若已在 GitHub 删除远端任务分支，跳过下一条。
git push origin --delete 10-python-client-binding
git branch -m 10-python-client-binding archive/10-python-client-binding_backup
# 归档只保留本地，不继续跟踪已删除的远端任务分支。
git branch --unset-upstream archive/10-python-client-binding_backup
git fetch origin --prune
git branch -vv
```

归档分支不自动推送至 GitHub，不使用 `git branch -d/-D` 删除它。已有 `archive/19-project-foundation` 和 `archive/7-audio-gen-demo/9-carla-sensor` 保持原名，不为了新增 `_backup` 后缀再次改名。Squash 会生成新提交，不能单凭 `git branch --merged` 判断旧任务是否交付，应结合 PR 状态、合并提交和文件差异核对。

`git pull` 的远端和分支是两个参数：`git pull --ff-only origin 7-audio-gen-demo/main`。`origin/7-audio-gen-demo/main` 是远端跟踪引用，不能作为这里的远端名；在集成分支上执行 `git pull origin main` 取的是仓库 `main`，不会取得 Demo 集成分支更新。`git fetch --prune` 只更新并清理远端跟踪引用，不会自动更新本地工作分支或删除本地归档。

### 启动旧任务分支前更新

#8 的 `7-audio-gen-demo/8-scene-create` 建立较早。开始该任务时再执行以下核对，本次不修改它：

```bash
git fetch origin --prune
git switch 7-audio-gen-demo/8-scene-create
git status --short --branch
git log --oneline --left-right HEAD...origin/7-audio-gen-demo/8-scene-create
git log --oneline --left-right HEAD...origin/7-audio-gen-demo/main
```

先检查本地与同名远端是否有尚未同步的工作；可快进时用 `git merge --ff-only origin/7-audio-gen-demo/8-scene-create` 同步。若双方分叉，先审阅各自提交再合并，不重置或覆盖。

随后将最新集成分支合入任务分支：没有独有提交且可快进时使用 `git merge --ff-only origin/7-audio-gen-demo/main`；已有独有提交造成分叉时，审阅后使用普通 `git merge origin/7-audio-gen-demo/main` 并解决冲突，避免对已有共享历史 rebase 或强推。验证差异和依赖后普通 push 同名任务分支。

更新必须包含 #9 和 #19；若 #8 依赖 #10 的成果，还要确认 #10 已合入集成分支再同步。#10 尚未合并时，不把它当作集成分支已有内容。保留 #8 的旧分支名，并核对其 Issue Development 关联。

## 构建 sensor 相关改动

下面是服务端和绑定一起变化时的完整检查顺序，逐条执行，上一条成功后再继续。纯文档修改不需要执行这些构建。

```bash
cd /home/test/carla-0.9.16
make LibCarla.server.release
make CarlaUE4Editor
make PythonAPI
```

- `LibCarla.server.release` 构建服务端库并提供 Unreal 依赖。
- `CarlaUE4Editor` 使用根目录构建入口，处理相关依赖、插件和编辑器，包括生成反射头文件。
- `PythonAPI` 构建客户端库与 Python 绑定。检查实际输出和产物，不能仅因已有旧安装而认为新绑定可用。
- 只修改部分组件时按受影响依赖选择构建；不默认运行会清理缓存的 `make rebuild` 或 `make clean`。
- 不同时启动多个构建，不手动修改 `CarlaDependencies/include` 等构建安装副本；源文件在 `LibCarla/source`。

查看产物与当前导入位置：

```bash
ls -lh PythonAPI/carla/dist/
python - <<'PY'
import sys
import carla

print("python:", sys.executable)
print("using carla:", carla.__file__)
print("AudioSourceEvent:", hasattr(carla, "AudioSourceEvent"))
PY
```

导入检查不连接服务器，也不证明编译产物与运行服务端一致。如果未找到 `AudioSourceEvent`，先检查构建结果和实际加载的包；若使用 wheel/egg，确保导入的是为该环境构建的自定义版本。

## 编辑器索引与头文件提示

完整操作参考 [Issue #16](https://github.com/ACienZ/carla-audio/issues/16)。Unreal、LibCarla、PythonAPI 的构建和编辑器解析配置不同，IntelliSense 红线本身不是编译失败的证据。

需要重新生成 Unreal 项目索引时，在上述构建成功后执行：

```bash
cd /home/test/carla-0.9.16/Unreal/CarlaUE4
"$UE4_ROOT/GenerateProjectFiles.sh" -project="$PWD/CarlaUE4.uproject" -game -engine -vscode
cd /home/test/carla-0.9.16
```

仓库现有 [fix_head_include.sh](../../fix_head_include.sh) 用于修复本机 Unreal IntelliSense 配置，不是业务逻辑或构建步骤。它含固定的安装路径、UE Clang 工具链、`Carla.1.rsp` 和配置名；先与本次生成结果核对，必要时才运行：

```bash
bash ./fix_head_include.sh
```

脚本会生成修正版编译数据库、修改本地 `c_cpp_properties.json` 并保留首次修改前的备份。索引重新生成后应重新核对这些配置。本轮没有移动脚本或工作区文件。

## 运行与测试

当前已验证的启动方式是在服务器图形桌面的终端中激活相同环境，从仓库根目录执行：

```bash
make launch
```

随后在 UE 编辑器中点击 Play，再在另一终端运行客户端。此步骤需要可用的图形会话，普通 SSH 终端不应假设能够显示编辑器；其他启动方式尚未作为本项目流程验证。

开始测试前应打印客户端的 `carla.__file__`，核对使用的新绑定，并确保服务端也来自对应构建。使用同步模式时，仅允许一个客户端推进世界，避免测量与快照跨帧。

### 当前已有与缺少的验证

- sensor 静态端到端测试的代码和历史成功记录位于 [#9 的评论](https://github.com/ACienZ/carla-audio/issues/9#issuecomment-6250272390)。历史对话也提供了连续 10 帧、15 米距离的成功输出。
- 该脚本尚未提交到当前仓库，因此这里没有宣称已经存在可运行的仓库测试入口。
- #10 的原计划保持不变；后续正式测试应放入 `AudioSimulationDataset/` 中的测试目录，提交后再补上准确运行命令。
- 静止车辆验证不能排除移动时距离落后一帧的问题；动态距离验证、长时间接收完整性和离线声音验收仍需后续实现。

### 后续测试应说明的内容

1. 测试的环境、代码版本、运行命令以及实际导入包。
2. Blueprint、事件类型、帧号、时间戳和 `distance_m` 是否正确。
3. 参考距离来自相同帧的世界快照，测量双方都是车辆 Actor 原点；参考值仅用于验证，不代替 sensor 测量。
4. 等待超时、迟到事件、缺失记录的区别。不得把无效距离写成零或将估算值混入原始记录。
5. 正常结束或异常时如何停止监听、销毁本次 actor，并恢复原世界设置。

## 文档交付检查

#19 已通过 PR #20 交付：检查了六份 Markdown 文档的链接、仓库路径、命令语法、接口说明、完整 diff，以及 GitHub 字段回读；这些检查不代表 CARLA 运行时测试、音频验收或 CI 已通过。

#10 的本次准备仅修改协作与进度文档，检查链接、Shell 示例语法、diff、Issue 分支关联和 Project 状态。客户端代码和测试仍待后续实施。提交时精确暂存本次文档；生成索引、日志、音频、配置与凭证不属于文档交付。Actions/CD 仍待另行确认。
