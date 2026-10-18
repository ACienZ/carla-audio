#pragma once
// 防止这个头文件在同一个编译单元中被重复包含。

// ============================================================
// 这个传感器要完成什么？
// ============================================================
//
// Python 端：
// 1. 创建数据采集车 ego。
// 2. 创建救护车 ambulance。
// 3. 获取 sensor.other.audio_source 的 blueprint。
// 4. 通过 target_actor_id 属性指定 ambulance.id。
// 5. 创建传感器，并通过 attach_to=ego 将它绑定到采集车。
// 6. 使用 sensor.listen(callback) 接收测量结果。
//
// Unreal / CARLA 服务端：
// 1. 从传感器的 owner 关系中找到 ego。
// 2. 根据 TargetActorId 找到 ambulance。
// 3. 每次采样时读取两辆车的位置。
// 4. 计算两辆车 Actor 原点之间的三维直线距离。
// 5. 将距离交给序列化器，并通过 CARLA 的传感器数据流发送。
//
// Python 最终读取的接口目标：
//
// def callback(event):
//     print(event.frame)
//     print(event.distance_m)
//
// event.frame：
//     来自 CARLA 通用的 SensorData 元数据。
//     表示这次测量所属的仿真帧。
//     不是 Python 回调执行时的时间，也不是收到的第几个事件。
//
// event.distance_m：
//     是我们要新增的测量字段。
//     表示 ego Actor 原点到目标 Actor 原点的三维距离，单位米。
//
// 注意：
// 这个头文件只声明传感器本身。
// 距离计算、数据序列化、事件类型和 Python 绑定需要在其他文件中实现。
// 单独添加这个头文件，不会自动产生 event.distance_m。

// ============================================================
// 引入依赖
// ============================================================

// 引入 CARLA 的传感器基类 ASensor。
//
// 继承 ASensor 后，可以接入 CARLA 已有的：
// - 传感器生命周期；
// - 采样更新机制；
// - sensor_tick 等通用配置；
// - 传感器数据流。
//
// 我们只需要实现这个传感器自己的测量逻辑，
// 不需要自行编写一套网络发送和接收机制。
#include "Carla/Sensor/Sensor.h"

// FActorDefinition 描述 blueprint 的定义。
//
// 它回答：
// - 这个传感器叫什么？
// - 支持哪些配置属性？
// - 属性是什么类型？
// - 默认值是什么？
//
// 本传感器计划使用的 blueprint ID：
// sensor.other.audio_source
#include "Carla/Actor/ActorDefinition.h"

// FActorDescription 描述某次创建 actor 时实际使用的配置。
//
// 区分：
// FActorDefinition  = 允许设置哪些属性。
// FActorDescription = 本次实际设置了哪些值。
//
// 例如 Python 设置：
// bp.set_attribute("target_actor_id", str(ambulance.id))
//
// Set() 会从 FActorDescription 中读取这个值。
#include "Carla/Actor/ActorDescription.h"

// 由 Unreal Header Tool 根据当前头文件生成。
//
// 文件名必须与当前头文件对应：
// AudioSourceSensor.h -> AudioSourceSensor.generated.h
//
// 这一行必须放在本文件所有 #include 的最后。
#include "AudioSourceSensor.generated.h"

// ============================================================
// 声明声源传感器类
// ============================================================

// 告诉 Unreal：这是一个需要参与反射系统的类。
// 它与下面的 GENERATED_BODY() 配合使用。
UCLASS()

// AAudioSourceSensor：
// - 前缀 A 是 Unreal 对 Actor 类的命名惯例。
// - AudioSource 表示这个传感器关注用于声音合成的声源信息。
// - Sensor 表示它通过 CARLA 的传感器接口提供数据。
//
// CARLA_API：
// 用于模块之间的符号导出与链接。
//
// public ASensor：
// 表示 AAudioSourceSensor 继承 CARLA 的 ASensor。
class CARLA_API AAudioSourceSensor : public ASensor
{
    // 配合 UCLASS()，接入 Unreal 自动生成的反射代码。
    GENERATED_BODY()

public:
    // ========================================================
    // 构造函数
    // ========================================================

    // Unreal 创建这个传感器对象时调用。
    //
    // ObjectInitializer：
    // Unreal 对象创建流程提供的初始化器。
    // 在 .cpp 中会传给父类构造函数 Super(ObjectInitializer)。
    //
    // 本版采用“指定目标持续测距”：
    // 不需要创建 BoxComponent，也不依赖 overlap 检测。
    AAudioSourceSensor(const FObjectInitializer &ObjectInitializer);

