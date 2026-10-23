"""第 4 步：先求传播对应的发声时间，再按传播路径长度衰减声音。"""

import wave
from pathlib import Path

import numpy as np


# 每秒音频采样点数，单位 Hz。
SAMPLE_RATE = 48000
# ego 接收声音的时间窗口长度，单位秒。
DURATION_SECONDS = 10.0
# 救护车发出的正弦波频率 f，单位 Hz。
FREQUENCY_HZ = 520.0
# 声源振幅 A，尚未乘距离衰减增益。
AMPLITUDE = 0.75
# 仿真时刻 0 的两车距离 d₀，单位米。
INITIAL_DISTANCE_METERS = 30.0
# 救护车朝静止 ego 匀速接近的速度 v，单位米/秒。
APPROACH_SPEED_MPS = 2.0
# 静止空气中的近似声速 c，单位米/秒。
SPEED_OF_SOUND_MPS = 343.0
# 参考距离，单位米；增益按距离倒数衰减，上限为 1。
REFERENCE_DISTANCE_METERS = 10.0
# 本例独立输出完整 WAV，不生成逐帧文件。
OUTPUT_PATH = Path(__file__).resolve().parent / "data/04_combined.wav"


def main():
    """结合第 3 步的发声时间和第 2 步的距离增益，生成接收波形。"""
    total_samples = round(DURATION_SECONDS * SAMPLE_RATE)
    receive_times = np.arange(total_samples) / SAMPLE_RATE

    # 第 3 步：由接收时间 t 反推发声时间 u，负时间延续录制前的声音。
    emission_times = (
        SPEED_OF_SOUND_MPS * receive_times - INITIAL_DISTANCE_METERS
    ) / (SPEED_OF_SOUND_MPS - APPROACH_SPEED_MPS)

    # 与第 2 步的区别：这里用发声时间 u 计算路径长度，而不是当前 t。
    # 当前听见的声音来自稍早、更远的位置，传播距离也应取那时的距离。
    path_distances = INITIAL_DISTANCE_METERS - APPROACH_SPEED_MPS * emission_times
    gain = REFERENCE_DISTANCE_METERS / np.maximum(
        path_distances, REFERENCE_DISTANCE_METERS
    )
    audio = AMPLITUDE * gain * np.sin(2 * np.pi * FREQUENCY_HZ * emission_times)

    # 默认振幅不超过 0.75，直接编码，不改变各步骤之间的音量比例。
    pcm = (audio * 32767).astype("<i2")
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUTPUT_PATH), "wb") as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(SAMPLE_RATE)
        wav_file.writeframes(pcm.tobytes())

    print(f"已保存：{OUTPUT_PATH}（{total_samples} 点，{DURATION_SECONDS} 秒）")


if __name__ == "__main__":
    main()
