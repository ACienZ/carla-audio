#pragma once

#include "Carla/Sensor/Sensor.h"

#include "Carla/Actor/ActorDefinition.h"
#include "Carla/Actor/ActorDescription.h"

#include "Components/BoxComponent.h"

// 注意：这一行必须放在所有 #include 的最后，且文件名必须与当前文件名完全一致
#include "SafeDistanceSensor.generated.h"

UCLASS()
class CARLA_API ASafeDistanceSensor : public ASensor
{
    GENERATED_BODY()

public:
    ASafeDistanceSensor(const FObjectInitializer &ObjectInitializer);

    static FActorDefinition GetSensorDefinition();

    void Set(const FActorDescription &ActorDescription) override;

    void SetOwner(AActor *Owner) override;

    // void Tick(float DeltaSeconds) override;
    void PostPhysTick(UWorld *World, ELevelTick TickType, float DeltaSeconds) override;

private:
    UPROPERTY()
    UBoxComponent *Box = nullptr;
};