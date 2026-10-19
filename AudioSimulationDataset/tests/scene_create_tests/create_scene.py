"""#8：ego 静止，救护车从后方匀速接近，保存声源 sensor 的测量。

基于 #10 的静态接收脚本，只增加逐帧移动、帧号核对和 JSON 保存。
运行前启动本项目构建的 CARLA 服务端；使用 UE 编辑器时先点击 Play。
"""

import json
from datetime import datetime
from pathlib import Path
from queue import Queue

import carla


# 独立验证脚本仍将参数放在顶部，暂不引入正式生成器的配置读取。
# CARLA 服务端地址；127.0.0.1 表示脚本与服务端运行在同一台机器。
HOST = "127.0.0.1"
# CARLA 服务端的 RPC 端口，需与启动服务端时使用的端口一致。
PORT = 2000
# 客户端调用及下方 world.tick() 的等待上限，单位为现实秒。
CLIENT_TIMEOUT_SECONDS = 10.0
# tick 后等待 sensor 事件入队的上限，单位为现实秒；超时直接报错。
EVENT_TIMEOUT_SECONDS = 5.0

# 静止采集车的 Blueprint ID，用于从蓝图库选择车辆类型。
EGO_BLUEPRINT = "vehicle.tesla.model3"
# 被 sensor 测距的救护车类型，两车朝向相同，救护车从后方接近。
AMBULANCE_BLUEPRINT = "vehicle.ford.ambulance"
# 当前地图出生点列表的索引，从 0 开始；需手动选择合适的直路位置。
SPAWN_POINT_INDEX = 0
# 救护车初始位于 ego 后方的距离，单位米，按两车 Actor 原点计算。
INITIAL_DISTANCE_METERS = 30.0
# 接近速度，单位米/仿真秒；用于逐帧算位置，不是设置车辆物理速度。
APPROACH_SPEED_MPS = 2.0
# 首条与末条测量之间的仿真时长，单位秒，不是脚本的现实运行时长。
DURATION_SECONDS = 10.0
# 每次 tick 推进的仿真秒数；0.05 秒对应每仿真秒 20 帧。
FIXED_DELTA_SECONDS = 0.05

# 输出相对脚本定位，从其他工作目录启动也会保存在这里。
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "data"


