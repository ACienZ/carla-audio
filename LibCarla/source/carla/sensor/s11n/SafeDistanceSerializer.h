// 核心作用:
// 把 Unreal 侧检测到的 AActor* 集合
// 转换成一段二进制数据 Buffer
// 再让 Python 端反序列化成 SafeDistanceEvent

// 也就是说，它连接的是：
// UE sensor 的 PostPhysTick()
//         ↓
// Stream.SerializeAndSend(...)
//         ↓
// SafeDistanceSerializer::Serialize(...)
//         ↓
// 网络传输 raw bytes
//         ↓
// SafeDistanceSerializer::Deserialize(...)
//         ↓
// Python 收到 SafeDistanceEvent

// 防止这个头文件被重复 include
#pragma once

// 引入 CARLA 的智能指针类型，比如: SharedPtr<SensorData>
// CARLA 代码里很多地方不用标准库裸写 std::shared_ptr，而是用自己的封装/别名
#include "carla/Memory.h"

// 引入 CARLA actor id 类型
// 里面实际定义了
// namespace carla {
// namespace rpc {
//   using ActorId = uint32_t;
// }

// using ActorId = rpc::ActorId;
// }
// 所以这里的 ActorId 本质上是一个 uint32_t，也就是 4 字节无符号整数
// Python 端收到的 [25]，底层就是一个 ActorId
#include "carla/rpc/ActorId.h"

// 引入 sensor 原始数据类型
// RawData 表示从 server 传到 client 的原始 bytes
// 后面的 Deserialize(RawData &&data) 会用到它
#include "carla/sensor/RawData.h"

// 引入固定宽度整数类型，例如：unit32_t
#include <cstdint>
// 引入 C 风格内存操作函数
// 这里用到了：std::memcpy(...)
// 它负责把 ids 里的 actor id 拷贝进连续的 byte buffer
#include <cstring>
// 引入 std::vector
// 这里先用 vector 暂存所有检测到的 actor id：std::vector<ActorId> ids;
#include <vector>

// 进入 carla 命名空间
// CARLA 的 LibCarla 代码都组织在这个命名空间下，避免和其他库重名
namespace carla
{
    // 进入 carla::sensor 命名空间
    // 这里放 sensor 相关类型
    namespace sensor
    {

        // 前向声明 SensorData
        // 这里不需要知道 SensorData 的完整定义，只需要声明“有这么一个类”
        // 后面这个函数会返回它的智能指针：static SharedPtr<SensorData> Deserialize(RawData &&data);
        class SensorData;

        // 进入 carla::sensor::s11n 命名空间
        // s11n 是 serialization 的缩写，意思是序列化/反序列化
        // 这个目录里的类通常负责：
        // 1. 把 Unreal server 侧的 sensor 数据序列化成 bytes
        // 2. 把 bytes 反序列化成 Python client 侧的 sensor
        namespace s11n
        {

            // 声明 SafeDistanceSerializer 类
            // 它是 sensor.other.safe_distance 的序列化器
            // 在 SensorRegistry.h 里会注册: std::pair<ASafeDistanceSensor *, s11n::SafeDistanceSerializer>
            // 意思是：
            // 当 ASafeDistanceSensor 调用 SerializeAndSend 时
            // 使用 SafeDistanceSerializer 来打包数据

            // 其中有两个核心函数：
            // 1. 模板函数 Serialize(...): 把 Unreal 侧检测到的 AActor* 集合，转换成一段二进制数据 Buffer
            // 2. Deserialize(...): 在 SafeDistanceSerializer.cpp 里实现
            class SafeDistanceSerializer
            {
            public:
                // 声明一个模板函数
                // 这里不用固定写死参数类型，而是让编译器根据调用时传入的对象推导:
                // SensorT    = 具体 sensor 类型，比如 ASafeDistanceSensor
                // EpisodeT   = episode 类型
                // ActorListT = detected_actors 的容器类型，比如 TSet<AActor *>
                // 这样 serializer 可以适配 CARLA 内部不同侧的类型
                template <typename SensorT, typename EpisodeT, typename ActorListT>

