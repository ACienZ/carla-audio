// 创建一个用于检测的 BoxComponent
// 注册 sensor.other.safe_distance 这个 blueprint
// 根据 Python 端传入的属性设置检测盒子的大小
// 每帧检测盒子里有没有其他车辆，并把检测到的 actor 发给 Python

// 引入 CARLA Unreal 插件的公共头文件。
// 在 CARLA 的 Unreal 侧 .cpp 文件里通常都会先 include 它，类似项目级公共入口。
#include "Carla.h"

// 引入当前类自己的头文件。
#include "Carla/Sensor/SafeDistanceSensor.h"

// 引入 CARLA 提供的 actor blueprint 工具函数。
// 后面用它来：
// MakeGenericSensorDefinition(...)
// RetrieveActorAttributeToFloat(...)
// 也就是创建 sensor blueprint 定义，以及读取 Python 设置的属性。
#include "Carla/Actor/ActorBlueprintFunctionLibrary.h"

// 引入 CarlaEpisode。
// 后面发送数据时会用到：
// GetEpisode()
// serializer 需要通过 episode 把 Unreal 里的 AActor* 映射成 CARLA actor id。
#include "Carla/Game/CarlaEpisode.h"

// 引入 bounding box 计算工具。
// 后面 SetOwner() 里用它获取 ego vehicle 的包围盒尺寸。
#include "Carla/Util/BoundingBoxCalculator.h"

// 引入 CARLA 车辆类。
// 后面检测 overlap actor 时，只筛选车辆：
// ACarlaWheeledVehicle::StaticClass()
#include "Carla/Vehicle/CarlaWheeledVehicle.h"

// 构造函数实现

// ASafeDistanceSensor::ASafeDistanceSensor(...)
// 表示实现 ASafeDistanceSensor 类的构造函数

// const FObjectInitializer &ObjectInitializer
// 是 Unreal 创建对象时传入的初始化器

// : Super(ObjectInitializer)
// 调用父类 ASensor 的构造函数
// Super 是 Unreal 常用写法，表示父类
ASafeDistanceSensor::ASafeDistanceSensor(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
    // 创建一个 UE4 的盒子组件 UBoxComponent

    // CreateDefaultSubobject<UBoxComponent>
    // 表示这个组件是 actor 的默认子对象，随 sensor 一起创建

    // TEXT("BoxOverlap")
    // 组件名字
    // TEXT(...) 是 Unreal 的字符串宏，用来兼容不同字符编码

    // 这个 Box 就是 safe distance sensor 的检测区域
    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxOverlap"));

    // 把 Box 挂到 sensor actor 的根组件下面
    // 这样 sensor 移动、旋转时，Box 会跟着 sensor 一起移动、旋转
    Box->SetupAttachment(RootComponent);

    // 运行游戏时隐藏这个 box
    // 如果调试时想看到检测盒子，可以改成 false，但正常运行时不显示
    Box->SetHiddenInGame(true); // Disable for debugging.

    // 设置 box 的碰撞配置为 OverlapAll
    // 这意味着它不会真的撞到别的物体，也不会产生物理阻挡，只会检测“有哪些 actor 和我重叠”
    // safe distance sensor 正是靠这个 overlap 来判断附近是否有车辆
    Box->SetCollisionProfileName(FName("OverlapAll"));

    // 让这个 sensor 每帧都调用 Tick()，以便每帧检测 overlap
    PrimaryActorTick.bCanEverTick = true;
}

// 实现静态函数 GetSensorDefinition()

// 这个函数告诉 CARLA:
// 1. 这个 sensor 在 blueprint library 里叫什么
// 2. 有哪些可配置属性

// 返回值是 FActorDefinition
FActorDefinition ASafeDistanceSensor::GetSensorDefinition()
{
    // 创建一个通用 sensor blueprint 定义
    // 传入两个参数：
    // TEXT("other") 表示这个 sensor 属于“其他”类别
    // TEXT("safe_distance") 表示这个 sensor 的 blueprint id 是 safe_distance
    // 组合起来，Python 端看到的 blueprint id 就是: sensor.other.safe_distance
    auto Definition = UActorBlueprintFunctionLibrary::MakeGenericSensorDefinition(
        TEXT("other"),
        TEXT("safe_distance"));

    // 创建一个 sensor 属性定义，名字叫 Front
    // FActorVariation 表示 blueprint 的一个可配置属性
    FActorVariation Front;
    // 这个属性在 Python 端的名字是 safe_distance_front
    // Python 可以这样设置: bp.set_attribute("safe_distance_front", "10.0")
    Front.Id = TEXT("safe_distance_front");
    // 属性类型是浮点数
    Front.Type = EActorAttributeType::Float;
    // 推荐默认值是 "1.0"
    // 注意这里是字符串，因为 CARLA blueprint attribute 在 Python/配置层通常以字符串形式保存，C++ 端再转成 float
    Front.RecommendedValues = {TEXT("1.0")};
    // 允许用户设置任意值，不限制在推荐值里
    Front.bRestrictToRecommended = false;

    // 创建一个 sensor 属性定义，名字叫 Back，即定义后方安全距离属性
    FActorVariation Back;
    Back.Id = TEXT("safe_distance_back");
    Back.Type = EActorAttributeType::Float;
    Back.RecommendedValues = {TEXT("0.5")}; // 默认推荐值是 0.5 米
    Back.bRestrictToRecommended = false;

    // 定义左右方向安全距离属性
    FActorVariation Lateral;
    Lateral.Id = TEXT("safe_distance_lateral");
    Lateral.Type = EActorAttributeType::Float;
    Lateral.RecommendedValues = {TEXT("0.5")}; // 默认推荐值是 0.5 米
    Lateral.bRestrictToRecommended = false;

    // 把这三个属性加入 sensor blueprint 定义
    // 所以 Python 端查 blueprint 时，就能看到这个 sensor 支持三个 attribute：safe_distance_front、safe_distance_back、safe_distance_lateral
    Definition.Variations.Append({Front, Back, Lateral});

    // 返回完整的 blueprint 定义
    return Definition;
}