def main():
    """创建场景、逐帧采集并保存，最后销毁本次对象、恢复世界设置。"""
    run_name = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    print("当前使用的 carla：", carla.__file__)
    client = carla.Client(HOST, PORT)
    client.set_timeout(CLIENT_TIMEOUT_SECONDS)
    world = client.get_world()

    # 单独保留原始设置。结束时恢复运行方式，不会让仿真时间倒退。
    original_settings = world.get_settings()
    actors = []
    sensor = None

    try:
        settings = world.get_settings()
        # 只由本脚本 tick 推进世界；每帧增加 0.05 个仿真秒。
        settings.synchronous_mode = True
        settings.fixed_delta_seconds = FIXED_DELTA_SECONDS
        world.apply_settings(settings)

        blueprints = world.get_blueprint_library()
        ego_transform = world.get_map().get_spawn_points()[SPAWN_POINT_INDEX]
        ego = world.spawn_actor(blueprints.find(EGO_BLUEPRINT), ego_transform)
        actors.append(ego)
        ego.set_simulate_physics(False)

        # forward 是车头方向的单位向量，不是位置或速度。
        # 从 ego 位置减去 forward × 距离，得到同一直线上后方的位置。
        # 请手动选择有足够长度的直路出生点；这里不搜索道路或判断车道。
        forward = ego_transform.get_forward_vector()
        ambulance_location = (
            ego_transform.location - forward * INITIAL_DISTANCE_METERS
        )
        ambulance_transform = carla.Transform(
            ambulance_location, ego_transform.rotation
        )
        ambulance = world.spawn_actor(
            blueprints.find(AMBULANCE_BLUEPRINT), ambulance_transform
        )
        actors.append(ambulance)
        # 关闭物理后可以直接摆放车辆，不受重力、摩擦或碰撞响应影响。
        # 本次用位置控制验证线性测距，不模拟油门、刹车或真实驾驶。
        ambulance.set_simulate_physics(False)

        sensor_blueprint = blueprints.find("sensor.other.audio_source")
        sensor_blueprint.set_attribute("target_actor_id", str(ambulance.id))
        sensor_blueprint.set_attribute("sensor_tick", "0.0")
        # 空 Transform 表示相对 ego 零偏移、零旋转；owner 为 ego。
        # 当前 sensor 测量两辆车 Actor 原点的三维直线距离，单位米。
        sensor = world.spawn_actor(
            sensor_blueprint, carla.Transform(), attach_to=ego
        )
        actors.append(sensor)

        events = Queue()
        # 服务端测量经客户端解码后，通过回调入队，再由下面的主循环取出。
        sensor.listen(events.put)

        # 10 / 0.05 = 200 次移动；另加初始位置，共 201 次测量。
        # 修改时长时请保持它是步长的整数倍，并确保最终距离仍安全为正。
        step_count = round(DURATION_SECONDS / FIXED_DELTA_SECONDS)
        measurements = []
        for step in range(step_count + 1):
            # step=0 采集初始距离；以后每步接近 2 × 0.05 = 0.1 米。
            # 从初始距离直接计算，避免反复减去小数造成位置累加误差。
            elapsed_seconds = step * FIXED_DELTA_SECONDS
            planned_distance = (
                INITIAL_DISTANCE_METERS - APPROACH_SPEED_MPS * elapsed_seconds
            )
            ambulance_location = ego_transform.location - forward * planned_distance
            # set_location 只改位置，保留救护车的朝向；随后 tick 才采样。
            ambulance.set_location(ambulance_location)

            frame = world.tick(CLIENT_TIMEOUT_SECONDS)
            event = events.get(timeout=EVENT_TIMEOUT_SECONDS)
            # tick 完成不代表回调已到达；上面的 get 会等事件，不要求立即到达。
            # 单个 event 只属于一帧；队列才可能积累多帧，每次 get 只取一个。
            # 本脚本收到当前帧后才 tick 下一帧，因此正常接收不会积压后续帧。
            # 帧号不符时停止，避免把旧帧当成本次移动的测量；不做缓冲或补帧。
            if event.frame != frame:
                raise RuntimeError(
                    f"帧号不一致：world.tick={frame}，sensor.frame={event.frame}"
                )

            measurements.append(
                {
                    "frame": event.frame,
                    "timestamp": event.timestamp,
                    "distance_m": event.distance_m,
                }
            )
            print(
                f"frame={event.frame}  "
                f"timestamp={event.timestamp:.3f}s  "
                f"distance_m={event.distance_m:.3f}m"
            )

        # 只在完整接收后保存，JSON 仅保留 measurements 原始测量。
        # 时间戳不归零、不四舍五入；#11 可减去首条时间戳得到音频相对时间。
        data = {
            "measurements": measurements,
        }
        OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)
        output_path = OUTPUT_DIRECTORY / f"{run_name}.json"

        # x 模式在极少数文件名重复时直接报错，不覆盖已有记录。
        with output_path.open("x", encoding="utf-8") as output_file:
            json.dump(data, output_file, ensure_ascii=False, indent=2)
            output_file.write("\n")
        print(f"已保存 {len(measurements)} 次测量：{output_path}")
    finally:
        # 正常完成、超时、帧号不符或 Ctrl+C 均进入这里。
        # 只清理本次创建的对象，逆序保证 sensor 先于车辆销毁。
        if sensor is not None and sensor.is_listening:
            sensor.stop()
        for actor in reversed(actors):
            actor.destroy()
        world.apply_settings(original_settings)
        print("已清理本次对象并恢复世界设置。")


if __name__ == "__main__":
    main()
