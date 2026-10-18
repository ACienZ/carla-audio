// ============================================================
// AudioSourceSensor.cpp
//
// 对应头文件：AudioSourceSensor.h
// 对应类：AAudioSourceSensor
//
// 当前职责：
// 1. 定义 sensor.other.audio_source blueprint。
// 2. 读取 Python 指定的 target_actor_id。
// 3. 每次采样时获取 ego 和目标的世界坐标。
// 4. 计算两者 Actor 原点之间的三维距离，单位米。
// 5. 通过 CARLA 传感器数据流发送测量结果。
//
// Python 最终读取：
// event.frame       -> CARLA 通用传感器头中的帧号。
// event.distance_m  -> 我们自定义载荷中的距离。
//
// 本文件不负责生成声音。
// Python 在收到并保存测量数据后，再进行声音合成。
// ============================================================

// CARLA Unreal 插件的公共头文件。
#include "Carla.h"

// 引入本类声明。
// 这里的文件名、类名必须与上一版头文件保持一致。
#include "Carla/Sensor/AudioSourceSensor.h"

// 提供 blueprint 定义与属性读取工具。
#include "Carla/Actor/ActorBlueprintFunctionLibrary.h"

// 提供 Actor ID 到 FCarlaActor 的查找接口。
#include "Carla/Actor/ActorRegistry.h"

// FCarlaActor 是 CARLA 对 Unreal actor 的管理对象。
// 后面通过它获取真正的 AActor。
#include "Carla/Actor/CarlaActor.h"

// 提供当前 episode 及其 actor registry。
#include "Carla/Game/CarlaEpisode.h"

// ============================================================
// 1. 构造函数
// ============================================================

AAudioSourceSensor::AAudioSourceSensor(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
    // : Super(ObjectInitializer)
    // 表示先调用父类 ASensor 的构造函数。
    // 父类负责初始化通用传感器组件和状态。

    // 允许这个 Actor 参与 tick 更新。
    //
    // ASensor::Tick() 由 CARLA 实现，且不能被子类重写。
    // 我们的采样逻辑放在下面的 PostPhysTick()。
    //
    // 这行只是允许更新，不代表强制每个世界帧都发送数据。
    // 实际采样仍受 sensor_tick 等配置影响。
    PrimaryActorTick.bCanEverTick = true;

    // 不创建 BoxComponent。
    //
    // 本版已经由 Python 指定目标车辆，
    // 不需要通过检测盒子搜索“附近有哪些车辆”。
    //
    // 无论目标在 ego 前方、后方还是侧面，
    // 只要目标有效，就可以计算距离。
}

// ============================================================
// 2. 定义 blueprint 名称和属性
// ============================================================

// FActorDefinition AAudioSourceSensor::GetSensorDefinition()
// {
//     // 创建一个通用传感器定义。
//     //
//     // "other"        -> 传感器类别。
//     // "audio_source" -> 传感器名称。
//     //
//     // 组合后的 Python blueprint ID：
//     // sensor.other.audio_source
//     auto Definition =
//         UActorBlueprintFunctionLibrary::MakeGenericSensorDefinition(
//             TEXT("other"),
//             TEXT("audio_source"));

//     // 声明一个可由 Python 设置的属性。
//     FActorVariation TargetId;

//     // Python 设置这个属性时使用的名称：
//     //
//     // bp.set_attribute(
//     //     "target_actor_id",
//     //     str(ambulance.id)
//     // )
//     TargetId.Id = TEXT("target_actor_id");

//     // 目标 ID 是整数，因此 blueprint 属性类型设为 Int。
//     //
//     // set_attribute() 的参数仍然写成字符串；
//     // CARLA 会根据这里声明的类型解释它。
//     TargetId.Type = EActorAttributeType::Int;

//     // 默认值 0 表示尚未指定有效目标。
//     //
//     // 默认值不代表让传感器自动寻找救护车。
//     // Python 必须在 spawn sensor 之前填写实际 ambulance.id。
//     TargetId.RecommendedValues = {TEXT("0")};

//     // 允许设置推荐列表以外的值。
//     // 否则 Python 就不能填写实际生成的车辆 ID。
//     TargetId.bRestrictToRecommended = false;

//     // 把目标 ID 属性加入 blueprint 定义。
//     Definition.Variations.Add(TargetId);

//     // 返回完整定义，供 CARLA 注册和创建传感器时使用。
//     return Definition;
// }