// 实现 Set() 函数
// 这个函数在 sensor 创建时被调用，用来读取用户传入的属性，并真正设置 sensor 行为
void ASafeDistanceSensor::Set(const FActorDescription &Description)
{
    // 先调用父类 ASensor::Set()
    // 这样父类会处理一些 sensor 通用属性，例如 sensor_tick 等
    Super::Set(Description);

    // 从 Description 里读取 safe_distance_front 属性，并转成 float
    // 三个参数含义是：
    // 1. "safe_distance_front": 要读取的属性名
    // 2. Description.Variations 是用户实际传入的属性值
    // 3. 默认值，如果用户没设置这个属性，就使用默认值 1.0
    // 即：Front = sensor 前方检测距离，单位是米
    float Front = UActorBlueprintFunctionLibrary::RetrieveActorAttributeToFloat(
        "safe_distance_front",
        Description.Variations,
        1.0f);
    // 读取后方检测距离。默认 0.5 米
    float Back = UActorBlueprintFunctionLibrary::RetrieveActorAttributeToFloat(
        "safe_distance_back",
        Description.Variations,
        0.5f);
    // 读取左右检测距离。默认 0.5 米
    float Lateral = UActorBlueprintFunctionLibrary::RetrieveActorAttributeToFloat(
        "safe_distance_lateral",
        Description.Variations,
        0.5f);

    // 定义米到厘米的换算系数
    // CARLA Python API 里我们通常用米，但 Unreal Engine 内部长度单位是厘米
    constexpr float M_TO_CM = 100.0f; // Unit conversion.

    // 在 Unreal/CARLA 中，通常：
    // X 轴：前后方向，正 X 是前方
    // Y 轴：左右方向
    // Z 轴：上下方向

    // 计算 box 相对于 sensor 原点在 X 方向的偏移
    // 为什么要偏移？
    // 假设：Front = 10 m Back  = 2 m
    // 我们希望检测范围从车后 2 米到车前 10 米，总长度是 12 米。这个 box 的中心不在车中心，而是偏向前方
    // 中心偏移量是：(Front - Back) / 2 = (10 - 2) / 2 = 4 m
    // 所以 box 中心往前移动 4 米
    float LocationX = M_TO_CM * (Front - Back) / 2.0f;
    // 计算 box 在 X 方向的半长度（extent）
    // 注意 UE 的 SetBoxExtent() 需要的是 half extent，不是完整长度
    // 如果前方 10 米、后方 2 米，总长度 12 米，那么：ExtentX = 12 / 2 = 6 m
    float ExtentX = M_TO_CM * (Front + Back) / 2.0f;
    // 计算 box 在 Y 方向的半宽度（extent）
    // 如果 Lateral = 2 m，那么 box 左右各 2 米，总宽度 4 米
    float ExtentY = M_TO_CM * Lateral;

    // 设置 box 相对于 sensor 的位置
    // 这里只在 X 方向偏移：X = LocationX Y = 0 Z = 0
    // 也就是根据前后安全距离，把 box 中心向前或向后移动
    Box->SetRelativeLocation(FVector{LocationX, 0.0f, 0.0f});
    // 设置 box 的半尺寸（extent）
    // ExtentX 是前后半长度，ExtentY 是左右半宽度
    // 这里 Z 先设成 0，因为后面 SetOwner() 会把 owner vehicle 的 bounding box 加进去，从而补上车辆自身高度
    Box->SetBoxExtent(FVector{ExtentX, ExtentY, 0.0f});
}

