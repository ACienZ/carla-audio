// 只实现了一件事：
// 把收到的 RawData 包装成 SafeDistanceEvent
// 也就是 serializer 的反序列化部分

// 引入自己的头文件
// 因为这里要实现：
// SafeDistanceSerializer::Deserialize(...)
// 函数声明在 SafeDistanceSerializer.h 里
#include "carla/sensor/s11n/SafeDistanceSerializer.h"

// 引入 SafeDistanceEvent
// Deserialize() 里要创建这个类型：
// new data::SafeDistanceEvent(...)
// 所以这里需要完整定义，不能只靠前向声明
#include "carla/sensor/data/SafeDistanceEvent.h"

// 进入 carla 命名空间
namespace carla
{
    // 进入 carla::sensor 命名空间
    namespace sensor
    {
        // 进入 carla::sensor::s11n 命名空间
        // 因为 SafeDistanceSerializer 定义在这个命名空间里
        namespace s11n
        {

            // 实现 SafeDistanceSerializer 的静态函数 Deserialize()

            // 返回类型：
            // SharedPtr<SensorData>
            // 意思是返回一个指向 SensorData 的智能指针
            // CARLA 里所有 sensor 数据最终都统一看作 SensorData 的子类
            // 例如：
            // Image                 是 SensorData
            // CollisionEvent        是 SensorData
            // ObstacleDetectionEvent 是 SensorData
            // SafeDistanceEvent     也是 SensorData
            // 所以返回基类指针，实际对象可以是子类

            // 参数：
            // RawData &&data
            // 表示从 server 收到的原始二进制数据
            // && 是右值引用
            // 这里的意思是：这段数据可以被“移动”进 SafeDistanceEvent，避免额外复制
            SharedPtr<SensorData> SafeDistanceSerializer::Deserialize(RawData &&data)
            {
                // 创建一个 SafeDistanceEvent，并把 raw data 移进去

                // 拆开看：
                // std::move(data)
                // 把 data 转成可移动对象
                // 后面 SafeDistanceEvent 会接管这段 raw bytes

                // new data::SafeDistanceEvent(std::move(data))
                // 创建一个新的 SafeDistanceEvent 对象
                // 这里的 data::SafeDistanceEvent 不是变量 data，而是命名空间：
                // carla::sensor::data::SafeDistanceEvent
                // 因为当前所在命名空间是：
                // carla::sensor::s11n
                // 所以 data::SafeDistanceEvent 指的是同级的：
                // carla::sensor::data::SafeDistanceEvent

                // SharedPtr<SensorData>(...)
                // 把裸指针包装成 CARLA 的智能指针，并向上转型成 SensorData
                // 最终返回给 CARLA sensor 系统
                return SharedPtr<SensorData>(new data::SafeDistanceEvent(std::move(data)));
            }

        } // namespace s11n
    } // namespace sensor
} // namespace carla

// 这段代码很短，但它在链路里很关键：
// server 端：
// ASafeDistanceSensor::PostPhysTick()
//   -> Stream.SerializeAndSend(...)
//   -> SafeDistanceSerializer::Serialize(...)
//   -> 发送 raw bytes

// client 端：
// 收到 raw bytes
//   -> SafeDistanceSerializer::Deserialize(...)
//   -> new SafeDistanceEvent(raw bytes)
//   -> Python callback 收到 SafeDistanceEvent

// 注意，它并没有在 Deserialize() 里手动解析 actor id
// 真正让 Python 能 list(event) 的地方，是 SafeDistanceEvent 继承了：
// Array<rpc::ActorId>

// 所以这段 raw bytes 会被解释为：
// 一段连续的 ActorId 数组