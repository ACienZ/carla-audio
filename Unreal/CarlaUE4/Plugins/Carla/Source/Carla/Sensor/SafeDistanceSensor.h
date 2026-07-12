// 声明一个继承自 ASensor 的 CARLA sensor：ASafeDistanceSensor
// 声明它如何注册 blueprint：GetSensorDefinition()
// 声明它如何在仿真中工作：Set()、SetOwner()、PostPhysTick()

// 在 CARLA 里，blueprint 可以理解为“可生成对象的模板/配置说明书”。
// 它不是已经存在于世界里的车或 sensor，而是告诉 CARLA：
// 我想生成什么类型的 actor？
// 这个 actor 有哪些可配置属性？
// 这些属性默认值是什么？

// 例如 Python 里：
// bp = world.get_blueprint_library().find("vehicle.tesla.model3")
// vehicle = world.spawn_actor(bp, transform)
// 这里：
// vehicle.tesla.model3  是 blueprint
// vehicle               是根据 blueprint 生成出来的真实 actor

// 对于 sensor 也是一样：
// bp = world.get_blueprint_library().find("sensor.other.safe_distance")
// sensor = world.spawn_actor(bp, transform, attach_to=ego_vehicle)
// 这里：
// sensor.other.safe_distance  是 blueprint
// sensor                      是实际挂到 ego 车上的 sensor actor

// blueprint 里还可以有属性，例如 safe distance sensor 的：
// safe_distance_front
// safe_distance_back
// safe_distance_lateral
// Python 端可以设置：
// bp.set_attribute("safe_distance_front", "10.0")
// bp.set_attribute("safe_distance_back", "5.0")
// bp.set_attribute("safe_distance_lateral", "2.0")

// 然后 C++ 端的：
// void ASafeDistanceSensor::Set(const FActorDescription &ActorDescription)
// 就会读取这些属性，并真正设置 sensor 的行为。

#pragma once // 防止头文件被重复包含

// 引入 CARLA 的基础 sensor 类 ASensor
// ASafeDistanceSensor 继承它，才能使用 CARLA sensor 的生命周期、数据流、PostPhysTick、GetDataStream() 等机制。
#include "Carla/Sensor/Sensor.h"

// 它描述一个 actor blueprint 的定义，例如这个 sensor 在 Python 里叫什么、属于哪个类别、有哪些可配置属性。
// 对 safe distance sensor 来说，后面会生成：
// sensor.other.safe_distance
#include "Carla/Actor/ActorDefinition.h"
// 引入 FActorDescription。
// 它表示用户实际 spawn sensor 时传进来的配置值。
// 区别是：
// FActorDefinition   = 这个 sensor 支持哪些属性
// FActorDescription  = 这次创建 sensor 时，用户实际填了哪些属性值
// 例如 definition 里声明支持：
// safe_distance_front
// safe_distance_back
// safe_distance_lateral
// description 里保存 Python 端实际设置的值。
#include "Carla/Actor/ActorDescription.h"

// 引入 UE4 的 UBoxComponent。
// safe distance sensor 用一个盒子区域检测周围是否有车辆进入安全距离，所以需要 box collision component。
// 这个是 safe distance 特有的。
#include "Components/BoxComponent.h"

// 注意：这一行必须放在所有 #include 的最后，且文件名必须与当前文件名完全一致
// 这是 Unreal Header Tool 生成的头文件。
// 只要类里用了 UCLASS()、UPROPERTY()、GENERATED_BODY() 等 UE 宏，就必须 include 对应的 *.generated.h。
#include "SafeDistanceSensor.generated.h"

