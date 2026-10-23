"""读取 #8 的距离记录，离线生成连续警笛及帧音频区间索引。"""

import json
import wave
from pathlib import Path

import numpy as np


# 输入为 #8 实测 JSON 的原样副本；相对脚本定位，与启动目录无关。
INPUT_PATH = Path(__file__).resolve().parent / "data/raw/20261019_133736_286515.json"
# 派生结果与原始记录分开保存；重复运行覆盖同名 WAV 和索引。
OUTPUT_DIRECTORY = Path(__file__).resolve().parent / "data/derived"

# 每个音频秒的采样点数，单位 Hz；与 CARLA 的世界帧率不同。
SAMPLE_RATE = 48000
# 静止空气中的近似声速，单位米/秒；本轮不模拟温湿度变化。
SPEED_OF_SOUND_MPS = 343.0
# 双音警笛的低音频率，单位 Hz。
LOW_TONE_HZ = 520.0
# 双音警笛的高音频率，单位 Hz。
HIGH_TONE_HZ = 760.0
# 每个音持续的仿真秒数，低音、高音各持续一次构成完整周期。
TONE_DURATION_SECONDS = 0.55
# 声源振幅系数，无单位；不对每帧单独归一化，保留音量变化。
SOURCE_LEVEL = 0.75
# 参考距离，单位米；更远时振幅按距离倒数衰减，更近时增益保持 1。
REFERENCE_DISTANCE_METERS = 10.0


def calculate_propagation(measurement_times, distances, receive_times):
    """把接收时刻映射到发声时刻，返回发声时间和对应传播路径长度。

    输入时间均为相对首条测量的仿真秒数，距离单位为米。
    限定麦克风静止、声源沿直线运动且速度低于声速，距离保持为正。
    测量点之间按直线插值，因而到达时间之间也可以线性反插值。
    """
    # 当前 JSON 没有录制前的轨迹。仅为开头所需的短暂历史，
    # 使用前两条测量的距离变化率向前外推；不是补充 sensor 真值。
    initial_distance_rate = (distances[1] - distances[0]) / (
        measurement_times[1] - measurement_times[0]
    )

    # t_receive = t_emit + distance(t_emit) / c。
    # 令首个接收时刻为 0，并代入初始直线距离，解出最早所需发声时刻。
    first_emission_time = -distances[0] / (
        SPEED_OF_SOUND_MPS + initial_distance_rate
    )
    first_distance = distances[0] + initial_distance_rate * first_emission_time
    source_times = np.concatenate(([first_emission_time], measurement_times))
    source_distances = np.concatenate(([first_distance], distances))

    # 麦克风不动，所以路径长度就是发声时刻声源到麦克风的距离。
    # 随运动变化的传播时间会自然改变波峰间隔，产生多普勒；不再额外变调。
    arrival_times = source_times + source_distances / SPEED_OF_SOUND_MPS
    emission_times = np.interp(receive_times, arrival_times, source_times)
    path_distances = np.interp(emission_times, source_times, source_distances)
    return emission_times, path_distances


def generate_siren(emission_times):
    """按全局发声时间生成振幅为 1 的双音警笛，支持录制前的负时间。

    相位由从时间 0 到当前时刻的累计振动次数计算，频率切换时不重置。
    使用 floor 划分周期，使负时间也延续同一警笛，而不是从录制点重启。
    """
    cycle_duration = 2 * TONE_DURATION_SECONDS
    cycle_numbers = np.floor(emission_times / cycle_duration)
    time_in_cycle = emission_times - cycle_numbers * cycle_duration

    # 完整周期的振动次数，加上本周期低音和高音各自已完成的次数。
    phase_cycles = cycle_numbers * (
        (LOW_TONE_HZ + HIGH_TONE_HZ) * TONE_DURATION_SECONDS
    )
    phase_cycles += LOW_TONE_HZ * np.minimum(time_in_cycle, TONE_DURATION_SECONDS)
    phase_cycles += HIGH_TONE_HZ * np.maximum(
        time_in_cycle - TONE_DURATION_SECONDS, 0.0
    )
    return np.sin(2 * np.pi * phase_cycles)


def main():
    """生成完整波形，再建立帧区间索引，最后保存两份派生文件。"""
    data = json.loads(INPUT_PATH.read_text(encoding="utf-8"))
    measurements = data["measurements"]
    timestamps = np.array([row["timestamp"] for row in measurements])
    distances = np.array([row["distance_m"] for row in measurements])
    measurement_times = timestamps - timestamps[0]

    # 音频时间表示麦克风接收声音的时刻，覆盖 [0, 最后测量时刻)。
    # 当前 201 个测量点围出 200 个区间，约 10 秒，不额外延长最后一帧。
    total_samples = round(measurement_times[-1] * SAMPLE_RATE)
    receive_times = np.arange(total_samples) / SAMPLE_RATE
    emission_times, path_distances = calculate_propagation(
        measurement_times, distances, receive_times
    )
    gain = REFERENCE_DISTANCE_METERS / np.maximum(
        path_distances, REFERENCE_DISTANCE_METERS
    )
    audio = SOURCE_LEVEL * gain * generate_siren(emission_times)

    # 从统一原点计算所有边界，相邻片段共用同一个采样索引。
    # 索引是接收时间区间，不是警笛发声时间；最后一条测量只提供终点。
    sample_boundaries = np.rint(measurement_times * SAMPLE_RATE).astype(int)
    intervals = []
    for i in range(len(measurements) - 1):
        intervals.append(
            {
                "frame": measurements[i]["frame"],
                "next_frame": measurements[i + 1]["frame"],
                "start_sample": int(sample_boundaries[i]),
                "end_sample": int(sample_boundaries[i + 1]),
            }
        )
    frame_index = {
        "sample_rate": SAMPLE_RATE,
        "total_samples": total_samples,
        "intervals": intervals,
    }

    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)
    wav_path = OUTPUT_DIRECTORY / f"{INPUT_PATH.stem}.wav"
    index_path = OUTPUT_DIRECTORY / f"{INPUT_PATH.stem}_frame_index.json"

    # gain <= 1、SOURCE_LEVEL = 0.75，默认波形不会超出 PCM 的振幅范围。
    # 单声道、16-bit 小端有符号整数；不额外缩放整段或单个片段的音量。
    pcm = (audio * 32767).astype("<i2")
    with wave.open(str(wav_path), "wb") as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(SAMPLE_RATE)
        wav_file.writeframes(pcm.tobytes())
    index_path.write_text(
        json.dumps(frame_index, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )

    print(f"已生成 {total_samples} 个采样点，时长 {total_samples / SAMPLE_RATE:.3f} 秒。")
    print(f"WAV：{wav_path}")
    print(f"帧区间索引：{index_path}（{len(intervals)} 个区间）")


if __name__ == "__main__":
    main()