FActorDefinition AAudioSourceSensor::GetSensorDefinition()
{
    // 创建通用传感器定义。
    //
    // Python blueprint ID：
    // sensor.other.audio_source
    //
    // 注意：
    // MakeGenericSensorDefinition() 不会自动添加 sensor_tick。
    // 因此下面需要显式声明它。
    auto Definition =
        UActorBlueprintFunctionLibrary::MakeGenericSensorDefinition(
            TEXT("other"),
            TEXT("audio_source"));

    // ========================================================
    // 1. 目标 Actor ID
    // ========================================================

    FActorVariation TargetId;

    // Python：
    // bp.set_attribute("target_actor_id", str(ambulance.id))
    TargetId.Id = TEXT("target_actor_id");

    // Actor ID 配置使用整数类型。
    TargetId.Type = EActorAttributeType::Int;

    // 0 表示尚未指定有效目标。
    TargetId.RecommendedValues = {TEXT("0")};

    // 允许 Python 填写实际生成的车辆 ID。
    TargetId.bRestrictToRecommended = false;

    // ========================================================
    // 2. 传感器采样时间间隔
    // ========================================================

    FActorVariation SensorTick;

    // Python：
    // bp.set_attribute("sensor_tick", "0.0")
    SensorTick.Id = TEXT("sensor_tick");

    // 时间间隔可以包含小数，因此使用浮点类型。
    SensorTick.Type = EActorAttributeType::Float;

    // 单位是仿真秒。
    //
    // 0.0：
    //     每个仿真更新步都进行采样。
    //
    // 正数：
    //     请求按指定的仿真时间间隔更新，
    //     实际执行仍受世界更新步长影响。
    SensorTick.RecommendedValues = {TEXT("0.0")};

    // 允许设置其他采样间隔。
    SensorTick.bRestrictToRecommended = false;

    // 将两个属性都加入 Blueprint。
    //
    // 创建传感器时：
    // - 本类 Set() 读取 target_actor_id。
    // - Super::Set() 读取 sensor_tick 并设置更新间隔。
    Definition.Variations.Append({TargetId, SensorTick});

    return Definition;
}

// ============================================================
// 3. 读取本次创建传感器时的实际配置
// ============================================================

void AAudioSourceSensor::Set(
    const FActorDescription &ActorDescription)
{
    // 先让父类处理通用属性，例如 sensor_tick。
    Super::Set(ActorDescription);

    // 读取 Python 设置的 target_actor_id。
    //
    // 三个参数分别是：
    // 1. 属性名。
    // 2. 本次创建传感器时使用的属性集合。
    // 3. 找不到该属性时使用的默认值。
    //
    // RetrieveActorAttributeToInt 返回有符号整数。
    // 因此先检查它大于 0，再转换为头文件中的 uint32。
    //
    // 本版配置入口使用正 int32 范围的 Actor ID。
    const int32 ConfiguredTargetId =
        UActorBlueprintFunctionLibrary::RetrieveActorAttributeToInt(
            TEXT("target_actor_id"),
            ActorDescription.Variations,
            0);

    // 先恢复为“未配置目标”。
    // 即使 Set() 再次被调用，也不会误用上一次保存的 ID。
    TargetActorId = 0u;

    if (ConfiguredTargetId <= 0)
    {
        // 配置阶段给出明确提示。
        //
        // 这里不伪造目标，也不选择场景中的其他车辆。
        // 后续采样会因为 TargetActorId == 0 而跳过发送。
        UE_LOG(
            LogCarla,
            Warning,
            TEXT(
                "AudioSourceSensor: target_actor_id must be positive; "
                "received %d. No measurements will be sent."),
            ConfiguredTargetId);

        return;
    }

    // static_cast<uint32>：
    // 显式转换成无符号 32 位整数。
    //
    // 因为已经排除了负数和 0，
    // 不会发生负数转换成巨大无符号数的问题。
    TargetActorId = static_cast<uint32>(ConfiguredTargetId);

    // Set() 只保存目标身份。
    //
    // 不在这里计算距离，也不要求此时已经绑定 ego。
    // 真正测量在 PostPhysTick() 中进行。
}

// ============================================================
// 4. 每次采样：获取位置、计算距离并发送
// ============================================================

