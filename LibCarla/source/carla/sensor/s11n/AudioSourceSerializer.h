#pragma once
// 防止这个头文件在同一个编译单元中被重复包含。

// ============================================================
// AudioSourceSerializer.h
//
// 这个文件定义声源传感器的序列化器。
//
// 序列化：
//     服务端的 float 距离值 -> 可以发送的二进制字节。
//
// 反序列化：
//     客户端收到的二进制数据 -> 测量事件对象。
//
// 当前数据格式：
//
// CARLA 通用传感器头：
//     包含 frame、timestamp、sensor transform 等元数据。
//     由 CARLA 的传感器数据流负责生成。
//
// 我们的自定义载荷：
//     [distance_m]
//      一个 float，4 字节
//
// Python 最终读取：
//     event.frame       来自通用传感器头。
//     event.distance_m  来自自定义距离载荷。
//
// 注意：
// 本文件不会计算距离，也不会查找车辆。
// 距离已经在 AudioSourceSensor.cpp 中计算并转换成米。
// ============================================================

// CARLA 的二进制缓冲区类型。
//
// Buffer 管理一段连续的字节内存，
// 后面用它保存 distance_m 的二进制表示。
#include "carla/Buffer.h"

// 提供 CARLA 的 SharedPtr 等智能指针定义。
//
// Deserialize() 将返回：
// SharedPtr<SensorData>
#include "carla/Memory.h"

// 提供 RawData 类型。
//
// RawData 表示 CARLA 客户端接收到的原始传感器数据。
// 后续反序列化时，会把它交给具体的测量事件对象。
#include "carla/sensor/RawData.h"

// 提供 std::memcpy()。
//
// memcpy 用于复制原始字节。
// 它不会把浮点数转换为字符串，也不会改变数值单位。
#include <cstring>

namespace carla
{
    namespace sensor
    {
        // 前向声明 SensorData。
        //
        // SensorData 是 CARLA 各类传感器数据的共同基类。
        //
        // 这里只需要声明一个返回它的智能指针的函数，
        // 暂时不需要访问它的成员，因此不必引入完整定义。
        class SensorData;

        namespace s11n
        {
            // s11n 是 serialization 的缩写，
            // 这个命名空间用于组织序列化器。

            // =================================================
            // 声源传感器的序列化器
            // =================================================
            //
            // 后续需要在 SensorRegistry.h 中建立对应关系：
            //
            // AAudioSourceSensor -> AudioSourceSerializer
            //
            // 这样 CARLA 才知道：
            // AAudioSourceSensor 调用 SerializeAndSend() 时，
            // 应该使用本类的 Serialize() 打包数据。
            class AudioSourceSerializer
            {
            public:
                // =============================================
                // 1. 序列化：距离值 -> 二进制缓冲区
                // =============================================

                // SensorT 是模板类型参数。
                //
                // 调用：
                // Stream.SerializeAndSend(*this, DistanceMeters);
                //
                // 时，CARLA 会将具体传感器类型传入这里，
                // 本例中 SensorT 对应 AAudioSourceSensor。
                //
                // 使用模板可以避免这个 LibCarla 头文件
                // 直接依赖 Unreal 的 AudioSourceSensor.h。
                //
                // 模板函数的定义通常需要对使用它的编译单元可见，
                // 因此这里把 Serialize() 的实现放在 .h 中。
                template <typename SensorT>
                static Buffer Serialize(
                    // 第一个参数是传感器对象。
                    //
                    // 本函数不需要读取传感器成员，
                    // 所以保留参数类型，但省略变量名。
                    //
                    // const：不通过这个引用修改传感器。
                    // &：通过引用传入，不复制传感器对象。
                    const SensorT &,

                    // 第二个参数是已经计算好的距离。
                    //
                    // 单位：米。
                    // 类型：float。
                    //
                    // 按值接收一个小型数值即可，
                    // 不需要传入 ego、目标车辆或 episode。
                    float distance_m)
                {
                    // 当前载荷格式约定 float 占 4 字节。
                    //
                    // static_assert 在编译时检查条件。
                    // 如果构建平台不满足约定，编译会直接报错，
                    // 避免发送端与接收端对数据长度理解不一致。
                    static_assert(
                        sizeof(float) == 4,
                        "AudioSourceSerializer requires a 4-byte float.");

                    // 分配一个刚好能容纳距离值的缓冲区。
                    //
                    // sizeof(float) 在这里是 4。
                    //
                    // 注意：
                    // 这 4 字节只是自定义载荷的长度，
                    // 不是整个网络消息的总长度。
                    // CARLA 的通用传感器头由数据流另外添加。
                    Buffer buffer{sizeof(float)};

                    // 将距离值的二进制表示复制到缓冲区。
                    //
                    // 三个参数分别是：
                    //
                    // buffer.data()
                    //     目标地址：缓冲区起始位置。
                    //
                    // &distance_m
                    //     源地址：当前 float 变量的内存地址。
                    //
                    // sizeof(float)
                    //     复制长度：4 字节。
                    //
                    // 例如 distance_m == 25.0f：
                    // 复制的是 float 25.0 的二进制表示，
                    // 不是字符 '2'、'5'、'.'、'0'。
                    //
                    // 此处沿用原传感器直接复制原生数值的方式，
                    // 不进行字节序转换。
                    // 后续接收端必须按相同的 float 格式读取。
                    std::memcpy(
                        buffer.data(),
                        &distance_m,
                        sizeof(float));

                    // 返回拥有这段字节内存的 Buffer。
                    //
                    // distance_m 是函数局部参数，
                    // 但它的内容已经复制进 buffer。
                    // 因此函数结束后不会引用失效的局部变量。
                    //
                    // 真正的发送由 SerializeAndSend() 继续完成：
                    // 通用传感器头 + 本函数返回的距离载荷。
                    return buffer;
                }

                // =============================================
                // 2. 声明反序列化接口
                // =============================================

                // 在 AudioSourceSerializer.cpp 中实现。
                //
                // 它负责把接收到的 RawData 转换为
                // 我们定义的声源测量事件对象，
                // 并以 SensorData 基类智能指针返回。
                //
                // SharedPtr<SensorData>：
                //     由智能指针管理事件对象的生命周期。
                //     实际对象可以是 SensorData 的派生类。
                //
                // RawData &&data：
                //     && 表示右值引用。
                //     允许后续实现将原始数据移动到事件对象中，
                //     避免不必要的缓冲区复制。
                //
                // 注意：
                // 本处只有函数声明，没有函数体。
                // 具体事件类型、载荷长度检查和距离读取，
                // 将在后续的 .cpp 与事件定义中配套实现。
                static SharedPtr<SensorData> Deserialize(RawData &&data);
            };

        } // namespace s11n
    } // namespace sensor
} // namespace carla