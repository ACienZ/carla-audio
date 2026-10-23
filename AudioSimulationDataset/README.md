# AudioSimulationDataset

本目录用于开发基于 CARLA 的音频仿真数据集，属于 [ACienZ/carla-audio](https://github.com/ACienZ/carla-audio) 仓库，不是独立 Git 仓库。

长期目标是通过配置控制 CARLA 场景，采集图像、车辆状态、声源距离等数据，再生成标注与声音，导出用于微调、验证和测试的数据集。正式生成器放在 `DatasetGenerator/`；测试、辅助工具和文档分别放在 `tests/`、`tools/`、`docs/`，未使用的目录暂不创建。

当前先完成一个声音 Demo：采集车静止，救护车从后方接近；保存自定义 sensor 提供的帧号、时间戳和距离，再由 Python 离线生成含距离衰减、传播时间与多普勒的连续警笛 WAV，以及帧音频区间索引。当前 sensor 输出测量数据，不直接输出音频。

## 从哪里开始

| 入口 | 内容 |
| --- | --- |
| [协作规范](AGENTS.md) | 任务管理、分支、提交、注释和验证规则 |
| [进度摘要](TODO.md) | 带核对日期的任务状态、估计和验收证据 |
| [目标与设计概览](docs/planning/project-overview.md) | 当前边界、模块职责、数据流和历史方案变化 |
| [DatasetGenerator 架构](DatasetGenerator/README.md) | 规划目录、模块职责、配置驱动和数据生成流程 |
| [#10 静态接收](tests/audio_source_tests/README.md) | 独立脚本、参数说明、运行方式与验证限制 |
| [#8 匀速接近与采集](tests/scene_create_tests/README.md) | 最小动态场景、JSON 原始记录及 #11 输入 |
| [#11 离线警笛合成](tests/audio_render_tests/README.md) | 连续 WAV、传播模型、帧区间索引及数值验证 |
| [#11 分步理解](tests/audio_render_tests/step_by_step/README.md) | 先阅读：纯正弦波 → 距离衰减 → 运动传播 → 两者结合 |
| [开发指南](docs/development.md) | 服务器环境、构建、导入检查和测试方式 |
| [GitHub Project](https://github.com/users/ACienZ/projects/9) | 实际进度的权威来源，需有访问权限 |

后续业务代码、测试和脚本随对应任务在本目录中增加。已有 sensor 源码继续位于仓库的 `Unreal/`、`LibCarla/`、`PythonAPI/`，本轮没有移动或修改它们。

#10 的静态接收已通过 PR #21 合入集成分支。#8 的最小动态采集经用户实测、JSON 回读后通过 PR #22 合入，Issue 已关闭、Project 为 Done，本地任务分支已归档。#11 当前为 In progress，已用这次实测记录生成 10 秒 WAV 和 200 个帧区间，通过离线数值检查，完整模块审阅和试听继续进行；2026-10-23 用户授权将当前开发进度提交并推送到 #11 任务分支，本次不创建 PR。后续正式数据集也先生成完整连续波形，再取所需时间片段。多模态格式、数据集规模、质量评价和 CD 留待后续讨论；当前不加入实时播放。
