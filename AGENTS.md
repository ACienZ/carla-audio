# carla-audio 协作入口

开始本仓库的工作前，先阅读 [AudioSimulationDataset/AGENTS.md](AudioSimulationDataset/AGENTS.md)，再核对其中链接的 TODO、GitHub Project、Issue 和当前分支。

后续业务代码、测试、脚本和文档集中在 `AudioSimulationDataset/`；现有 sensor 集成源码保留在 CARLA 原目录。当前 Demo 子任务分支使用 `7-audio-gen-demo/<issue编号>-<任务描述>`，不加 `codex/` 前缀。

先修改并检查，交用户审阅确认后才在服务器本地 commit。模块开发完成且用户确认交付后，才 push 并创建 PR；项目管理授权不替代用户审阅。
