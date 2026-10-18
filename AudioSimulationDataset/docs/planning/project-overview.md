# 项目目标与设计概览

本文件记录截至 2026-10-19 已确认的目标与边界。进度见 [TODO](../../TODO.md) 和 [GitHub Project](https://github.com/users/ACienZ/projects/9)，开发操作见 [开发指南](../development.md)。

## 项目定位与当前目标

项目从 [CARLA](https://github.com/carla-simulator/carla) fork，并以 0.9.16 release 内容作为开发基线。当前实际开发位置是服务器上的同一 Git 仓库；后续自有业务开发集中到 `AudioSimulationDataset/`。

长期方向是生成用于自动驾驶音频研究的数据集。历史讨论涉及救护车声音分离、方位判断和避让系统，这些作为背景保留，不作为本轮实现要求。

当前先完成 [#7 音频生成 Demo](https://github.com/ACienZ/carla-audio/issues/7)：

1. 数据采集车 ego 保持静止，救护车从后方靠近。
2. 服务端自定义 sensor 测量 ego 与指定救护车的距离。
3. Python 客户端保存测量所属帧号、仿真时间戳和距离。
4. 独立离线脚本读取本地记录，根据距离调整声源音量并生成 WAV。

采集和合成分开运行，音频生成不依赖实时播放。完整单条数据的组成与标注、批量规模、数据质量评价、神经网络和避让系统均在后续阶段讨论。

## 模块职责与数据流

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
| 场景脚本 | 场景配置、actor 生命周期、推进仿真；不把客户端计算值伪装为 sensor 测量 |
| CARLA sensor 集成 | 在服务端计算并发送指定目标距离；已有源码保持原目录 |
| Python 采集 | 接收事件、保留帧号与仿真时间、保存原始数据和必要配置 |
| 离线处理与声音合成 | 检查已保存数据，按确认后的模型生成 WAV，保留处理结果与原始测量的区别 |
| 验证与文档 | 说明检查对象、同帧参考值、通过条件和未覆盖范围 |

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
| 静态验证 | 历史输出显示 10 帧同帧距离与时间戳检查通过 | 测试脚本尚未提交，#10 继续保持 TODO |

历史中讨论过检测盒子、多目标记录、多个麦克风、左右声道、传播延迟、多普勒、环境噪声和 UE 音频捕获等方案。它们不因出现在对话中就成为当前实施范围，也不据旧名称覆盖现有 `AudioSourceSensor` 接口。

## 尚待设计

- #10 的正式测试脚本及运行说明，如何验证 Python 收到的新事件和同帧距离。
- #8 的动态场景、时长、运动方式和采样参数，以及动态距离验证。
- 原始记录格式、目录、必要元数据、帧对齐与缺失处理；原始数据与估算或插值结果分开保存。
- #11 的声源 WAV 来源、声学模型、采样率、通道和定量验收方法；当前仅确认距离影响声音，不提前承诺其他声学效果。
- 当前 Demo 完成后，再定义一条完整数据及其打包与标注方式。
- 是否采用 Actions 实现 CD，等交付物与部署目标明确后决定。本轮不改现有 workflow。

当前文档整理任务不新增或修改公开 API，不迁移 sensor 文件，不增加声音依赖，也不生成数据集。
