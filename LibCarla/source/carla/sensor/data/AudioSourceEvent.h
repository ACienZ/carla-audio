#pragma once
// 防止这个头文件在同一个编译单元中被重复包含。

// ============================================================
// AudioSourceEvent.h
//
// 定义客户端收到的声源测量事件。
//
// 数据来源：
//
// 服务端 AudioSourceSensor.cpp
//     计算 DistanceMeters
//         ↓
// AudioSourceSerializer::Serialize()
//     将 float 距离复制成 4 字节载荷
//         ↓
// CARLA 数据流
//     发送通用传感器头 + 自定义距离载荷
//         ↓
// 客户端 AudioSourceSerializer::Deserialize()
//     创建 AudioSourceEvent
//         ↓
// 本类
//     保存元数据并读取距离
//
// 后续完成 Python 绑定后，可以使用：
//
// def callback(event):
//     print(event.frame)
//     print(event.timestamp)
//     print(event.distance_m)
//
// 当前事件表示一个指定目标的一次距离测量。
// 它不是车辆 ID 列表，也不是音频波形。
// ============================================================

// 引入所有 CARLA 传感器数据的共同基类。
//
// SensorData 提供：
// GetFrame()
// GetTimestamp()
// GetSensorTransform()
//
// 后续 Python 通过继承关系读取：
// event.frame
// event.timestamp
// event.transform
#include "carla/sensor/SensorData.h"

// 引入原始传感器数据类型。
//
// RawData 提供两类访问接口：
//
// 通用元数据：
//     GetFrame()
//     GetTimestamp()
//     GetSensorTransform()
//
// 自定义载荷：
//     data()
//     size()
#include "carla/sensor/RawData.h"

// 提供 std::memcpy()，用于读取距离的原始字节。
#include <cstring>

// 提供 std::invalid_argument。
//
// 如果收到的载荷长度不符合约定，拒绝构造事件，
// 避免越界读取或产生错误的距离。
#include <stdexcept>

namespace carla
{
    namespace sensor
    {
        namespace data
        {
            // 具体的传感器数据类型放在：
            // carla::sensor::data 命名空间中。

            // =================================================
            // 声源测量事件
            // =================================================

            // public SensorData：
            // 本类继承 CARLA 的通用传感器数据接口。
            //
            // 本类新增距离读取能力，
            // 帧号、时间戳和传感器 transform 由父类保存。
            class AudioSourceEvent : public SensorData
            {
            public:
                // =============================================
                // 1. 根据收到的 RawData 构造事件
                // =============================================

                // explicit：
                // 禁止编译器把 RawData 隐式转换为本事件类型。
                // 调用方需要明确创建 AudioSourceEvent。
                //
                // RawData &&data：
                // 接收反序列化器传入的原始数据。
                // 与上一轮 std::move(data) 的调用配套。
                //
                // 注意：
                // && 并不意味着本类必须保留整个原始缓冲区。
                // 当前只有一个 float，读取并保存数值即可。
                explicit AudioSourceEvent(RawData &&data)
                    // 构造函数初始化列表。
                    //
                    // SensorData(data) 调用父类构造函数，
                    // 从 RawData 中读取并保存：
                    //
                    // - 服务端采样帧号；
                    // - 仿真时间戳；
                    // - 传感器 transform。
                    //
                    // 父类保存这些元数据，
                    // 不负责解析我们的距离载荷。
                    //
                    // 这里的 data 是有名字的变量表达式，
                    // 可以传给父类的 const RawData& 参数。
                    : SensorData(data)
                {
                    // 与发送端保持一致：
                    // 本协议使用一个 4 字节 float 表示距离。
                    //
                    // 这是编译期检查，不是运行期判断。
                    static_assert(
                        sizeof(float) == 4,
                        "AudioSourceEvent requires a 4-byte float.");

                    // RawData::size() 返回自定义载荷的字节数，
                    // 不包含 CARLA 通用传感器头。
                    //
                    // 当前 Serialize() 只发送一个 float，
                    // 所以这里要求长度恰好等于 sizeof(float)。
                    //
                    // 小于 4 字节：
                    //     无法完整读取距离。
                    //
                    // 大于 4 字节：
                    //     与当前单个 float 的协议不一致。
                    //
                    // 后续增加相对位置等字段时，
                    // 发送端和这里的读取格式必须一起更新。
                    if (data.size() != sizeof(float))
                    {
                        throw std::invalid_argument(
                            "AudioSourceEvent: expected exactly "
                            "4 payload bytes for distance_m.");
                    }

                    // 从原始载荷复制出距离值。
                    //
                    // 三个参数分别是：
                    //
                    // &_distance_m
                    //     目标地址：本事件保存距离的成员变量。
                    //
                    // data.data()
                    //     源地址：自定义载荷的起始位置。
                    //
                    // sizeof(_distance_m)
                    //     复制长度：一个 float 的大小。
                    //
                    // 重要：
                    // RawData::data() 已经跳过通用传感器头，
                    // 不需要我们再手动加一个 header offset。
                    //
                    // 使用 memcpy 读取字节，
                    // 不要求原始载荷地址满足 float 的内存对齐。
                    //
                    // 与发送端一样，这里不做字节序转换；
                    // 两端需要使用一致的 float 表示和字节序。
                    std::memcpy(
                        &_distance_m,
                        data.data(),
                        sizeof(_distance_m));

                    // 到这里：
                    //
                    // SensorData 父类已经保存元数据；
                    // _distance_m 已经保存距离数值。
                    //
                    // 事件不依赖原始缓冲区继续存在，
                    // 因此不需要把 RawData 保存成成员变量。
                }

                // =============================================
                // 2. 提供距离读取接口
                // =============================================

                // 返回本次采样的距离，单位米。
                //
                // 按值返回 float：
                // 读取的是已保存的数值，
                // 不会重新查询服务器或重新计算距离。
                //
                // const：
                // 这个函数不修改当前事件。
                //
                // 后续在 Python 绑定中，将此函数暴露为：
                // event.distance_m
                float GetDistance() const
                {
                    return _distance_m;
                }

            private:
                // =============================================
                // 本次测量的距离
                // =============================================

                // 服务端计算出的：
                // ego Actor 原点到目标 Actor 原点的三维距离。
                //
                // 单位：米。
                //
                // private：
                // 外部代码通过 GetDistance() 读取，
                // 不能直接修改这个成员。
                //
                // 0.0f 只是构造过程中的初始值。
                // 成功构造事件时，它会被载荷中的距离覆盖。
                //
                // 载荷长度错误时构造函数抛出异常，
                // 不会返回一个用 0 米冒充有效测量的事件。
                float _distance_m = 0.0f;
            };

        } // namespace data
    } // namespace sensor
} // namespace carla