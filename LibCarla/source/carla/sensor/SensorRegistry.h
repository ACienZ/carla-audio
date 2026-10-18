// Copyright (c) 2022 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef LIBCARLA_SENSOR_REGISTRY_INCLUDE_H
#define LIBCARLA_SENSOR_REGISTRY_INCLUDE_H

#include "carla/sensor/CompositeSerializer.h"

// =============================================================================
// Follow the 4 steps to register a new sensor.
// =============================================================================

// 1. Include the serializer here.
#include "carla/sensor/s11n/CollisionEventSerializer.h"
#include "carla/sensor/s11n/DVSEventArraySerializer.h"
#include "carla/sensor/s11n/EpisodeStateSerializer.h"
#include "carla/sensor/s11n/GnssSerializer.h"
#include "carla/sensor/s11n/ImageSerializer.h"
#include "carla/sensor/s11n/NormalsImageSerializer.h"
#include "carla/sensor/s11n/OpticalFlowImageSerializer.h"
#include "carla/sensor/s11n/IMUSerializer.h"
#include "carla/sensor/s11n/LidarSerializer.h"
#include "carla/sensor/s11n/NoopSerializer.h"
#include "carla/sensor/s11n/ObstacleDetectionEventSerializer.h"
#include "carla/sensor/s11n/RadarSerializer.h"
#include "carla/sensor/s11n/SemanticLidarSerializer.h"
#include "carla/sensor/s11n/GBufferUint8Serializer.h"
#include "carla/sensor/s11n/GBufferFloatSerializer.h"
#include "carla/sensor/s11n/V2XSerializer.h"

// 1.
// 引入声源传感器的序列化器。
//
// SensorRegistry 后面需要引用：
// s11n::AudioSourceSerializer
//
// 这个类声明并实现了距离的 Serialize()，
// 同时声明了客户端使用的 Deserialize()。
//
// 注意这里引入的是 LibCarla 的序列化器，
// 不是 Unreal 的传感器头文件。
#include "carla/sensor/s11n/AudioSourceSerializer.h"

// 2. Add a forward-declaration of the sensor here.
class ACollisionSensor;
class ADepthCamera;
class ANormalsCamera;
class ADVSCamera;
class AGnssSensor;
class AInertialMeasurementUnit;
class ALaneInvasionSensor;
class AObstacleDetectionSensor;
class AOpticalFlowCamera;
class ARadar;
class ARayCastSemanticLidar;
class ARayCastLidar;
class ASceneCaptureCamera;
class ASemanticSegmentationCamera;
class AInstanceSegmentationCamera;
class ACosmosControlSensor;
class ARssSensor;
class FWorldObserver;
struct FCameraGBufferUint8;
struct FCameraGBufferFloat;
class AV2XSensor;
class ACustomV2XSensor;

// 2.
// 前向声明 Unreal 侧的声源传感器类。
//
// 含义是告诉编译器：
// 存在一个名为 AAudioSourceSensor 的类。
//
// 后面的注册项只需要写出它的指针类型：
// AAudioSourceSensor *
//
// 因此这里不需要引入完整的 Unreal 类定义。
//
// 这样客户端编译 LibCarla 时，也可以识别注册表中的类型名称，
// 而不必依赖完整的 Unreal Engine 头文件。
class AAudioSourceSensor;

namespace carla
{
  namespace sensor
  {

    // 3. Register the sensor and its serializer in the SensorRegistry.