void AAudioSourceSensor::PostPhysTick(
    UWorld *World,
    ELevelTick TickType,
    float DeltaSeconds)
{
    // 保留父类的 PostPhysTick 调用。
    Super::PostPhysTick(World, TickType, DeltaSeconds);

    // --------------------------------------------------------
    // 4.1 检查是否指定了目标
    // --------------------------------------------------------

    if (TargetActorId == 0u)
    {
        // 没有目标就没有有效测量。
        // 本次不发送，不把距离填成 0。
        return;
    }

    // --------------------------------------------------------
    // 4.2 获取采集车 ego
    // --------------------------------------------------------

    // Python 创建 sensor 时使用 attach_to=ego，
    // CARLA 会设置它与 ego 的附着和 owner 关系。
    //
    // GetOwner() 返回服务端的 Unreal actor 指针。
    //
    // 注意：
    // 我们测的是 ego 原点到目标原点的距离，
    // 所以这里使用 GetOwner()。
    //
    // 如果改用传感器自己的 GetActorLocation()，
    // 测到的就是传感器安装位置到目标的距离。
    const AActor *EgoActor = GetOwner();

    // IsValid() 不仅检查是否为空，
    // 还检查 Unreal 对象是否处于有效状态。
    if (!IsValid(EgoActor))
    {
        // 例如没有 attach_to=ego，或者 ego 已被销毁。
        UE_LOG(
            LogCarla,
            Warning,
            TEXT("AudioSourceSensor: ego owner is invalid; skip measurement."));

        return;
    }

    // --------------------------------------------------------
    // 4.3 根据 Actor ID 查找目标
    // --------------------------------------------------------

    // GetEpisode() 返回当前 CARLA episode 的只读引用。
    //
    // episode 内部的 actor registry 保存了：
    // CARLA Actor ID -> FCarlaActor
    //
    // 使用 registry 的 const 查找接口，
    // 不需要修改 episode，也不需要去掉 const。
    const FActorRegistry &Registry = GetEpisode().GetActorRegistry();

    const FCarlaActor *TargetCarlaActor =
        Registry.FindCarlaActor(TargetActorId);

    // 查不到目标时返回空指针。
    //
    // 查到注册信息也不一定代表目标当前处于活动状态，
    // 例如大地图中的 dormant actor。
    //
    // 本版只对活动的目标进行测量。
    if (TargetCarlaActor == nullptr || !TargetCarlaActor->IsActive())
    {
        UE_LOG(
            LogCarla,
            Warning,
            TEXT("AudioSourceSensor: target %u is missing or inactive; skip measurement."),
            TargetActorId);

        return;
    }

    // FCarlaActor 是 CARLA 的管理对象；
    // GetActor() 取出它对应的 Unreal AActor。
    //
    // 后面通过 AActor 读取目标世界坐标。
    const AActor *TargetActor = TargetCarlaActor->GetActor();

    // 防止 Python 误把 ego.id 配置成目标 ID。
    //
    // 当前任务要求测量 ego 与另一个目标之间的距离，
    // 所以这里不接受把 ego 自己当作声源。
    if (!IsValid(TargetActor) || TargetActor == EgoActor)
    {
        UE_LOG(
            LogCarla,
            Warning,
            TEXT("AudioSourceSensor: target %u is invalid or equals ego; skip measurement."),
            TargetActorId);

        return;
    }

    // --------------------------------------------------------
    // 4.4 在本次采样中读取两端位置
    // --------------------------------------------------------

    // FVector 包含 X、Y、Z 三个分量。
    //
    // GetActorLocation() 返回 Unreal 世界坐标，
    // 本处长度单位为厘米。
    //
    // 读取的是 Actor 原点，不是车头、保险杠，
    // 也不是救护车车顶警笛的精确安装位置。
    const FVector EgoLocation = EgoActor->GetActorLocation();
    const FVector TargetLocation = TargetActor->GetActorLocation();

    // --------------------------------------------------------
    // 4.5 计算三维距离并转换为米
    // --------------------------------------------------------

    // 三维欧氏距离：
    //
    // sqrt(
    //     (target.x - ego.x)^2 +
    //     (target.y - ego.y)^2 +
    //     (target.z - ego.z)^2
    // )
    //
    // FVector::Dist() 完成上面的计算。
    const float DistanceCm = FVector::Dist(
        EgoLocation,
        TargetLocation);

    // 100 厘米 = 1 米。
    constexpr float CmToMeters = 0.01f;

    const float DistanceMeters = DistanceCm * CmToMeters;

    // 避免把异常坐标导致的 NaN / Infinity 当成有效距离发送。
    //
    // IsFinite() 表示这个数值既不是 NaN，也不是无穷大。
    if (!FMath::IsFinite(DistanceMeters))
    {
        UE_LOG(
            LogCarla,
            Warning,
            TEXT("AudioSourceSensor: distance is not finite; skip measurement."));

        return;
    }

    // --------------------------------------------------------
    // 4.6 获取本次采样的数据流并发送
    // --------------------------------------------------------

    // *this 表示当前 AAudioSourceSensor 对象。
    //
    // CARLA 使用具体传感器类型匹配 SensorRegistry。
    // 创建本次发送对象时，CARLA 会准备通用传感器头，
    // 其中包含帧号、时间戳以及传感器 transform。
    //
    // 我们在当前采样函数中计算距离并创建发送对象，
    // 不把距离缓存到下一帧再发送。
    auto Stream = GetDataStream(*this);

    // 自定义载荷当前只需要一个距离值。
    //
    // event.frame：
    //     来自 CARLA 通用头。
    //
    // event.distance_m：
    //     来自 DistanceMeters，经序列化、反序列化和
    //     Python 绑定后提供给客户端。
    //
    // SerializeAndSend() 会：
    // 1. 根据 SensorRegistry 找到 AudioSourceSerializer。
    // 2. 调用其 Serialize(*this, DistanceMeters)。
    // 3. 将通用头和距离载荷一起交给数据流发送。
    //
    // 注意：
    // 后续 AudioSourceSerializer 必须实现匹配这个调用的接口。
    // 旧 AudioSerializer 接收的是 episode 和 actor 集合，
    // 不能直接与本行配套使用。
    Stream.SerializeAndSend(*this, DistanceMeters);
}