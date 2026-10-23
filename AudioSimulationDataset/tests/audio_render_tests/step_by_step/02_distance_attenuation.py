"""第 2 步：暂不考虑传播时间，只让正弦波随距离减小而增大。"""

from scipy.io import wavfile
from pathlib import Path

import numpy as np


# 每秒音频采样点数，单位 Hz。
SAMPLE_RATE = 48000
# 输出音频时长，单位秒。
DURATION_SECONDS = 10.0
# 正弦波频率 f，单位 Hz；本例不改变音调。
FREQUENCY_HZ = 440.0
# 声源振幅 A；后面还要乘随距离变化的增益。
AMPLITUDE = 0.75

# t = 0 时救护车到静止 ego 的距离 d₀，单位米。
INITIAL_DISTANCE_METERS = 220.0
# 救护车朝 ego 接近的速度 v，单位米/秒，正数表示靠近。
APPROACH_SPEED_MPS = 20.0
# 参考距离，单位米；超过它时按距离倒数衰减，小于它时增益保持 1。
REFERENCE_DISTANCE_METERS = 10.0

# 本例独立输出，不覆盖其他步骤的 WAV。
OUTPUT_PATH = Path(__file__).resolve().parent / "data/02_distance_attenuation.wav"


def main():
    """先计算当前距离，再把对应的振幅增益乘到正弦波上。"""
    total_samples = round(DURATION_SECONDS * SAMPLE_RATE)
    times = np.arange(total_samples) / SAMPLE_RATE

    # d(t) = d₀ - vt：10 秒内由 30 米接近到 10 米。
    # 本步刻意忽略传播时间，直接使用接收时刻 t 的距离。
    distances = INITIAL_DISTANCE_METERS - APPROACH_SPEED_MPS * times
    # print(distances[: 30])

    # g(d) = reference_distance / max(d, reference_distance)。
    # 振幅按 1/d 衰减：30 米增益 1/3，20 米增益 1/2，10 米增益 1。
    gain = REFERENCE_DISTANCE_METERS / np.maximum(
        distances, REFERENCE_DISTANCE_METERS
    )
    audio = AMPLITUDE * gain * np.sin(2 * np.pi * FREQUENCY_HZ * times)

    wavfile.write(str(OUTPUT_PATH), SAMPLE_RATE, audio.astype(np.float32))


if __name__ == "__main__":
    main()
