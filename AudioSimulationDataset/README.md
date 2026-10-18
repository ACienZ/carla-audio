# AudioSimulationDataset

本目录用于开发基于 CARLA 的音频仿真数据集，属于 [ACienZ/carla-audio](https://github.com/ACienZ/carla-audio) 仓库，不是独立 Git 仓库。

当前先完成一个声音 Demo：采集车静止，救护车从后方接近；保存自定义 sensor 提供的帧号、时间戳和距离，再由 Python 离线读取记录、根据距离调整警笛声音并输出 WAV。当前 sensor 输出测量数据，不直接输出音频。

## 从哪里开始

| 入口 | 内容 |
| --- | --- |
| [协作规范](AGENTS.md) | 任务管理、分支、提交、注释和验证规则 |
| [进度摘要](TODO.md) | 带核对日期的任务状态、估计和验收证据 |
| [目标与设计概览](docs/planning/project-overview.md) | 当前边界、模块职责、数据流和历史方案变化 |
| [开发指南](docs/development.md) | 服务器环境、构建、导入检查和测试方式 |
| [GitHub Project](https://github.com/users/ACienZ/projects/9) | 实际进度的权威来源，需有访问权限 |

后续业务代码、测试和脚本随对应任务在本目录中增加。已有 sensor 源码继续位于仓库的 `Unreal/`、`LibCarla/`、`PythonAPI/`，本轮没有移动或修改它们。

完整单条数据的定义、数据集规模、质量评价和 CD 留待后续讨论；当前不加入实时播放。目录建立不代表声音 Demo 已完成。
