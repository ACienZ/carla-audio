#pragma once

#include "carla/Memory.h"
#include "carla/rpc/ActorId.h"
#include "carla/sensor/RawData.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace carla
{
    namespace sensor
    {

        class SensorData;

        namespace s11n
        {

            class SafeDistanceSerializer
            {
            public:
                template <typename SensorT, typename EpisodeT, typename ActorListT>
                static Buffer Serialize(
                    const SensorT &,
                    const EpisodeT &episode,
                    const ActorListT &detected_actors)
                {
                    // const uint32_t size_in_bytes = sizeof(ActorId) * detected_actors.Num();
                    // Buffer buffer{size_in_bytes};
                    // unsigned char *it = buffer.data();
                    // for (auto *actor : detected_actors)
                    // {
                    //     ActorId id = episode.FindActor(actor).GetActorId();
                    //     std::memcpy(it, &id, sizeof(ActorId));
                    //     it += sizeof(ActorId);
                    // }
                    // return buffer;

                    // GPT modified
                    std::vector<ActorId> ids;
                    ids.reserve(detected_actors.Num());

                    for (auto *actor : detected_actors)
                    {
                        if (auto *carla_actor = episode.FindCarlaActor(actor))
                        {
                            ids.push_back(carla_actor->GetActorId());
                        }
                    }

                    // Buffer buffer{sizeof(ActorId) * ids.size()};
                    // std::memcpy(buffer.data(), ids.data(), sizeof(ActorId) * ids.size());
                    // return buffer;
                    Buffer buffer{sizeof(ActorId) * ids.size()};
                    if (!ids.empty())
                    {
                        std::memcpy(buffer.data(), ids.data(), sizeof(ActorId) * ids.size());
                    }
                    return buffer;
                }

                static SharedPtr<SensorData> Deserialize(RawData &&data);
            };

        } // namespace s11n
    } // namespace sensor
} // namespace carla