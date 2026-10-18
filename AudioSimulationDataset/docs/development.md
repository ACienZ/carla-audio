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

## 本轮文档交付检查

本轮 #19 检查 Markdown 链接、仓库路径、命令语法、接口说明、完整 diff，以及 GitHub 字段回读；不会据此声明 CARLA 运行时测试、音频验收或 CI 已通过。

提交时仅暂存本轮六份 Markdown 文档。生成索引、日志、音频、配置与凭证不属于本轮交付。Actions/CD 和业务测试入口等对应需求确认后再处理。
