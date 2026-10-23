"""第 1 步：生成固定频率、固定振幅的正弦波，并保存为 WAV。"""

from scipy.io import wavfile
from pathlib import Path

import numpy as np


# 每秒保存的音频采样点数，单位 Hz；不是正弦波振动的频率。
SAMPLE_RATE = 48000
# 输出音频时长，单位秒。
DURATION_SECONDS = 10.0
# 正弦波每秒振动的次数，单位 Hz，对应公式中的 f。
FREQUENCY_HZ = 440.0
# 波形振幅，对应公式中的 A；0.75 表示峰值为数字满幅的 75%。
AMPLITUDE = 0.75

# 输出到脚本旁的 data/，与运行命令时所在的目录无关。
OUTPUT_PATH = Path(__file__).resolve().parent / "data/01_sine_wave.wav"


def main():
    """建立采样时间，计算正弦波，将浮点振幅编码成 PCM 后写入 WAV。"""
    total_samples = round(DURATION_SECONDS * SAMPLE_RATE)
    # arange 得到 0、1、2……479999；除以采样率，得到每个点的时间 t。
    # 时间范围为 [0, 10)，最后一个点在 10 秒之前，正好保存 480000 点。
    times = np.arange(total_samples) / SAMPLE_RATE
    # print(len(times), times[: 10])

    # s(t) = A × sin(2πft)。2πft 是相位，表示振动走到了哪一步。
    # np.sin 对整个时间数组逐个计算，audio 就是待保存的完整连续波形采样。
    audio = AMPLITUDE * np.sin(2 * np.pi * FREQUENCY_HZ * times)

    # 保存为wav
    wavfile.write(str(OUTPUT_PATH), SAMPLE_RATE, audio.astype(np.float32))


if __name__ == "__main__":
    main()