// 告诉 Unreal：下面这个 C++ 类是一个 UE 反射系统可识别的类。
// 有了它，Unreal 才能：
// 识别这个类是 UObject / AActor 体系的一部分
// 参与反射
// 参与 garbage collection
// 被 Unreal Header Tool 处理
// 在 CARLA actor factory / blueprint 系统中使用
UCLASS()
// A 是 Unreal 命名惯例，表示这个类继承自 AActor。
// ASensor 本身也是 actor，所以 sensor 是可以 spawn 到 world 里的对象。
// CARLA_API 是导出宏。
// 它让这个类可以被 CARLA 的不同模块正确链接到。Unreal 插件/模块之间访问 C++ 类时通常需要这种 API 宏。
// public ASensor
// 表示 safe distance sensor 是 CARLA sensor。
// 继承 ASensor 后，它才能像其他 sensor 一样被 Python spawn、attach、listen，并通过 CARLA streaming 机制把数据发到客户端。
class CARLA_API ASafeDistanceSensor : public ASensor
{
    // GENERATED_BODY() 会展开 Unreal 自动生成的一堆代码。
    // 它和 UCLASS() 配套使用，提供反射、类型信息、构造辅助等功能。
    // 只要是 UE 的 UCLASS，基本都需要它。
    GENERATED_BODY()

public:
    // 构造函数
    // FObjectInitializer 是 UE 创建对象时传入的初始化器。
    // 在 .cpp 里会用它调用父类构造函数，并创建默认组件，例如：
    // Box = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxOverlap"));
    // 这个构造函数负责初始化 sensor 自己需要的 UE 组件。
    ASafeDistanceSensor(const FObjectInitializer &ObjectInitializer);

    // 静态函数，用来告诉 CARLA：
    // 这个 sensor 的 blueprint id 是什么
    // 属于哪个类别
    // 有哪些可配置属性
    // 每个属性的类型是什么
    // 默认推荐值是什么
    // 对于 safe distance sensor，它会定义出：
    // sensor.other.safe_distance
    // 以及属性：
    // safe_distance_front
    // safe_distance_back
    // safe_distance_lateral
    // 之所以是 static，是因为 CARLA 注册 blueprint 时不需要先创建一个 sensor 实例，只需要问这个类：“你是什么 sensor？”
    static FActorDefinition GetSensorDefinition();

    // 重写 ASensor::Set()。
    // 这个函数会在 actor 根据 blueprint description 创建后被调用。
    // 它负责读取 Python 端传进来的属性，并应用到 sensor 自己身上。
    // safe distance sensor 里，它会读取：
    // safe_distance_front
    // safe_distance_back
    // safe_distance_lateral
    // 然后设置 box 的大小和位置。
    void Set(const FActorDescription &ActorDescription) override;

    // 重写 SetOwner()。
    // Owner 通常就是这个 sensor attach 到的 actor，例如 ego vehicle。
    // safe distance sensor 需要知道 owner 的 bounding box，因为它的检测区域要围绕 ego vehicle 扩展。
    // 所以在 .cpp 里会根据 owner 的车身尺寸调整 Box 的范围。
    void SetOwner(AActor *Owner) override;

    // CARLA 0.9.16 里的 ASensor::Tick() 是 final，子类不能 override。
    // 如果写：
    // void Tick(float DeltaSeconds) override;
    // 会编译失败。
    // 所以官方教程里让 override Tick()，在 0.9.16 里就是不适用的。
    // void Tick(float DeltaSeconds) override;

    // 重写 PostPhysTick()。
    // 这是 CARLA sensor 推荐使用的每帧更新入口。
    // 它在物理仿真 tick 之后执行。
    // 参数含义：
    // UWorld *World
    // 当前 Unreal world。
    // ELevelTick TickType
    // tick 类型，一般不需要重点关注。
    // float DeltaSeconds
    // 距离上一帧经过的仿真时间，单位是秒。
    // safe distance sensor 在这里做：
    // 每帧检测 box 内有哪些车辆
    // 如果检测到车辆，就把 actor id 发给 Python
    void PostPhysTick(UWorld *World, ELevelTick TickType, float DeltaSeconds) override;

private:
    // 告诉 Unreal 这个成员变量受 UE 反射/GC 系统管理。
    // 这里尤其重要，因为 Box 是一个 UE object/component。
    // 如果不用 UPROPERTY() 标记，Unreal 的垃圾回收系统可能不知道这个指针还在被使用。
    UPROPERTY()
    // 声明一个指向 box collision component 的指针。
    // 它代表 safe distance sensor 的检测区域。
    // UBoxComponent
    // UE4 的盒状碰撞组件。
    // *Box
    // 指针变量名。
    // = nullptr
    // 初始化为空指针，避免野指针。
    // 真正创建是在构造函数 .cpp 里完成的。
    UBoxComponent *Box = nullptr;
};