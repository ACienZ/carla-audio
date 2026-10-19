# 项目目标与设计概览

本文件记录截至 2026-10-19 已确认的目标与边界。进度见 [TODO](../../TODO.md) 和 [GitHub Project](https://github.com/users/ACienZ/projects/9)，开发操作见 [开发指南](../development.md)。

## 项目定位与当前目标

项目从 [CARLA](https://github.com/carla-simulator/carla) fork，并以 0.9.16 release 内容作为开发基线。当前实际开发位置是服务器上的同一 Git 仓库；后续自有业务开发集中到 `AudioSimulationDataset/`。

长期目标是生成包含视觉、车辆状态、场景标注和音频的驾驶数据集，用于包含音频理解能力的模型微调、验证与测试。场景覆盖需要让与不需要让两类情况；当前先跑通数据链路，不实施模型训练或完整避让系统。

当前先完成 [#7 音频生成 Demo](https://github.com/ACienZ/carla-audio/issues/7)：

1. 数据采集车 ego 保持静止，救护车从后方同车道匀速靠近。
2. 服务端自定义 sensor 测量 ego 与指定救护车的距离。
3. Python 客户端保存测量所属帧号、仿真时间戳和距离。
4. 独立离线脚本读取本地记录，先用此前正弦波 siren 方案，根据距离调整声源音量并生成 WAV。

实施顺序为 #10 静态接收 → #8 动态场景与记录 → #11 离线声音。#10 的 [最小脚本](../../tests/audio_source_tests/README.md) 只接收并打印，不包含同车道运动、保存数据或合成声音。采集和合成分开运行，音频生成不依赖实时播放。

## 模块职责与数据流

正式生成器的目录和职责统一记录在 [DatasetGenerator 架构](../../DatasetGenerator/README.md)。使用 `ScenarioGenerator`，其内部为 `EnvoronmentController` 和 `VehicleController`；地图、天气和场景事件归前者。`SensorContainer` 管理传感器，`FrameCollector` 组织同帧数据，`DataWriter` 保存并组织标注、音频生成，`DatasetExporter` 面向不同用途采样导出。

下面是已实现 sensor 与后续采集、声音处理之间的数据关系，不表示整条链路已经实现：

```text
场景脚本：创建 ego、救护车和 sensor，控制场景
    ↓ 指定 target_actor_id，attach_to=ego
Unreal AudioSourceSensor：在采样帧计算两车距离
    ↓ float 距离载荷 + CARLA 通用元数据
LibCarla：序列化、传输、反序列化为 AudioSourceEvent
    ↓ sensor.listen(callback)
Python 采集：按 event.frame 识别测量，保存原始记录
    ↓ 本地文件（格式待设计）
Python 离线处理：读取记录、检查数据、合成 WAV
```

| 模块 | 责任边界 |
| --- | --- |
| 场景与会话 | 场景控制器管理环境和车辆；SimulationSession 负责推进仿真；不把客户端计算值伪装为 sensor 测量 |
| CARLA sensor 集成 | 在服务端计算并发送指定目标距离；已有源码保持原目录 |
| Python 采集 | 接收事件、保留帧号与仿真时间、保存原始数据和必要配置 |
| 离线处理与声音合成 | 检查已保存数据，按确认后的模型生成 WAV，保留处理结果与原始测量的区别 |
| 验证与文档 | 说明检查对象、同帧参考值、通过条件和未覆盖范围 |

## 后续数据与场景设计

以下是设计方向，不作为 #10 的实现要求：

- 场景配置覆盖 ego 和紧急车辆类型、出生位置、距离、速度、路线、车道、自动驾驶和天气。可以安排后方同车道或不同车道靠近、ego 静止或移动、紧急车辆从前方路口经过等场景。
- 参数写入配置文件，由脚本组合或采样参数、批量执行场景。格式尚未确定，推荐 YAML，等实现批量生成时结合具体代码选择，不提前引入依赖。
- 片段记录 N 个相机视角（初步以 6 个为基础）、车辆位姿与运动状态、声源距离、事件、标注及音频关联。相机内外参和各模态的帧号、仿真时间戳需要保存。
- 3D Detection、Occupancy、Map Segmentation 的真值来源和表示分别设计；相机语义分割不等于三维占用或 BEV 地图标注。标注依赖的原始信息需在场景运行时采集。
- 音频是连续波形，通过时间范围与帧对齐；替换警笛素材时可独立重跑音频生成。原始记录、派生结果和最终数据集分别放在 raw、derived、exported 中。
- 需要让与不需要让的场景语义、车辆实际执行的动作分别记录，不能直接用“脚本进行了避让”代替正确性标签。

### 围绕避让事件采样

以 ego 在前方行驶、救护车从后方同车道靠近为例：脚本在时刻 t 触发 ego 避让，记录对应帧 ft。目标区间 [fa, fb] 包含要学习的避让过程，输入来自 fa 之前的历史帧，输出标签覆盖目标行为。

每次训练围绕该事件随机采样，而不是将整段数据固定作为一次输入。完整片段初步约 60 帧，一次采样的输入和输出合计约 20～30 帧；采样频率、历史窗口、未来时长及边界规则尚未确定。事件时刻用于采样和监督，不把未来信息直接作为模型输入。训练、验证和测试的划分应避免同一原始片段的重叠窗口跨集合。

### 模型适配参考

后续拟以 [Qwen-Drive-1.0](https://arxiv.org/html/2609.00111) 为基础，增加音频理解模型生成 embedding；接入位置与训练方式尚未设计。论文感知模块使用多视角图像和相机标定；规划模块使用历史轨迹、导航和当前状态，并预测 5 秒、10 Hz 的未来轨迹。因此上述 60 帧和 20～30 帧设想不能直接当成已适配的模型格式，需要结合采样频率再确定。

`DatasetExporter` 保留完整片段到任务样本的转换职责，不将采集格式绑定到单一训练接口。偏好数据还需要候选行为和偏好关系等额外设计。静态测试与声音 Demo 只打通链路，用于学习避让的轨迹需后续生成。

## 当前 sensor 接口

以下是现有代码接口，不是本轮新增设计：

| 接口 | 含义 |
| --- | --- |
| `sensor.other.audio_source` | 服务端 Blueprint ID |
| `target_actor_id` | Python 传入的目标 actor ID，配置为字符串形式的 ID |
| `attach_to=ego` | 将 sensor 绑定至采集车，服务端通过 owner 获取 ego |
| `sensor_tick` | 采样间隔，单位仿真秒；0.0 请求每个仿真更新步采样 |
| `carla.AudioSourceEvent` | Python 接收的事件类型，继承 SensorData，不由 Python 直接构造 |
| `event.frame` | 服务端采样所属帧号，不是回调到达顺序 |
| `event.timestamp` | 该事件的仿真时间戳，不是本机接收时刻 |
| `event.distance_m` | ego 与目标 Actor 原点的三维直线距离，单位米，只读 |
| `event.transform` | sensor 自身的位姿，不是目标救护车的位姿 |

服务端将厘米换算成米，自定义载荷只有一个 4 字节 float；frame、timestamp 等随通用传感器头发送。当前单目标事件不是 Actor ID 数组，也没有旧版 `list(event)`、`len(event)`、`raw_data` 数组接口。

未配置目标、目标失效、owner 无效或距离无效时，当前实现可能跳过发送，不用 0 米表示失败。因此采集方不能假设每个世界帧必有事件；等待超时也不能直接当作永久丢帧。具体等待、收尾核对和缺失处理策略在采集任务中确定。

源码对应位置：

- [Unreal sensor](../../../Unreal/CarlaUE4/Plugins/Carla/Source/Carla/Sensor/AudioSourceSensor.cpp)
- [序列化器](../../../LibCarla/source/carla/sensor/s11n/AudioSourceSerializer.h) 与 [事件类型](../../../LibCarla/source/carla/sensor/data/AudioSourceEvent.h)
- [注册表](../../../LibCarla/source/carla/sensor/SensorRegistry.h) 与 [Python 绑定](../../../PythonAPI/carla/source/libcarla/SensorData.cpp)

## 历史方案变化

| 阶段 | 当时的内容 | 当前如何使用 |
| --- | --- | --- |
| SafeDistance 示例复现 | 检测范围内车辆，传 Actor ID，打通 C++ 到 Python 链路 | 作为开发与注释参考，不是当前测距协议 |
| 早期 audio 实验 | 仍传 Actor ID；Python 查询车辆位置、计算距离、生成声音 | 有场景和 WAV 实验记录，但不算新版 Demo 已验收 |
| AudioSourceSensor | Python 指定单一目标；服务端持续测量两车 Actor 原点距离 | 当前采用的接口，代码已通过 PR #18 合入 Demo 集成分支 |
| 静态验证 | #9 有历史同帧数值检查；#10 用户已运行简化接收脚本，收到 10 次连续帧测量 | 本次输出与版本警告保存在脚本说明中；不把简化接收扩大为严格同帧、动态或全面兼容性验证 |

历史中讨论过检测盒子、多目标记录、多个麦克风、左右声道、传播延迟、多普勒、环境噪声和 UE 音频捕获等方案。它们不因出现在对话中就成为当前实施范围，也不据旧名称覆盖现有 `AudioSourceSensor` 接口。

## 尚待设计

- #10 最小接收已有用户实测证据，补充注释和记录已通过最终审阅，进入 PR 交付阶段；严格同帧距离对照不属于本次最小接收实现。
- #8 的动态场景、时长、运动方式和采样参数，以及动态距离验证。
- 原始记录格式、目录、必要元数据、帧对齐与缺失处理；原始数据与估算或插值结果分开保存。
- #11 的声源 WAV 来源、声学模型、采样率、通道和定量验收方法；当前仅确认距离影响声音，不提前承诺其他声学效果。
- 当前多模态片段与采样设想的具体保存格式、时间窗口、标注和导出方式。
- 是否采用 Actions 实现 CD，等交付物与部署目标明确后决定。本轮不改现有 workflow。

当前 #10 不修改 sensor 公开 API，不迁移 sensor 文件，不增加配置或声音依赖，也不生成数据集。
