// 定义了 Python callback 最终收到的事件类型：
// def callback(event):
//     print(type(event))
//     print(list(event))

// 这里的 event 就是 SafeDistanceEvent
// 它的作用是：
// 把 serializer 发送过来的 raw bytes
// 解释成一个 ActorId 数组

// 防止头文件重复 include
#pragma once

// 引入 ActorId 类型
// ActorId 本质上是：
// uint32_t
// 也就是 4 字节整数
// safe distance 的 payload 格式是：
// [ActorId][ActorId][ActorId]...
// 所以这里需要知道 ActorId 是什么类型
#include "carla/rpc/ActorId.h"

// 引入 CARLA 的 sensor array 数据类型
// Array<T> 可以把一段 RawData 当作 T 类型数组来看

// 例如：
// Array<rpc::ActorId>
// 表示：
// 这段 raw bytes 是一个 ActorId 数组
// 因此它天然支持：
// size()
// at(pos)
// begin()
// end()

// 后面 Python binding 才能暴露：
// len(event)
// list(event)
// event[0]
#include "carla/sensor/data/Array.h"

// 进入 carla 命名空间
namespace carla
{
    // 进入 carla::sensor 命名空间
    namespace sensor
    {
        // 进入 carla::sensor::data 命名空间
        // CARLA 中具体 sensor data 类型通常放在这里

        // 例如：
        // carla::sensor::data::Image
        // carla::sensor::data::CollisionEvent
        // carla::sensor::data::SafeDistanceEvent
        namespace data
        {
            // 定义 SafeDistanceEvent 类
            // 它继承：
            // Array<rpc::ActorId>
            // 意思是：
            // SafeDistanceEvent 本质上就是一个 ActorId 数组类型的 sensor event
            // 所以如果 raw bytes 是：
            // 19 00 00 00
            // 它会被解释成：
            // ActorId = 25
            // Python 端：
            // list(event)
            // 得到：
            // [25]
            class SafeDistanceEvent : public Array<rpc::ActorId>
            {
            public:
                // 构造函数
                // 参数是 RawData &&data，也就是从 CARLA stream 收到的原始二进制数据
                // explicit 的意思是禁止隐式转换
                // 也就是说，不能让编译器偷偷把 RawData 自动转换成 SafeDistanceEvent，必须显式构造
                // 这是 C++ 里避免意外转换的常见写法
                explicit SafeDistanceEvent(RawData &&data)
                    // : Array<rpc::ActorId>(std::move(data)) {} // 这个写法有问题

                    // 这是构造函数初始化列表
                    // 它调用父类 Array<rpc::ActorId> 的构造函数
                    // 两个参数含义：
                    // 0u
                    // payload offset，表示数据从第 0 个字节开始就是数组内容

                    // std::move(data)
                    // 把 raw bytes 移动给父类 Array 管理，避免复制

                    // 为什么是 0u？
                    // 因为 serializer 发送的 payload 没有 header：
                    // [ActorId][ActorId][ActorId]...
                    // 第 0 个字节开始就是第一个 actor id
                    // 如果 serializer 发送的是：
                    // [count][ActorId][ActorId]...
                    // 那这里就不能是 0u，而应该跳过 count，例如：
                    // Array<rpc::ActorId>(sizeof(uint32_t), std::move(data))
                    // 但现在没有 count，所以必须是：
                    // 0u
                    : Array<rpc::ActorId>(0u, std::move(data))
                // 构造函数体为空，因为真正的初始化已经在初始化列表里完成了
                {
                }
            };

        } // namespace data
    } // namespace sensor
} // namespace carla