import glob
import queue
import sys
import time

# try:
#     sys.path.append(glob.glob("PythonAPI/carla/dist/carla-*%d.%d-%s.egg" % (
#         sys.version_info.major,
#         sys.version_info.minor,
#         "linux-x86_64"))[0])
# except IndexError:
#     pass

# import carla

try:
    egg = glob.glob("PythonAPI/carla/dist/carla-*%d.%d-%s.egg" % (
        sys.version_info.major,
        sys.version_info.minor,
        "linux-x86_64"))[0]
    sys.path.insert(0, egg)
except IndexError:
    pass

import carla
print("using carla:", carla.__file__)


def callback(event):
    print("type:", type(event))
    print("event:", event)
    print("detected ids:", list(event))
    print("frame:", event.frame, "timestamp:", event.timestamp)

    print("has raw_data:", hasattr(event, "raw_data"))
    print("has len:", hasattr(event, "__len__"))

    if hasattr(event, "raw_data"):
        raw = bytes(event.raw_data)
        print("raw_data bytes:", len(raw))
        print("raw_data hex:", raw.hex())

    try:
        print("len:", len(event))
        print("detected ids:", list(event))
    except Exception as e:
        print("list/len failed:", repr(e))


def main():
    client = carla.Client("127.0.0.1", 2000)
    client.set_timeout(10.0)

    world = client.get_world()
    bp_lib = world.get_blueprint_library()

    vehicle_bp = bp_lib.filter("vehicle.*")[0]
    sensor_bp = bp_lib.find("sensor.other.safe_distance")
    sensor_bp.set_attribute("safe_distance_front", "8.0")
    sensor_bp.set_attribute("safe_distance_back", "2.0")
    sensor_bp.set_attribute("safe_distance_lateral", "3.0")

    spawn_points = world.get_map().get_spawn_points()
    if len(spawn_points) < 2:
        raise RuntimeError("Need at least two spawn points")

    actors = []
    events = queue.Queue()

    try:
        ego = world.spawn_actor(vehicle_bp, spawn_points[0])
        actors.append(ego)

        other = world.spawn_actor(vehicle_bp, spawn_points[1])
        actors.append(other)

        sensor = world.spawn_actor(
            sensor_bp,
            carla.Transform(carla.Location(x=0.0, z=1.0)),
            attach_to=ego)
        actors.append(sensor)

        sensor.listen(events.put)

        print("ego id:", ego.id)
        print("other id:", other.id)
        print("sensor id:", sensor.id)
        print("listening for 10 seconds...")

        deadline = time.time() + 1.0
        while time.time() < deadline:
            try:
                data = events.get(timeout=1.0)
                # print("event:", data)
                callback(data)
                try:
                    print("as list:", list(data))
                except TypeError:
                    pass
            except queue.Empty:
                print("no event yet")

    finally:
        for actor in reversed(actors):
            if actor.is_alive:
                actor.destroy()


if __name__ == "__main__":
    main()