"""#10：用两辆静止车辆，确认 Python 能收到声源 sensor 的测量。

运行前启动本项目构建的 CARLA 服务端；使用 UE 编辑器时先点击 Play。
本脚本只接收并打印数据，不保存文件，也不进行完整的测距精度验证。
"""

from queue import Queue

import carla


# 本次是独立的接收验证，先在这里集中修改参数。
# 正式 DatasetGenerator 的配置文件格式会在后续实现时确定。
HOST = "127.0.0.1"
PORT = 2000
CLIENT_TIMEOUT_SECONDS = 10.0
EVENT_TIMEOUT_SECONDS = 5.0

EGO_BLUEPRINT = "vehicle.tesla.model3"
AMBULANCE_BLUEPRINT = "vehicle.ford.ambulance"
SPAWN_POINT_INDEX = 0
DISTANCE_METERS = 15.0
FIXED_DELTA_SECONDS = 0.05
MEASUREMENT_COUNT = 10


def main():
    """连接服务端、创建车辆与 sensor、接收测量，最后恢复运行环境。"""
    print("当前使用的 carla：", carla.__file__)
    client = carla.Client(HOST, PORT)
    client.set_timeout(CLIENT_TIMEOUT_SECONDS)
    world = client.get_world()

    # WorldSettings 管同步模式、时间步长、渲染和物理子步等运行选项。
    # 保存脚本启动前的设置，下面再取另一份修改，避免覆盖这份备份。
    # 结束时还原设置，避免脚本退出后，原本自动运行的世界仍在等待 tick。
    # 这里只备份运行设置，不是备份车辆状态，也不能让仿真时间倒退。
    original_settings = world.get_settings()
    actors = []
    sensor = None

    try:
        settings = world.get_settings()
        # 同步模式：服务端等待客户端调用 world.tick()，才推进下一步。
        # 这样可以先接收并处理一次数据，再让世界继续。
        settings.synchronous_mode = True
        # 每步增加 0.05 个仿真秒，即每个仿真秒有 20 个世界帧。
        # 计算这一帧花多少现实时间由机器负载决定，不一定也是 0.05 秒。
        settings.fixed_delta_seconds = FIXED_DELTA_SECONDS
        # 修改 settings 只是在修改本地对象，apply_settings 才应用到服务端。
        world.apply_settings(settings)

        # 同步模式下，只有本脚本调用 world.tick() 推进世界。
        # 不切换地图、不清理已有车辆，使用当前地图的一个出生点。
        blueprints = world.get_blueprint_library()
        ego_transform = world.get_map().get_spawn_points()[SPAWN_POINT_INDEX]
        ego = world.spawn_actor(blueprints.find(EGO_BLUEPRINT), ego_transform)
        actors.append(ego)
        # 关闭这辆车的物理模拟，让它保持摆放位置，不因重力、悬挂等移动。
        # 这不是踩刹车；后续正常驾驶场景需要采用相应的车辆控制方式。
        ego.set_simulate_physics(False)

        # forward 只描述车头方向，长度为 1，不是车辆的位置。
        # 例如 ego 在 (100, 50, 0)，朝向 x 正方向，则 forward = (1, 0, 0)。
        # 向后 15 米的位置就是 (100, 50, 0) - (15, 0, 0) = (85, 50, 0)。
        # 使用方向向量后，无论车头朝哪里，都能按同一公式算出后方位置。
        # 这是静态测距摆放，不代表已经实现同车道寻路或动态场景。
        forward = ego_transform.get_forward_vector()
        ambulance_location = ego_transform.location - forward * DISTANCE_METERS
        # Transform 包含位置和朝向：救护车位于上面算出的后方位置，
        # rotation 使用 ego 的朝向，使两车朝同一个方向；spawn_actor 才真正创建车。
        ambulance_transform = carla.Transform(
            ambulance_location, ego_transform.rotation
        )
        ambulance = world.spawn_actor(
            blueprints.find(AMBULANCE_BLUEPRINT), ambulance_transform
        )
        actors.append(ambulance)
        ambulance.set_simulate_physics(False)

        # sensor 的 owner 是 ego，target_actor_id 是被测量的救护车。
        # 距离由服务端计算，Python 不另算一个距离来替代测量值。
        sensor_blueprint = blueprints.find("sensor.other.audio_source")
        sensor_blueprint.set_attribute("target_actor_id", str(ambulance.id))
        # sensor_tick 是采样间隔，单位是仿真秒；0.0 请求每个世界更新步采样。
        # Blueprint 属性用字符串传入，再由服务端转成数值。
        # 目标无效等情况仍可能不发送测量，并非设置为 0 就保证每帧收到。
        sensor_blueprint.set_attribute("sensor_tick", "0.0")
        # attach_to=ego 时，Transform 表示相对 ego 的位置和朝向。
        # 空的 Transform 使用零偏移、零旋转：sensor 跟随 ego，位于其原点。
        # 当前自定义 sensor 按 ego 与救护车的 Actor 原点测距，不按安装偏移测距。
        sensor = world.spawn_actor(
            sensor_blueprint, carla.Transform(), attach_to=ego
        )
        actors.append(sensor)

        # Queue 是线程安全的队列，用来把异步到达的事件交给主循环。
        events = Queue()
        # listen 注册回调。每次收到该 sensor 的测量，CARLA 就调用 events.put(data)。
        # 这里传递 put 方法本身，不加括号；等收到数据后才由 CARLA 调用。
        # 服务端测距 → 客户端解码为 AudioSourceEvent → put 入队 → get 取出。
        sensor.listen(events.put)

        # 正常情况下循环 10 次，每次推进一步、取出一个事件并打印。
        for _ in range(MEASUREMENT_COUNT):
            # tick 请求服务端推进一帧并等待世界更新完成，会返回新帧号。
            # 参数 10 秒是现实时间的等待上限，不是本次要推进的仿真时长。
            # tick 完成也不保证 sensor 数据已经到达 Python，因此还要等待队列。
            world.tick(CLIENT_TIMEOUT_SECONDS)
            # 取出最早入队且尚未取走的事件；队列为空就等待，超过 5 秒报错。
            # event 就是上面的 put 收到的测量对象，不是 sensor 的配置对象。
            event = events.get(timeout=EVENT_TIMEOUT_SECONDS)

            # frame 是测量所属的世界仿真帧编号，不是客户端接收次数或图片编号。
            # timestamp 是采样时的仿真秒数，不是本机收到数据的时刻。
            # 两条测量若属于相邻世界帧，当前固定步长下时间戳应相差 0.05 秒。
            # distance_m 是两辆车 Actor 原点之间的三维直线距离，单位米。
            # 本脚本未比较 tick 返回值与 event.frame；正式多传感器采集需按帧对齐，
            # 不能把接收先后顺序当作采样顺序，也不能假定每个 sensor 每帧都有数据。
            print(
                f"frame={event.frame}  "
                f"timestamp={event.timestamp:.3f}s  "
                f"distance_m={event.distance_m:.3f}m"
            )

        print(f"已收到 {MEASUREMENT_COUNT} 次测量。")
    finally:
        # 正常结束、接收超时或 Ctrl+C 时，都进入这里收尾。
        # 只销毁本脚本创建的对象，逆序保证 sensor 先于车辆销毁。
        if sensor is not None and sensor.is_listening:
            sensor.stop()
        for actor in reversed(actors):
            actor.destroy()
        # 还原同步模式和步长等设置；销毁对象与恢复设置是两件不同的事。
        world.apply_settings(original_settings)
        print("已清理本次对象并恢复世界设置。")


if __name__ == "__main__":
    main()
