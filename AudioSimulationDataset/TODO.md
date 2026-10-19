# 任务与进度摘要

核对日期：2026-10-19（Asia/Tokyo）。实际进度以 [carla-audio tracker](https://github.com/users/ACienZ/projects/9) 为准，后续开始任务时重新读取；本表不是实时看板。

## 当前工作

[Issue #8：编写场景脚本](https://github.com/ACienZ/carla-audio/issues/8) 保持 In progress，使用原有关联分支 `7-audio-gen-demo/8-scene-create`。启动时从 `ef3412669` 本地快进同步至集成基线 `935324bdc`，包含 #9、#19 和 #10，准备期间没有推送。[最小场景脚本与说明](tests/scene_create_tests/README.md) 已通过用户审阅和实际运行，助手已回读实际 JSON 核对 201 条测量。用户确认当前最小目标完成，授权补充验收记录、本地 commit、push 和创建目标为 `7-audio-gen-demo/main` 的 PR。关联 PR 从 Issue Development 查看；本轮不合并 PR、不删除分支、不关闭 Issue。

[#10](https://github.com/ACienZ/carla-audio/issues/10) 已通过 [PR #21](https://github.com/ACienZ/carla-audio/pull/21) squash 合入 `7-audio-gen-demo/main`，合并提交为 `935324bdc4ab2e790b6806b084a7e62df3562a94`。Issue 已关闭、Project 为 Done，远端任务分支已删除；本地原提交 `f47b34688` 保留在 `archive/7-audio-gen-demo/10-python-client-binding_backup`，已取消远端跟踪。归档时核对原任务 tip 与 squash 合并提交的文件树一致。

2026-10-19 修正协作流程：先完成修改和检查，用户审阅确认后再本地 commit；模块完成且用户确认交付后才 push、创建 PR。流程文档的本地提交 `a3f0e372e` 已随 #10 交付。此前准备提交 `917dc1739` 曾提前推送，保留该历史，随后调整分支名称并恢复 Issue 关联；不沿用提前推送的流程。

#19 已通过 [PR #20](https://github.com/ACienZ/carla-audio/pull/20) squash 合入集成分支，用户已关闭 Issue，Project 为 Done。原任务分支已从 GitHub 删除，本地保留为 `archive/19-project-foundation`；#9 的旧分支也已在远端删除、本地保留为 `archive/7-audio-gen-demo/9-carla-sensor`。这些既有归档不补改名称，之后按 `archive/<原分支名>_backup` 归档。

## 完整任务清单

表中 Attention 对应看板的 `Details that Need Attention`。标签、优先级和 Size 的含义见 [协作规范](AGENTS.md)。历史估计为本次回顾性整理，不是实际耗时记录。

| Issue | 任务 | Project Status | 标签 | Priority | Size |
| --- | --- | --- | --- | --- | --- |
| [#1](https://github.com/ACienZ/carla-audio/issues/1) | 删除多余分支 | Done | maintenance、removal | P2 | XS |
| [#2](https://github.com/ACienZ/carla-audio/issues/2) | 复现 sensor 示例 | Done | attention、research、testing | P1 | L |
| [#3](https://github.com/ACienZ/carla-audio/issues/3) | 注意 Git 流程 | Attention | attention、documentation、maintenance | P2 | S |
| [#4](https://github.com/ACienZ/carla-audio/issues/4) | 同步标签体系 | Done | maintenance | P2 | XS |
| [#5](https://github.com/ACienZ/carla-audio/issues/5) | 历史 include 排错 | Done | bug、documentation | P2 | M |
| [#6](https://github.com/ACienZ/carla-audio/issues/6) | 主分支重命名 | Done | maintenance | P2 | XS |
| [#7](https://github.com/ACienZ/carla-audio/issues/7) | 音频生成 Demo | In progress | enhancement | P1 | XL |
| [#8](https://github.com/ACienZ/carla-audio/issues/8) | 编写场景脚本 | In progress | enhancement、testing | P1 | M |
| [#9](https://github.com/ACienZ/carla-audio/issues/9) | 编写 CARLA sensor | Done | enhancement、testing | P1 | L |
| [#10](https://github.com/ACienZ/carla-audio/issues/10) | 与 Python 客户端绑定 | Done | enhancement、testing | P1 | M |
| [#11](https://github.com/ACienZ/carla-audio/issues/11) | 利用 Python 合成声音 | TODO | enhancement、testing | P1 | M |
| [#12](https://github.com/ACienZ/carla-audio/issues/12) | 数据集规模 | Attention | attention、research | P2 | S |
| [#14](https://github.com/ACienZ/carla-audio/issues/14) | 如何评价数据质量 | Attention | attention、research | P2 | L |
| [#15](https://github.com/ACienZ/carla-audio/issues/15) | 分析复现的 sensor 示例 | Done | research、documentation | P2 | M |
| [#16](https://github.com/ACienZ/carla-audio/issues/16) | 解决头文件引用报错问题 | Attention | attention、documentation | P2 | M |
| [#17](https://github.com/ACienZ/carla-audio/issues/17) | 每次完成模块后的测试前操作 | Attention | attention、documentation、testing | P2 | S |
| [#19](https://github.com/ACienZ/carla-audio/issues/19) | 整理开发目录与项目协作规范 | Done | documentation、maintenance | P1 | M |

#13 已由用户删除，不重建。#18、#20、#21 是已合并的 PR，不是本表遗漏的 Issue。

## 验收证据与限制

- [PR #18](https://github.com/ACienZ/carla-audio/pull/18) 已合入 `7-audio-gen-demo/main`，合并提交为 `d14da83d3c755162438e8fcba1f62f2e8794b4d2`；它没有合入仓库 `main`。
- #9 的实现包括 Unreal 测距、LibCarla 序列化与事件类型、注册，以及 Python `AudioSourceEvent.distance_m` 绑定，因此 #9 保持 Done。
- [#9 的测试记录](https://github.com/ACienZ/carla-audio/issues/9#issuecomment-6250272390) 包含测试脚本和用户测试成功的说明。用户提供的历史对话末尾还有连续 10 帧静态测试输出：距离为 15 米，帧号一致，打印精度内距离与时间戳误差为零，无等待超时。本轮没有重跑该端到端测试。
- #10 的简化接收脚本：两车静止、15 米间隔、0.05 秒步长，打印 10 次事件；不包含旧脚本的同帧数值对照。用户已审阅、实测并合并 PR，#10 保持原标题和范围，状态为 Done。
- 前期助手检查确认 Conda `carla` 可导入 `AudioSourceEvent`，当时连接服务端超时；此后用户提供了实际运行输出：frame 9387～9396 共 10 次测量，timestamp 164.448～164.898 秒，相邻间隔 0.05 秒，distance_m 均显示 15.000 米，并执行到清理末尾且没有报错。完整输出及边界见 [运行记录](tests/audio_source_tests/README.md#本轮检查)。这是用户实测，不是助手重跑。
- 本次输出同时包含客户端 `ef3412669-dirty`、服务端 `a3f0e372e-dirty` 的版本警告。已核对源码：该警告由版本字符串不相等触发，不直接判定协议兼容性；保留为验收限制，不隐藏警告或仅为版本字符串执行全量重编。
- #10 最小接收已验证；当时的清理后状态回读、再次运行、异常清理、严格同帧数值对照与动态测距没有新增证据，不宣称通过。
- #10 静态检查包括 Python 语法、事件字段导入、CARLA 向量摆放计算、文档链接、Bash 示例语法、diff 空白和生成数据忽略规则。补充注释前后脚本 AST 一致，执行逻辑不变。
- PR #20 的合并提交为 `e85293db97ad0976fc81b66110bd3430e42ae801`。#19 的文档与 Project 字段整理已交付，未运行 CARLA 全量构建或端到端测试。
- 早期曾运行接近场景和 WAV 实验，但使用的是传 Actor ID 的旧 sensor，距离由 Python 查询位置后另算。这些结果不能替代新版接口的验收。#8 当前已开始，#11 仍为 TODO。
- #8 已回读核对：原有关联分支存在，Project 为 In progress，标签 enhancement、testing，Priority P1、Size M 保持不变。准备阶段只在本地快进同步；本次用户确认交付后才执行提交和推送。
- #8 脚本按已确认参数采集：初始 30 米、速度 2 米/秒、步长 0.05 秒、时长 10 秒，含初始位置共 201 条记录，终点 10 米。事件按 tick 帧号核对后保存原始字段；计划距离不替代 sensor 测量。
- #8 用户审阅后将 JSON 精简为仅有 `measurements`，不保留配置元数据；已同步读取示例和设计说明。补充顶部各参数的中文含义、单位及超时说明，并恢复审阅修改中误删的 ego 创建语句，避免引用未定义变量。帧号判断保持原逻辑，明确它检查采样帧而非回调到达时间。
- #8 Python 语法、自定义事件字段导入、本地文档链接、Shell 示例语法、diff 空白及数据忽略规则检查通过。没有运行 CARLA 全量构建。
- #8 使用内存中的客户端/世界/actor 替身和实际 CARLA 几何类型做离线检查：201 条 JSON 回读、10 秒时间跨度、斜向直线位置控制、连续运行两次生成不同文件、逆序清理与设置恢复通过；事件帧号不符、接收超时和第二辆车创建失败均按预期报错、不开始保存数据，并清理已创建对象、恢复设置。检查脚本未加入项目，临时数据已删除。这些检查不模拟真实服务端、网络或 sensor，不能替代动态验收。
- JSON 精简后再次进行离线检查：正常接收和稍后回调到达均保存 201 条测量，JSON 只有 `measurements` 且各条只有三个原始字段；旧帧直接失败、缺失帧等待超时，失败不开始保存，并执行清理、恢复设置。Python 语法、导入、文档链接、Shell 示例和空白复查通过；临时输出已删除，仍不是实际服务端验收。
- #8 前期助手的服务端只读探测在默认沙箱与沙箱外均等待 3 秒超时。此后用户实际运行成功，提供 frame 1658～1858 连续 201 次测量及保存、清理完成输出；这是用户实测，不是助手重跑。客户端版本 `ef3412669-dirty` 与服务端 `935324bdc-dirty` 的警告保留，不把成功接收扩大为全面兼容性结论。
- 助手回读用户实际生成的 `data/20261019_133736_286515.json`：201 条测量、三个原始字段，与终端打印精度内结果一致；首末时间差 10.000000149011612 秒，距离由 30.0 米降至 9.999999046325684 米，与计划距离的最大偏差约 3.81 × 10⁻⁶ 米，小于 0.01 米容差。完整结果见 [运行记录](tests/scene_create_tests/README.md#用户实测与-json-回读2026-10-19)，数据文件保持忽略、不提交。
- 本次没有接收超时或帧号不符，脚本正常保存并执行到清理末尾；清理后 actor/设置独立回读、真实异常清理及再次运行没有新增证据，不宣称通过。用户已确认最小采集目标满足并授权交付；物理驾驶、自动车道控制和音频合成不属于 #8。
- #5 是历史排错记录，已有说明指向 #16 的新流程；保留其 Done 状态与历史，不据此关闭仍作为操作参考的 #16、#17。

## 后续讨论入口

#8 的最小目标已通过用户审阅和运行验证，本轮交付到 PR 创建完成；用户审阅合并后再收尾、讨论 #11 的声音合成。当前 Demo 的 JSON 交接格式与直接失败策略已确认，正式生成器的数据组织、缺失处理及声音模型仍需单独设计。

#8 的旧分支已经按 [开发指南](docs/development.md) 完成本地同步；后续新增提交和远端同步仍遵循先审阅、再本地 commit、模块完成并确认交付后才 push 的顺序。

已记录多模态片段和围绕避让事件采样的初步设计：完整片段约 60 帧，每次输入与输出合计约 20～30 帧，具体采样频率和格式待定。配置驱动、批量场景、无头采集与不同用途的数据导出均为后续方向，未作为 #10 实现。#12 的一万条数据备注、#14 的质量评价继续保留为 Attention；它们不是当前承诺交付。CD 是否采用 GitHub Actions 另行确定。
