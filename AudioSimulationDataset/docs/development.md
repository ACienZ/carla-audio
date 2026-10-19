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

当前流程是任务分支 → `7-audio-gen-demo/main` → `main`。Demo 子任务分支保留 `7-audio-gen-demo/` 前缀。下面保留 #10 当时的创建命令作为历史示例；该任务已经合并归档，不要重新执行。后续新任务替换编号和名称；#8 已有关联分支，使用下文的旧分支同步流程。开始前确认工作区干净，已有修改先妥善保存。

```bash
git fetch origin --prune
gh issue develop 10 --repo ACienZ/carla-audio \
  --base 7-audio-gen-demo/main --name 7-audio-gen-demo/10-python-client-binding --checkout
gh issue develop 10 --repo ACienZ/carla-audio --list
git status --short --branch
git rev-parse HEAD origin/7-audio-gen-demo/main
```

创建时两条提交 ID 应相同，Issue Development 应显示关联分支，本地应检出 `7-audio-gen-demo/10-python-client-binding` 并跟踪同名远端分支。后续创建 PR 时仍显式核对目标为 `7-audio-gen-demo/main`，并确认 PR 出现在 Issue 的 Development 中。

创建 Issue 关联分支只使用 GitHub 已有的集成分支提交，不表示开发期间可以上传本地改动。分支改名后重新核对 Development 关联；不能只看分支名改成功就认为关联仍在。

### 本地开发、审阅与交付