// 实现 SetOwner() 函数
// 当 sensor attach 到某个 actor 上时，这个 actor 就是 owner
// 例如 Python 里：world.spawn_actor(sensor_bp, transform, attach_to=ego_vehicle)
// 这里的 ego_vehicle 就会成为 sensor 的 owner
void ASafeDistanceSensor::SetOwner(AActor *Owner)
{
    // 先调用父类 SetOwner()，让 Unreal/CARLA 正确记录 owner 关系
    Super::SetOwner(Owner);

    // 获取 owner actor 的 bounding box
    // 对于 safe distance sensor 来说，owner 通常是 ego vehicle
    // 这个 bounding box 代表 ego 车辆本身的尺寸
    auto BoundingBox = UBoundingBoxCalculator::GetActorBoundingBox(Owner);

    // 把检测 box 的尺寸扩大，加上 ego vehicle 自身的尺寸
    // 原因是前面 Set() 里设置的 Front/Back/Lateral 更像是“车身外额外扩出去的安全距离”
    // 但我们希望检测范围是“ego vehicle 本身 + 安全距离”，所以要把 ego vehicle 的 bounding box 加进去
    // 最终 box 半尺寸 = 车辆自身半尺寸 + 用户设置的安全距离半尺寸
    // 例如 ego 车本身长 4 米，前方安全距离 10 米，那么最终检测区域会覆盖：车身本体 + 前方额外 10 米左右的范围
    Box->SetBoxExtent(BoundingBox.Extent + Box->GetUnscaledBoxExtent());
}

// 原本想通过 Tick() 每帧运行逻辑
// 但在 CARLA 0.9.16 里，ASensor::Tick() 是 final，子类不能 override
// 所以这段不能用

// void ASafeDistanceSensor::Tick(float DeltaSeconds)
// {
//     Super::Tick(DeltaSeconds);

//     // 创建一个 Unreal 的集合，用来保存检测到的 actor
//     // TSet 类似 C++ 标准库里的 std::set，但这是 Unreal 自己的容器
//     TSet<AActor *> DetectedActors;
//     // 获取当前和 Box 重叠的 actor，并且只保留类型是 ACarlaWheeledVehicle 的 actor -> 也就是只检测车辆
//     Box->GetOverlappingActors(DetectedActors, ACarlaWheeledVehicle::StaticClass());
//     // 从检测结果中移除 owner 自己
//     // 因为 sensor attach 在 ego vehicle 上，box 很可能和 ego 本身重叠
//     // 我们只关心 ego 车周围的其他车辆，所以要把 ego 自己排除掉
//     DetectedActors.Remove(GetOwner());

//     // 如果检测到的其他车辆数量大于 0，才发送事件
//     if (DetectedActors.Num() > 0)
//     {
//         // 获取这个 sensor 对应的数据流
//         // CARLA sensor 的数据最终就是通过这个 stream 发给 Python client
//         auto Stream = GetDataStream(*this);

//         // 这行也是问题点
//         // Send() 不会走 SensorRegistry 对应的 serializer
//         // 我们这个 sensor 需要把 DetectedActors 转成 actor id buffer，所以必须走 serializer
//         // 因此正确写法是后面的：SerializeAndSend(...)
//         Stream.Send(*this, GetEpisode(), DetectedActors);
//     }
// }

// 实现真正使用的每帧逻辑
// PostPhysTick() 是物理仿真之后的 tick
// UWorld *World: 当前 Unreal world
// ELevelTick TickType: tick 类型，一般不需要重点关注
// float DeltaSeconds: 上一帧到这一帧的时间间隔，单位秒
// 这三个参数在当前实现里没有使用
void ASafeDistanceSensor::PostPhysTick(UWorld *World, ELevelTick TickType, float DeltaSeconds)
{
    // 创建一个 actor 集合，用来保存检测到的车辆
    TSet<AActor *> DetectedActors;
    // 查询当前和 box 重叠的车辆
    // 这行是 safe distance sensor 的核心检测逻辑
    // 谁进入了我的安全距离盒子？
    // 只检测 ACarlaWheeledVehicle，所以行人、建筑、传感器等不会被放进结果
    Box->GetOverlappingActors(DetectedActors, ACarlaWheeledVehicle::StaticClass());
    // 从结果里删除 owner 自己
    // 否则 ego vehicle 可能永远被检测到，导致 sensor 一直触发
    DetectedActors.Remove(GetOwner());

    // 如果检测到至少一辆其他车，才发送数据
    // 所以这个 sensor 不是每帧都发
    // 只有检测区域里有车时才发 event
    if (DetectedActors.Num() > 0)
    {
        // 获取当前 sensor 的数据流对象
        // *this 表示当前 ASafeDistanceSensor 实例
        // 这个 stream 会把 UE server 端的数据发给 Python client
        auto Stream = GetDataStream(*this);
        // 序列化并发送数据
        // 这行非常关键
        // 它会根据 SensorRegistry 找到 ASafeDistanceSensor -> SafeDistanceSerializer
        // 然后调用 serializer，把 DetectedActors 转成 bytes
        // safe distance 的 serializer 会把 actor 转成 actor id，所以 Python 端最终能收到：list(event)
        // 也就是检测到的车辆 actor id 列表
        // 类似于：[25]

        // 参数含义：
        // 1. *this: 当前 sensor 对象
        // 2. GetEpisode(): 当前 CARLA episode，用来把 UE actor 映射成 CARLA actor id
        // 3. DetectedActors: 要发送的数据源，也就是检测到的车辆集合
        Stream.SerializeAndSend(*this, GetEpisode(), DetectedActors);
    }
}
