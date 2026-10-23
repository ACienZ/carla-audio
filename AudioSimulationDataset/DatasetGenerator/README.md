# DatasetGenerator

本目录是后续数据生成主体。当前只记录架构，尚未实现正式生成器；独立验证包括 [#10 接收](../tests/audio_source_tests/README.md)、[#8 采集](../tests/scene_create_tests/README.md) 和 [#11 合成](../tests/audio_render_tests/README.md)。项目目标和阶段安排见 [设计概览](../docs/planning/project-overview.md)。

## 规划目录

以下是目标结构，不表示目录和功能已经全部存在。随对应任务增加实现，不提前创建空类或占位方法。`EnvoronmentController` 按用户确认的拼写保留。

```text
AudioSimulationDataset/
├── DatasetGenerator/
│   ├── configs/
│   ├── SimulationSession/
│   ├── ScenarioGenerator/
│   │   ├── EnvoronmentController/
│   │   └── VehicleController/
│   ├── SensorContainer/
│   ├── FrameCollector/
│   ├── DataWriter/
│   │   ├── AnnotationBuilder/
│   │   └── AudioRenderer/
│   ├── DatasetExporter/
│   ├── scripts/
│   └── dataset/
│       ├── raw/
│       ├── derived/
│       └── exported/
├── tests/
│   ├── audio_source_tests/
│   ├── scene_create_tests/
│   └── audio_render_tests/
├── tools/
└── docs/
```

现有 README、AGENTS、TODO 继续放在 `AudioSimulationDataset/` 下。

## 各模块做什么

| 模块 | 职责 |
| --- | --- |
| configs | 场景、车辆、传感器、采样、输出等参数；格式待定，推荐 YAML，实施时结合批量配置生成代码讨论 |
| SimulationSession | 连接 CARLA，设置同步模式，推进仿真时间，恢复会话设置 |
| ScenarioGenerator | 根据配置组织一次场景执行，不要求生成 Python 源码 |
| EnvoronmentController | 地图、天气、场景事件与触发条件；记录事件实际发生的帧号和时间戳 |
| VehicleController | 车辆创建、起点和运动控制；后续扩展静止、匀速、自动驾驶、路线、车道和避让 |
| SensorContainer | 持有 AudioSourceSensor、CameraSensor 等对象，统一创建、监听、停止和销毁 |
| FrameCollector | 将传感器测量、世界快照中的车辆状态和事件按帧组织；不把回调到达顺序当作采样顺序 |
| DataWriter | 保存原始片段和配置，并组织派生数据的生成与保存 |
| AnnotationBuilder | 根据采集的场景真值生成标注；后续分别设计 3D Detection、Occupancy、Map Segmentation |
| AudioRenderer | 读取记录和声源，先生成完整连续波形及时间索引，允许替换警笛后单独重跑 |
| DatasetExporter | 从完整片段采样并导出，用于监督微调、偏好微调、验证、测试等；具体任务格式另行设计 |
| scripts | 单次运行、批量配置生成、批量执行、后处理和导出的短入口 |
| tests / tools / docs | 分别用于验证、辅助检查与可视化、说明及设计记录 |

地图和天气由场景决定。切换地图后，由入口协调重新取得 world 并设置同步参数；一次运行只有一个地方负责 tick，控制器和传感器回调不自行推进世界。

FrameCollector 负责收集，DataWriter 负责保存，完整采集链路才是原始数据生成过程。DataWriter 内部的原始保存、标注生成、音频合成分别调用，避免更换声音素材时重新运行 CARLA。具体类的方法和文件格式在对应实现时确定。

## 从一次采集到数据集

```text
场景模板 / 参数范围
    → 批量生成具体配置
    → SimulationSession + ScenarioGenerator 执行一次场景
    → SensorContainer + FrameCollector 收集
    → DataWriter 保存 raw
    → AnnotationBuilder / AudioRenderer 生成 derived
    → DatasetExporter 采样、组织并写入 exported
```

单次配置描述地图、天气、车辆类型、出生位置、距离、速度、传感器和事件条件。先能手动运行一份配置，再由批量脚本组合或采样参数；批量调度与场景内的事件触发分开。每个片段保存实际配置、随机种子、运行结果和事件记录，便于追查；相同种子本身不保证仿真结果完全一致。

`dataset/raw/` 保存原始测量、状态和配置，`derived/` 保存标注、WAV 等后处理结果，`exported/` 保存按用途组织的最终样本。派生结果可重建，不覆盖原始记录。后续输出根目录可配置，默认生成内容已由本目录的 `.gitignore` 排除。

音频处理方式已确认：先生成完整连续波形，再按接收时间范围切片，不逐帧重新开始警笛。保存完整 WAV 和轻量的帧区间索引，DatasetExporter 可按历史窗口或事件窗口取样，不必逐帧保存小 WAV。相邻测量点围出左闭右开的音频区间，最后一点只提供终点；所有边界从同一时间原点换算为采样索引。面向当前时刻的模型输入只能取截至该时刻的声音。

#11 的独立验证先使用双音正弦波、距离衰减和传播时间映射，多普勒由传播时间变化产生。其静止麦克风和录制前轨迹外推假设不直接推广到所有场景；正式生成器应记录运动和发声状态，为更完整的传播模型提供输入。

后续无头采集需要保留相机渲染，采用 off-screen rendering；`no_rendering_mode` 会让相机返回空数据。服务端必须包含本项目的自定义 sensor。打包、无头启动和批量调度均尚未在本项目验收，当前仍按编辑器 Play 的方式运行。参考 [CARLA 0.9.16 渲染说明](https://carla.readthedocs.io/en/0.9.16/adv_rendering_options/)。