依据 [Issue #3](https://github.com/ACienZ/carla-audio/issues/3#issuecomment-4759475264) 及用户最新确认，执行顺序为：

1. 在服务器任务分支修改代码或文档，完成对应检查，查看完整 `git diff` 和未跟踪文件。
2. 提供改动、验证结果和待审阅差异，保留未提交修改，等待用户审阅确认。
3. 用户确认后精确暂存已审阅文件，检查 `git diff --cached` 与 `git diff --cached --check`，再执行本地 commit。中间提交留在服务器，不 push。
4. 模块开发完成、验收检查完成且用户确认交付后，才 push 任务分支，再创建指向集成分支的 PR，并核对 Issue 关联。

用户对某一版本的本地提交确认不自动授权推送，也不覆盖后续新增改动。助手自查通过、项目管理授权、分支已在 GitHub 存在，都不替代上述审阅与交付确认。

### Squash 合并后的收尾

参考 [Issue #3](https://github.com/ACienZ/carla-audio/issues/3#issuecomment-4759475264)，采用用户最新约定：远端任务分支删除，本地任务分支改名归档。先确认 PR 已 squash 合并、交付验收完成，以及任务分支没有合并后新增的待交付提交；再核对 Issue 已关闭、Project 为 Done。当前集成分支不是仓库默认分支，不能依赖关闭关键词自动完成收尾。

以下以 #10 展示归档步骤；该任务现已完成以下收尾，不要重复执行：

```bash
git switch 7-audio-gen-demo/main
git pull --ff-only origin 7-audio-gen-demo/main
# 若已在 GitHub 删除远端任务分支，跳过下一条。
git push origin --delete 7-audio-gen-demo/10-python-client-binding
git branch -m 7-audio-gen-demo/10-python-client-binding archive/7-audio-gen-demo/10-python-client-binding_backup
# 归档只保留本地，不继续跟踪已删除的远端任务分支。
git branch --unset-upstream archive/7-audio-gen-demo/10-python-client-binding_backup
git fetch origin --prune
git branch -vv
```

归档分支不自动推送至 GitHub，不使用 `git branch -d/-D` 删除它。已有 `archive/19-project-foundation` 和 `archive/7-audio-gen-demo/9-carla-sensor` 保持原名，不为了新增 `_backup` 后缀再次改名。Squash 会生成新提交，不能单凭 `git branch --merged` 判断旧任务是否交付，应结合 PR 状态、合并提交和文件差异核对。

`git pull` 的远端和分支是两个参数：`git pull --ff-only origin 7-audio-gen-demo/main`。`origin/7-audio-gen-demo/main` 是远端跟踪引用，不能作为这里的远端名；在集成分支上执行 `git pull origin main` 取的是仓库 `main`，不会取得 Demo 集成分支更新。`git fetch --prune` 只更新并清理远端跟踪引用，不会自动更新本地工作分支或删除本地归档。

### 启动旧任务分支前更新

#8 的 `7-audio-gen-demo/8-scene-create` 建立较早。2026-10-19 启动任务时已按以下流程核对并同步；后续继续开发时仍先检查远端与工作区：

```bash
git fetch origin --prune
git switch 7-audio-gen-demo/8-scene-create
git status --short --branch
git log --oneline --left-right HEAD...origin/7-audio-gen-demo/8-scene-create
git log --oneline --left-right HEAD...origin/7-audio-gen-demo/main
```

先检查本地与同名远端是否有尚未同步的工作；可快进时用 `git merge --ff-only origin/7-audio-gen-demo/8-scene-create` 同步。若双方分叉，先审阅各自提交再合并，不重置或覆盖。

随后将最新集成分支合入任务分支：没有独有提交且可快进时使用 `git merge --ff-only origin/7-audio-gen-demo/main`；已有独有提交造成分叉时，使用 `git merge --no-commit --no-ff origin/7-audio-gen-demo/main` 准备合并，解决冲突并检查，交用户审阅确认后再创建本地合并提交，避免对已有共享历史 rebase 或强推。同步结果先留在服务器；仍需等模块完成且用户确认交付后才 push。

启动 #8 时已确认 #10 的 PR #21 合并，本地分支从 `ef3412669` 快进到 `935324bdc`，包含 #9、#19、#10 的成果。分支名和 Issue Development 关联保持不变，Project 已设为 In progress；该同步阶段远端 #8 分支保留原提交，没有提前推送。以后同步也先核对依赖确已进入集成分支。

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

### #10 静态接收入口

脚本与说明位于 [audio_source_tests](../tests/audio_source_tests/README.md)。在服务端已 Play、没有其他客户端推进世界时运行：

```bash
conda activate carla
cd /home/test/carla-0.9.16
python AudioSimulationDataset/tests/audio_source_tests/receive_audio_source.py
```

默认创建两辆间隔 15 米的静止车辆，设置同步步长 0.05 秒，接收 10 次事件，打印事件自身的帧号、仿真时间戳和距离。参数集中在脚本顶部；若出生位置被占用，调整出生点索引，不增加自动重试。脚本不切换地图，不保存数据，也不要求 DatasetGenerator 已实现。

回调将事件放入队列，主循环推进世界并等待事件。超时会直接报错；`finally` 负责停止监听、销毁本次对象和恢复世界设置。服务端断连时清理也可能失败，当前不提供复杂恢复机制。

### #10 验证范围与结果

- 2026-10-19：Conda `carla` 可以导入 `carla.AudioSourceEvent`，实际包位于 `/home/test/miniconda3/envs/carla/lib/python3.11/site-packages/carla/__init__.py`。
- 前期助手连接本机服务端超时；此后用户实际运行成功，提供了连续 10 次测量（frame 9387～9396），时间戳间隔 0.05 秒、距离均显示 15.000 米，并执行到清理末尾。完整输出见 [用户运行记录](../tests/audio_source_tests/README.md#本轮检查)，不作为助手重跑结果。清理后的独立状态核验、再次运行和异常清理尚无新增证据。
- 此次运行有客户端 `ef3412669-dirty` 与服务端 `a3f0e372e-dirty` 的版本警告。源码仅比较版本字符串；标识来源与判断边界见 [版本警告说明](../tests/audio_source_tests/README.md#版本警告的含义)。当前接收成功不等于所有 API 全面兼容，本轮不隐藏警告或为消除标识差异全量重编。
- 本脚本只验证能接收并访问 frame、timestamp、distance_m。它不将事件帧号假定为刚 tick 的帧，也不做同帧快照对照或测距误差计算。
- [#9 历史记录](https://github.com/ACienZ/carla-audio/issues/9#issuecomment-6250272390) 包含更完整的静态测试及用户成功说明；这些不是本脚本的运行结果。
- 严格的动态测距精度验证需要用同帧世界快照中的 Actor 原点计算独立参考距离；不得把参考值替代为 sensor 测量。#8 当前只按已确认的计划距离初步核对线性变化，不宣称完成独立快照精度验证。

### #8 匀速接近与采集入口

脚本和说明位于 [scene_create_tests](../tests/scene_create_tests/README.md)。使用同一 Conda 环境，服务端 Play 后运行：

```bash
python AudioSimulationDataset/tests/scene_create_tests/create_scene.py
```

先选择适合当前地图的直路出生点，修改脚本顶部索引；不自动选路。默认 ego 静止，救护车关闭物理后按步数设置位置，从 30 米以 2 米/秒接近到 10 米。固定步长 0.05 秒，含初始位置共记录 201 次测量，首末时间差约 10 秒。

每次 tick 后核对事件帧号；超时或帧号不符直接失败并进入清理。完整接收后保存 `tests/scene_create_tests/data/<运行时间>.json`，仅包含 `measurements` 原始测量，不保存运行配置；该数据目录已忽略。JSON 的读取方法、初步容差和异常边界见脚本说明。本次只为 #11 提供输入，不生成音频或修改 sensor。

2026-10-19 用户实际运行后确认正常，助手回读该次 JSON：frame 1658～1858 共 201 条，首末时间差约 10 秒，距离从 30 米降至约 10 米，与计划距离的最大偏差约 3.81 × 10⁻⁶ 米。该结果来自用户运行，不是助手重跑；完整核对及清理、版本警告等限制见 [运行记录](../tests/scene_create_tests/README.md#用户实测与-json-回读2026-10-19)。

## 正式生成器与批量运行

目录与职责见 [DatasetGenerator](../DatasetGenerator/README.md)，场景和数据设想见 [设计概览](planning/project-overview.md)。#10 的独立脚本使用顶部参数；正式生成器再实现配置读取。YAML 为推荐，格式未定，本轮不增加依赖。

后续批量执行将由脚本生成具体配置，并复用场景控制器运行。无头图像采集使用离屏渲染，不能开启会使相机返回空数据的 `no_rendering_mode`。打包服务端、自定义 sensor 集成、无头启动和批量调度尚未验收，不能把官方支持等同于本项目已经跑通。参考 [CARLA 渲染选项](https://carla.readthedocs.io/en/0.9.16/adv_rendering_options/)。

## 文档交付检查

#19 已通过 PR #20 交付：检查了六份 Markdown 文档的链接、仓库路径、命令语法、接口说明、完整 diff，以及 GitHub 字段回读；这些检查不代表 CARLA 运行时测试、音频验收或 CI 已通过。

#10 的架构说明、静态接收脚本、补充注释及验收记录已通过用户审阅，并随 PR #21 合并；检查过 Python AST、差异、链接、Shell 示例语法和空白。任务已关闭并归档。

#8 本轮新增动态采集脚本、运行说明与数据忽略规则，同步上述进度。用户已审阅并提供实际运行结果，随后授权补充验收记录并 commit、push、创建 PR。交付按精确暂存、检查暂存差异、本地提交、推送、创建目标为 `7-audio-gen-demo/main` 的 PR 顺序执行；本轮停在 PR 创建完成，#8 保持 In progress，等待用户审阅合并。检查结果与未验证部分记录在 [TODO](../TODO.md)，不修改 Actions/CD。
