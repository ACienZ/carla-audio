# 声源 sensor 静态接收

对应 [Issue #10](https://github.com/ACienZ/carla-audio/issues/10)。[receive_audio_source.py](receive_audio_source.py) 创建静止的 ego 和救护车，接收并打印 10 次帧号、仿真时间戳和距离，方便先熟悉 Python 客户端接口。

## 运行

服务端需要使用本项目包含声源 sensor 的构建。按 [开发指南](../../docs/development.md) 启动 UE 编辑器并点击 Play，确保没有其他客户端推进世界，再运行：

```bash
conda activate carla
cd /home/test/carla-0.9.16
python AudioSimulationDataset/tests/audio_source_tests/receive_audio_source.py
```

脚本顶部可修改连接地址、超时、车辆类型、出生点索引、间距、步长和接收次数。默认使用当前地图的第一个出生点，将救护车沿 ego 朝向向后放置 15 米，关闭两车物理运动。不会切换地图或清理已有车辆；若出生点或救护车位置被占用，创建会直接报错，可调整出生点索引再运行。该摆放方式不保证道路形状或同车道关系，不用于验证动态场景。

正常输出应包含 10 行 `frame=... timestamp=... distance_m=...`，距离应接近 15 米。时间戳使用仿真秒数，距离来自 sensor，帧号不是本地循环次数。本次用户运行记录见下文。

## 阅读顺序与边界

按 `main()` 的顺序阅读：连接 → 保存设置 → 创建车辆 → 创建 sensor → 监听 → 推进世界并取出事件 → 清理。队列把异步到达的事件交给主循环；等待超过 5 秒会抛出 `queue.Empty`，脚本不重试，也不将超时解释为已证实的永久丢帧。

本脚本只验证能否接收并访问三个字段，不做同帧快照对照、严格帧对齐、测距精度或长时间接收完整性验证，不生成数据文件。正常结束、超时和 Ctrl+C 会进入 `finally`，停止 sensor、销毁本次对象、恢复世界设置；服务端断开时清理调用本身仍可能失败，不增加嵌套恢复逻辑。

## 本轮检查

前期助手检查确认 Conda `carla` 可以导入 `carla.AudioSourceEvent`，但当时连接本机服务端超时，未进行实际接收。随后用户在服务器运行脚本，提供了以下输出；这是用户实测证据，不是助手重跑或预期示例：

```text
当前使用的 carla： /home/test/miniconda3/envs/carla/lib/python3.11/site-packages/carla/__init__.py
WARNING: Version mismatch detected: You are trying to connect to a simulator that might be incompatible with this API
WARNING: Client API version     = ef3412669-dirty
WARNING: Simulator API version  = a3f0e372e-dirty
frame=9387  timestamp=164.448s  distance_m=15.000m
frame=9388  timestamp=164.498s  distance_m=15.000m
frame=9389  timestamp=164.548s  distance_m=15.000m
frame=9390  timestamp=164.598s  distance_m=15.000m
frame=9391  timestamp=164.648s  distance_m=15.000m
frame=9392  timestamp=164.698s  distance_m=15.000m
frame=9393  timestamp=164.748s  distance_m=15.000m
frame=9394  timestamp=164.798s  distance_m=15.000m
frame=9395  timestamp=164.848s  distance_m=15.000m
frame=9396  timestamp=164.898s  distance_m=15.000m
已收到 10 次测量。
已清理本次对象并恢复世界设置。
```

结果支持 #10 的最小接收目标：收到 10 次测量，帧号连续，打印精度内相邻时间戳相差 0.05 秒，距离均为 15.000 米，执行到清理末尾且没有报错。此次没有读取清理后的 actor 状态或设置作独立核验，也没有提供再次运行、异常清理、同帧快照对照或动态测距结果，不据此宣称这些验证通过。

### 版本警告的含义

[客户端检查](../../../LibCarla/source/carla/client/detail/Simulator.cpp) 直接比较两端版本字符串，不相等就打印警告。本项目 [构建版本生成函数](../../../Util/BuildTools/Environment.sh) 在当前普通分支使用 Git 短提交号，并在检测到已跟踪文件差异时追加 `-dirty`；[Setup.sh](../../../Util/BuildTools/Setup.sh) 将该值写入生成的版本头文件。

因此，这次警告说明两端报告的构建版本标识不同，不等于已证实接口或协议不兼容。`-dirty` 也不能还原当时完整的源码内容；不能仅凭两个提交号推断安装包和服务端的所有差异，更不能把一次接收成功当作全面兼容验证。本轮保留警告记录，不隐藏警告或为消除字符串差异执行全量重编；后续若修改 sensor/API 或出现通信异常，需核对两端源码、构建产物与 Python 实际导入位置，再按受影响组件构建。

用户已审阅代码、补充注释和验收记录，确认本次运行没有问题，并授权 commit、push 和创建 PR。注释补充前后脚本执行逻辑不变；PR 指向 `7-audio-gen-demo/main`，合并前 #10 保持 In progress。静态检查摘要见 [TODO](../../TODO.md)。
