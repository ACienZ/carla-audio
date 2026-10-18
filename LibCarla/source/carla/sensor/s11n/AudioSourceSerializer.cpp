// ============================================================
// AudioSourceSerializer.cpp
//
// 对应头文件：AudioSourceSerializer.h
//
// 服务端序列化：
//     float distance_m -> 4 字节距离载荷。
//     已在 AudioSourceSerializer.h 中实现。
//
// 客户端反序列化：
//     RawData -> AudioSourceEvent。
//     在本文件中实现。
//
// Python 最终读取：
//     event.frame
//     event.distance_m
//
// 职责划分：
// - 本文件：创建具体的事件对象。
// - AudioSourceEvent：检查并读取距离载荷，提供距离访问接口。
// - SensorData 基类：提供帧号、时间戳等通用元数据。
// - Python 绑定：让 Python 能访问这些 C++ 接口。
// ============================================================

// 引入序列化器声明。
//
// AudioSourceSerializer.h 中已经声明了：
//
// static SharedPtr<SensorData> Deserialize(RawData &&data);
//
// 本文件为这个函数提供具体实现。
#include "carla/sensor/s11n/AudioSourceSerializer.h"

// 引入具体的事件类型。
//
// 后面需要执行：
// new data::AudioSourceEvent(...)
//
// 创建对象时，编译器必须知道这个类的完整定义，
// 因此这里不能仅使用前向声明。
//
// 注意：
// 这是下一步需要新增的头文件。
// 旧的 AudioEvent.h 仍然按 Actor ID 数组解释载荷，
// 不能直接用于读取当前的 float 距离。
#include "carla/sensor/data/AudioSourceEvent.h"

// 提供 std::move()。
#include <utility>

namespace carla
{
    namespace sensor
    {
        namespace s11n
        {
            // =================================================
            // 实现客户端反序列化入口
            // =================================================

            // AudioSourceSerializer::Deserialize：
            // 表示实现 AudioSourceSerializer 类中声明的函数。
            //
            // 头文件中已经写过 static，
            // 在类外定义函数时不再重复写 static。
            //
            // 返回值 SharedPtr<SensorData>：
            //
            // CARLA 使用 SensorData 作为不同传感器数据的
            // 共同基类，便于通过统一接口处理事件。
            //
            // 返回的实际对象仍然是 AudioSourceEvent，
            // 不会因为使用基类指针而丢失派生类对象。
            //
            // 参数 RawData &&data：
            //
            // data 是接收到的原始传感器数据。
            // 通过 RawData 可以访问通用元数据和自定义载荷。
            //
            // && 是右值引用，允许事件对象接管这份数据，
            // 避免不必要的缓冲区复制。
            SharedPtr<SensorData> AudioSourceSerializer::Deserialize(
                RawData &&data)
            {
                // 创建 AudioSourceEvent，并交给智能指针管理。
                //
                // 分开理解下面这一行：
                //
                // 1. std::move(data)
                //
                //    将 data 转换成可用于移动的表达式。
                //    std::move 本身不搬运字节；
                //    实际如何接管数据由构造函数及其调用决定。
                //
                //    从这里交出数据后，本函数不再读取它。
                //
                // 2. new data::AudioSourceEvent(...)
                //
                //    调用 AudioSourceEvent 的构造函数。
                //
                //    data:: 是命名空间：
                //    carla::sensor::data
                //
                //    括号里的 data 则是本函数的参数。
                //    两者名字相同，但含义不同。
                //
                //    下一步的事件构造函数需要：
                //    - 将通用元数据交给 SensorData 基类；
                //    - 检查距离载荷是否恰好为 sizeof(float)；
                //    - 按照 Serialize() 的格式读取距离。
                //
                // 3. SharedPtr<SensorData>(...)
                //
                //    用 CARLA 的智能指针管理新对象，
                //    并通过 SensorData 基类指针返回。
                //
                //    这里直接交给智能指针接管，
                //    调用方不需要手动 delete。
                return SharedPtr<SensorData>(
                    new data::AudioSourceEvent(std::move(data)));
            }

        } // namespace s11n
    } // namespace sensor
} // namespace carla