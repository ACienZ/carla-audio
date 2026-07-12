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

// 构造函数实现。
ASafeDistanceSensor::ASafeDistanceSensor(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{

    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxOverlap"));
    Box->SetupAttachment(RootComponent);
    Box->SetHiddenInGame(true); // Disable for debugging.
    Box->SetCollisionProfileName(FName("OverlapAll"));

    PrimaryActorTick.bCanEverTick = true;
}

FActorDefinition ASafeDistanceSensor::GetSensorDefinition()
{
    auto Definition = UActorBlueprintFunctionLibrary::MakeGenericSensorDefinition(
        TEXT("other"),
        TEXT("safe_distance"));

    FActorVariation Front;
    Front.Id = TEXT("safe_distance_front");
    Front.Type = EActorAttributeType::Float;
    Front.RecommendedValues = {TEXT("1.0")};
    Front.bRestrictToRecommended = false;

    FActorVariation Back;
    Back.Id = TEXT("safe_distance_back");
    Back.Type = EActorAttributeType::Float;
    Back.RecommendedValues = {TEXT("0.5")};
    Back.bRestrictToRecommended = false;

    FActorVariation Lateral;
    Lateral.Id = TEXT("safe_distance_lateral");
    Lateral.Type = EActorAttributeType::Float;
    Lateral.RecommendedValues = {TEXT("0.5")};
    Lateral.bRestrictToRecommended = false;

    Definition.Variations.Append({Front, Back, Lateral});

    return Definition;
}

void ASafeDistanceSensor::Set(const FActorDescription &Description)
{
    Super::Set(Description);

    float Front = UActorBlueprintFunctionLibrary::RetrieveActorAttributeToFloat(
        "safe_distance_front",
        Description.Variations,
        1.0f);
    float Back = UActorBlueprintFunctionLibrary::RetrieveActorAttributeToFloat(
        "safe_distance_back",
        Description.Variations,
        0.5f);
    float Lateral = UActorBlueprintFunctionLibrary::RetrieveActorAttributeToFloat(
        "safe_distance_lateral",
        Description.Variations,
        0.5f);

    constexpr float M_TO_CM = 100.0f; // Unit conversion.

    float LocationX = M_TO_CM * (Front - Back) / 2.0f;
    float ExtentX = M_TO_CM * (Front + Back) / 2.0f;
    float ExtentY = M_TO_CM * Lateral;

    Box->SetRelativeLocation(FVector{LocationX, 0.0f, 0.0f});
    Box->SetBoxExtent(FVector{ExtentX, ExtentY, 0.0f});
}

void ASafeDistanceSensor::SetOwner(AActor *Owner)
{
    Super::SetOwner(Owner);

    auto BoundingBox = UBoundingBoxCalculator::GetActorBoundingBox(Owner);

    Box->SetBoxExtent(BoundingBox.Extent + Box->GetUnscaledBoxExtent());
}

// void ASafeDistanceSensor::Tick(float DeltaSeconds)
// {
//     Super::Tick(DeltaSeconds);

//     TSet<AActor *> DetectedActors;
//     Box->GetOverlappingActors(DetectedActors, ACarlaWheeledVehicle::StaticClass());
//     DetectedActors.Remove(GetOwner());

//     if (DetectedActors.Num() > 0)
//     {
//         auto Stream = GetDataStream(*this);
//         Stream.Send(*this, GetEpisode(), DetectedActors);
//     }
// }

void ASafeDistanceSensor::PostPhysTick(UWorld *World, ELevelTick TickType, float DeltaSeconds)
{
    TSet<AActor *> DetectedActors;
    Box->GetOverlappingActors(DetectedActors, ACarlaWheeledVehicle::StaticClass());
    DetectedActors.Remove(GetOwner());

    if (DetectedActors.Num() > 0)
    {
        auto Stream = GetDataStream(*this);
        Stream.SerializeAndSend(*this, GetEpisode(), DetectedActors);
    }
}