    // ========================================================
    // 定义传感器 blueprint
    // ========================================================

    // 告诉 CARLA 这个传感器的名称和可配置属性。
    //
    // 计划定义：
    //
    // blueprint ID：
    //     sensor.other.audio_source
    //
    // 自定义属性：
    //     target_actor_id
    //
    // Python 使用方式：
    //
    // bp = world.get_blueprint_library().find(
    //     "sensor.other.audio_source"
    // )
    // bp.set_attribute("target_actor_id", str(ambulance.id))
    //
    // static：
    // 获取 blueprint 定义时，不需要先创建一个传感器实例。
    //
    // 注意：
    // 这里仅声明函数，具体属性定义写在 .cpp 中。
    static FActorDefinition GetSensorDefinition();

    // ========================================================
    // 读取本次创建传感器时的配置
    // ========================================================

    // 重写父类的 Set()。
    //
    // override：
    // 让编译器检查这个函数是否正确重写了父类的虚函数。
    //
    // ActorDescription：
    // 包含本次创建传感器时使用的属性值。
    //
    // .cpp 中的实现需要：
    // 1. 调用 Super::Set(ActorDescription)，处理通用配置。
    // 2. 读取 target_actor_id。
    // 3. 将目标 ID 保存到 TargetActorId。
    //
    // 这里只保存目标身份。
    // 距离需要随着车辆运动重新计算，不能在 Set() 中只算一次。
    void Set(const FActorDescription &ActorDescription) override;

    // ========================================================
    // 执行本次采样
    // ========================================================

    // 在 CARLA 的物理更新后采样阶段执行测量逻辑。
    //
    // 注意“每次采样”和“每个世界帧”有区别：
    // 具体采样频率还取决于 sensor_tick 等调度设置。
    //
    // 参数：
    //
    // World：
    //     当前 Unreal 世界。
    //
    // TickType：
    //     当前更新的类型，由 Unreal / CARLA 传入。
    //
    // DeltaSeconds：
    //     本次调用传入的更新时间间隔，单位秒。
    //
    // .cpp 中计划执行：
    //
    // 1. 通过 GetOwner() 获取采集车 ego。
    //
    //    Python 创建 sensor 时使用 attach_to=ego，
    //    CARLA 会建立相应的附着和 owner 关系。
    //
    // 2. 根据 TargetActorId 从当前 CARLA episode 查找目标。
    //
    //    Python 的 ambulance 对象不等于服务端 AActor*。
    //    需要使用双方都能识别的 Actor ID 查找服务端对象。
    //
    // 3. 确认 ego 和目标有效后，读取二者的世界坐标。
    //
    // 4. 计算 Actor 原点之间的三维欧氏距离。
    //
    //    Unreal 长度单位是厘米。
    //    发送给 Python 前，需要转换成米。
    //
    // 5. 使用本次采样对应的 CARLA 数据流，
    //    将距离交给序列化器并发送。
    //
    //    帧号由 CARLA 通用传感器元数据提供。
    //    不在本类中维护一个自行递增的 FrameCounter，
    //    避免它与真实的世界帧号产生偏差。
    //
    // 目标找不到时：
    // 不能把距离伪造为 0 米。
    // 具体采用何种无效测量表示，后续在事件格式中一起确定。
    //
    // CARLA 的 ASensor::Tick() 是 final，
    // 因此本类使用 PostPhysTick() 放置采样逻辑。
    void PostPhysTick(
        UWorld *World,
        ELevelTick TickType,
        float DeltaSeconds) override;

private:
    // ========================================================
    // 本传感器跟踪的目标
    // ========================================================

    // 目标车辆的 CARLA Actor ID。
    //
    // 来源：
    // Python 的 ambulance.id，经 target_actor_id 属性传入。
    //
    // 例如：
    // ambulance.id == 34
    // 则完成配置后 TargetActorId == 34。
    //
    // uint32：
    // Unreal 提供的无符号 32 位整数类型。
    //
    // 初始值 0：
    // 本实现约定表示尚未配置有效目标。
    // 后续 .cpp 需要检查配置及目标查找结果。
    //
    // 这里保存 ID，不长期保存目标车辆的裸指针。
    // 目标可能被销毁，因此每次采样仍需要检查它是否存在。
    //
    // 本版只跟踪一个目标。
    // 采集车通过 GetOwner() 获取，不需要再保存一个 EgoActorId。
    uint32 TargetActorId = 0u;
};