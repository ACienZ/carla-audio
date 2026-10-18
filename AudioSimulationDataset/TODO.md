# 任务与进度摘要

核对日期：2026-10-19（Asia/Tokyo）。实际进度以 [carla-audio tracker](https://github.com/users/ACienZ/projects/9) 为准，后续开始任务时重新读取；本表不是实时看板。

## 当前工作

[Issue #10：与 Python 客户端绑定](https://github.com/ACienZ/carla-audio/issues/10) 已进入 In progress，使用关联分支 `7-audio-gen-demo/10-python-client-binding`。分支从最新 `7-audio-gen-demo/main` 的 `e85293db9` 创建，本地工作区已切换至该分支。本次先完成分支准备和协作流程文档更新；客户端测试脚本及运行验证尚未交付。

2026-10-19 修正协作流程：先完成修改和检查，用户审阅确认后再本地 commit；模块完成且用户确认交付后才 push、创建 PR。本次文档修正保留为未提交修改，等待用户审阅，再进入 #10 的正式开发。此前准备提交 `917dc1739` 已提前推送，本次保留该历史，远端仅调整分支名称并恢复 Issue 关联，没有上传新的提交。

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
| [#8](https://github.com/ACienZ/carla-audio/issues/8) | 编写场景脚本 | TODO | enhancement、testing | P1 | M |
| [#9](https://github.com/ACienZ/carla-audio/issues/9) | 编写 CARLA sensor | Done | enhancement、testing | P1 | L |
| [#10](https://github.com/ACienZ/carla-audio/issues/10) | 与 Python 客户端绑定 | In progress | enhancement、testing | P1 | M |
| [#11](https://github.com/ACienZ/carla-audio/issues/11) | 利用 Python 合成声音 | TODO | enhancement、testing | P1 | M |
| [#12](https://github.com/ACienZ/carla-audio/issues/12) | 数据集规模 | Attention | attention、research | P2 | S |
| [#14](https://github.com/ACienZ/carla-audio/issues/14) | 如何评价数据质量 | Attention | attention、research | P2 | L |
| [#15](https://github.com/ACienZ/carla-audio/issues/15) | 分析复现的 sensor 示例 | Done | research、documentation | P2 | M |
| [#16](https://github.com/ACienZ/carla-audio/issues/16) | 解决头文件引用报错问题 | Attention | attention、documentation | P2 | M |
| [#17](https://github.com/ACienZ/carla-audio/issues/17) | 每次完成模块后的测试前操作 | Attention | attention、documentation、testing | P2 | S |
| [#19](https://github.com/ACienZ/carla-audio/issues/19) | 整理开发目录与项目协作规范 | Done | documentation、maintenance | P1 | M |

#13 已由用户删除，不重建。#18、#20 是已合并的 PR，不是本表遗漏的 Issue。

## 验收证据与限制

- [PR #18](https://github.com/ACienZ/carla-audio/pull/18) 已合入 `7-audio-gen-demo/main`，合并提交为 `d14da83d3c755162438e8fcba1f62f2e8794b4d2`；它没有合入仓库 `main`。
- #9 的实现包括 Unreal 测距、LibCarla 序列化与事件类型、注册，以及 Python `AudioSourceEvent.distance_m` 绑定，因此 #9 保持 Done。
- [#9 的测试记录](https://github.com/ACienZ/carla-audio/issues/9#issuecomment-6250272390) 包含测试脚本和用户测试成功的说明。用户提供的历史对话末尾还有连续 10 帧静态测试输出：距离为 15 米，帧号一致，打印精度内距离与时间戳误差为零，无等待超时。本轮没有重跑该端到端测试。
- 测试脚本尚未纳入当前仓库，历史记录不能代替可复现的正式交付。#10 保持原标题和范围；2026-10-19 按用户要求由 TODO 转为 In progress，仍由该任务完成客户端验证交付。
- PR #20 的合并提交为 `e85293db97ad0976fc81b66110bd3430e42ae801`。#19 的文档与 Project 字段整理已交付，未运行 CARLA 全量构建或端到端测试。
- 早期曾运行接近场景和 WAV 实验，但使用的是传 Actor ID 的旧 sensor，距离由 Python 查询位置后另算。这些结果不能替代新版接口的验收，#8、#11 保持 TODO。
- #5 是历史排错记录，已有说明指向 #16 的新流程；保留其 Done 状态与历史，不据此关闭仍作为操作参考的 #16、#17。

## 后续讨论入口

先推进 #10，再围绕 #8、#11 完成当前声音 Demo；具体实现与验收标准在实施前确认。距离与帧记录的落盘格式、异常或缺失数据处理、声源素材和声音模型仍需设计。

#8 的既有分支 `7-audio-gen-demo/8-scene-create` 当前仍停在 `ef3412669`，缺少 #9 sensor 和 #19 文档，也不会自动获得 #10 的后续提交。开始 #8 时按 [开发指南](docs/development.md) 检查并同步届时最新的 `7-audio-gen-demo/main`；需要 #10 的成果时先确认其 PR 已合入集成分支，再同步。本次不更新 #8 分支，其状态保持 TODO。

Demo 完成后再讨论完整单条数据的定义。#12 的一万条数据备注、#14 的质量评价继续保留为 Attention；它们不是当前承诺交付。CD 是否采用 GitHub Actions 另行确定。