    /// Contains a registry of all the sensors available and allows serializing
    /// and deserializing sensor data for the types registered.
    ///
    /// Use s11n::NoopSerializer if the sensor does not send data (sensors that
    /// work only on client-side).
    using SensorRegistry = CompositeSerializer<
        std::pair<ACollisionSensor *, s11n::CollisionEventSerializer>,
        std::pair<ADepthCamera *, s11n::ImageSerializer>,
        std::pair<ANormalsCamera *, s11n::NormalsImageSerializer>,
        std::pair<ADVSCamera *, s11n::DVSEventArraySerializer>,
        std::pair<AGnssSensor *, s11n::GnssSerializer>,
        std::pair<AInertialMeasurementUnit *, s11n::IMUSerializer>,
        std::pair<ALaneInvasionSensor *, s11n::NoopSerializer>,
        std::pair<AObstacleDetectionSensor *, s11n::ObstacleDetectionEventSerializer>,
        std::pair<AOpticalFlowCamera *, s11n::OpticalFlowImageSerializer>,
        std::pair<ARadar *, s11n::RadarSerializer>,
        std::pair<ARayCastSemanticLidar *, s11n::SemanticLidarSerializer>,
        std::pair<ARayCastLidar *, s11n::LidarSerializer>,
        std::pair<ARssSensor *, s11n::NoopSerializer>,
        std::pair<ASceneCaptureCamera *, s11n::ImageSerializer>,
        std::pair<ASemanticSegmentationCamera *, s11n::ImageSerializer>,
        std::pair<AInstanceSegmentationCamera *, s11n::ImageSerializer>,
        std::pair<ACosmosControlSensor *, s11n::ImageSerializer>,
        std::pair<FWorldObserver *, s11n::EpisodeStateSerializer>,
        std::pair<FCameraGBufferUint8 *, s11n::GBufferUint8Serializer>,
        std::pair<FCameraGBufferFloat *, s11n::GBufferFloatSerializer>,
        std::pair<AV2XSensor *, s11n::CAMDataSerializer>,
        std::pair<ACustomV2XSensor *, s11n::CustomV2XDataSerializer>,

        // 3.
        // 注册“声源传感器类型”和“序列化器类型”的对应关系。
        //
        // std::pair 的两个类型参数分别是：
        //
        // AAudioSourceSensor *
        //     Unreal 服务端的具体传感器类型。
        //
        // s11n::AudioSourceSerializer
        //     与它配套的数据序列化器。
        //
        // 服务端调用：
        // Stream.SerializeAndSend(*this, DistanceMeters);
        //
        // 时，注册表会根据 *this 的传感器类型，
        // 匹配 AudioSourceSerializer::Serialize()。
        //
        // 客户端收到消息后，则通过通用传感器头中的类型标识，
        // 匹配 AudioSourceSerializer::Deserialize()，
        // 最终创建 AudioSourceEvent。
        //
        // 这里的指针类型用于模板中的类型匹配，
        // 不表示把服务端传感器指针发送给 Python。
        std::pair<AAudioSourceSensor *, s11n::AudioSourceSerializer>

        >;

  } // namespace sensor
} // namespace carla

#endif // LIBCARLA_SENSOR_REGISTRY_INCLUDE_H

#ifdef LIBCARLA_SENSOR_REGISTRY_WITH_SENSOR_INCLUDES

// 4. Include the sensor here.
#include "Carla/Sensor/CollisionSensor.h"
#include "Carla/Sensor/DepthCamera.h"
#include "Carla/Sensor/NormalsCamera.h"
#include "Carla/Sensor/DVSCamera.h"
#include "Carla/Sensor/GnssSensor.h"
#include "Carla/Sensor/InertialMeasurementUnit.h"
#include "Carla/Sensor/LaneInvasionSensor.h"
#include "Carla/Sensor/ObstacleDetectionSensor.h"
#include "Carla/Sensor/OpticalFlowCamera.h"
#include "Carla/Sensor/Radar.h"
#include "Carla/Sensor/RayCastLidar.h"
#include "Carla/Sensor/RayCastSemanticLidar.h"
#include "Carla/Sensor/RssSensor.h"
#include "Carla/Sensor/SceneCaptureCamera.h"
#include "Carla/Sensor/SemanticSegmentationCamera.h"
#include "Carla/Sensor/InstanceSegmentationCamera.h"
#include "Carla/Sensor/CosmosControlSensor.h"
#include "Carla/Sensor/WorldObserver.h"
#include "Carla/Sensor/V2XSensor.h"
#include "Carla/Sensor/CustomV2XSensor.h"

// 4.
// 在需要完整 Unreal 传感器类型的编译场景下，
// 引入 AAudioSourceSensor 的完整定义。
//
// 前面的前向声明只提供类名；
// 这个头文件提供继承关系、成员函数等完整信息。
//
// 必须保留在
// LIBCARLA_SENSOR_REGISTRY_WITH_SENSOR_INCLUDES
// 控制的区域内，不要移到文件顶部。
#include "Carla/Sensor/AudioSourceSensor.h"

#endif // LIBCARLA_SENSOR_REGISTRY_WITH_SENSOR_INCLUDES