                // 声明静态函数 Serialize，返回 Buffer
                // Buffer 是 CARLA 用来承载即将发送的二进制数据的类型
                // 简单理解: Buffer = 一段 bytes
                // 这个函数的任务就是：把 detected_actors 转换成 Buffer
                static Buffer Serialize(
                    // 第一个参数是 sensor 本身
                    // 这里没有变量名，说明当前函数不使用这个参数
                    // 它依然存在，是因为 CARLA serializer 的接口约定通常会传 sensor 进来
                    const SensorT &,
                    // 第二个参数是当前 episode
                    // 它很重要，因为我们需要通过 episode 把 Unreal 的 AActor* 转成 CARLA actor id
                    // 也就是后面这行：
                    // episode.FindCarlaActor(actor)
                    const EpisodeT &episode,
                    // 第三个参数是检测到的 actor 集合
                    // 在 SafeDistanceSensor.cpp 里传进来的就是：
                    // DetectedActors
                    // 类型大致是：
                    // TSet<AActor *>
                    const ActorListT &detected_actors)
                {
                    // 原本想做的事情是：
                    // 1. 根据检测到的 actor 数量创建 buffer
                    // 2. 遍历每个 actor
                    // 3. 找到 actor id
                    // 4. 把 actor id 一个一个写进 buffer
                    // 但这里有一个 CARLA 0.9.16 的问题：
                    // episode.FindActor(actor)
                    // 这个接口在你们当前版本里不适用
                    // 应该使用：
                    // episode.FindCarlaActor(actor)
                    // 所以这段被注释掉了

                    // const uint32_t size_in_bytes = sizeof(ActorId) * detected_actors.Num();
                    // Buffer buffer{size_in_bytes};
                    // unsigned char *it = buffer.data();
                    // for (auto *actor : detected_actors)
                    // {
                    //     ActorId id = episode.FindActor(actor).GetActorId();
                    //     std::memcpy(it, &id, sizeof(ActorId));
                    //     it += sizeof(ActorId);
                    // }
                    // return buffer;

                    // 现在的做法是：
                    // 创建一个 vector，用来暂存检测到的 actor id
                    // 例如检测到 actor id 25 和 31，那么：
                    // ids = [25, 31]
                    std::vector<ActorId> ids;
                    // 提前为 vector 预留空间
                    // detected_actors.Num() 是 Unreal 容器的数量接口，表示检测到多少个 actor
                    // reserve 不会改变 vector 的长度，只是提前分配内存，避免后面 push_back 时反复扩容
                    ids.reserve(detected_actors.Num());

                    // 遍历所有检测到的 actor
                    // auto *actor 表示每个元素是一个指针，实际类型大致是：AActor *
                    for (auto *actor : detected_actors)
                    {
                        // 把 Unreal actor 指针转换成 CARLA actor 对象

                        // 这里要理解两个层次：
                        // AActor*         = Unreal 世界里的 actor 指针
                        // CarlaActor      = CARLA 自己管理的 actor 包装对象
                        // Carla actor id  = Python API 里看到的 actor.id

                        // episode.FindCarlaActor(actor) 的意思是：
                        // 给我一个 Unreal AActor*
                        // 查一下它在 CARLA episode 里对应哪个 CarlaActor

                        // 这里用了 if 是因为不一定所有 Unreal actor 都有对应的 CARLA actor
                        // 如果找不到，返回空指针，就不会进入 if
                        if (auto *carla_actor = episode.FindCarlaActor(actor))
                        {
                            // 如果找到了对应的 CARLA actor，就取出它的 actor id，加入 ids
                            // 例如：
                            // carla_actor->GetActorId() = 25
                            // ids.push_back(25)
                            ids.push_back(carla_actor->GetActorId());
                        }
                    } // 遍历结束, 此时 ids 里保存的是所有检测到车辆的 CARLA actor id

                    // 它的问题是：如果 ids 为空，ids.data() 在某些情况下不适合直接拿去 memcpy，即使拷贝长度是 0，也不够稳妥
                    // 所以后来改成了下面这个版本
                    // Buffer buffer{sizeof(ActorId) * ids.size()};
                    // std::memcpy(buffer.data(), ids.data(), sizeof(ActorId) * ids.size());
                    // return buffer;

                    // 创建一个 buffer
                    // 大小是：每个 ActorId 的字节数 × actor id 数量
                    // 因为 ActorId 是 uint32_t，所以每个 id 是 4 字节
                    // 如果： ids = [25], 那么 buffer 大小是： 4 bytes
                    // 如果：ids = [25, 31], 那么 buffer 大小是： 8 bytes
                    // 这个 payload 没有额外 header，也没有 count
                    // 它的格式就是： [ActorId][ActorId][ActorId]...
                    Buffer buffer{sizeof(ActorId) * ids.size()};
                    // 如果 ids 不为空，才执行内存拷贝
                    // 这是为了避免空 vector 时不必要的 memcpy
                    if (!ids.empty())
                    {
                        // 把 vector 里的 actor id 原样拷贝到 buffer
                        // 这是一段二进制拷贝，不是字符串转换
                        // 例如 actor id 是 25，ActorId 是 4 字节小端序，那么 raw bytes 可能是：
                        // 19 00 00 00
                        // 所以 Python 端看到：
                        // raw_data hex: 19000000
                        std::memcpy(buffer.data(), ids.data(), sizeof(ActorId) * ids.size());
                    }
                    // 返回序列化后的二进制数据
                    // 这段数据会通过 CARLA 的 sensor stream 发给 Python client
                    return buffer;
                }

                // 声明反序列化函数
                // 这个函数在 .cpp 文件里实现
                // 它的作用是：
                // 把收到的 RawData 包装成 SafeDistanceEvent

                // 注意这里用的是：
                // RawData &&data
                // 这是右值引用，表示可以把这段 raw data 移动进去，避免复制
                // 最终 .cpp 里会类似：
                // return SharedPtr<SensorData>(new data::SafeDistanceEvent(std::move(data)));
                // 也就是说，反序列化不一定真的解析每一个 id，而是把 raw bytes 交给 SafeDistanceEvent，后者继承 Array<ActorId>，所以 Python 端可以 list(event)
                static SharedPtr<SensorData> Deserialize(RawData &&data);
            };

        } // namespace s11n
    } // namespace sensor
} // namespace carla