"""第 3 步：只模拟运动中的传播时间，观察时移和多普勒，保持振幅固定。"""

import wave
from pathlib import Path

import numpy as np


# 每秒音频采样点数，单位 Hz。
SAMPLE_RATE = 48000
# ego 接收声音的时间窗口长度，单位秒。
DURATION_SECONDS = 10.0
# 救护车发出的正弦波频率 f，单位 Hz；接收频率由传播过程决定。
FREQUENCY_HZ = 440.0
# 固定振幅 A；本例不叠加距离衰减。
AMPLITUDE = 0.75

# 仿真时刻 0 的两车距离 d₀，单位米。
INITIAL_DISTANCE_METERS = 220.0
# 救护车朝静止 ego 匀速接近的速度 v，单位米/秒。
APPROACH_SPEED_MPS = 20.0
# 静止空气中的近似声速 c，单位米/秒。
SPEED_OF_SOUND_MPS = 343.0

OUTPUT_PATH = Path(__file__).resolve().parent / "data/03_sound_propagation.wav"


def main():
    """对每个接收时刻 t 求发声时刻 u，再计算声源在 u 时刻的振动。"""
    total_samples = round(DURATION_SECONDS * SAMPLE_RATE)
    receive_times = np.arange(total_samples) / SAMPLE_RATE

    # 声音在 u 时刻发出，此时路径长 d(u) = d₀ - vu。
    # 到达时刻 t = u + d(u)/c，整理得到 u = (ct - d₀)/(c - v)。
    # ego 静止，救护车始终在后方接近；录制前也延续相同的匀速运动。
    emission_times = (
        SPEED_OF_SOUND_MPS * receive_times - INITIAL_DISTANCE_METERS
    ) / (SPEED_OF_SOUND_MPS - APPROACH_SPEED_MPS)

    # 开头的 u 约为 -0.088 秒：听到的是录制开始之前已发出的声音。
    # 正弦波可以直接计算负时间，表示声源早已持续发声，不添加开头静音。
    # u 随 t 变化得更快，接收频率自然成为 f*c/(c-v)，不再额外变调。
    audio = AMPLITUDE * np.sin(2 * np.pi * FREQUENCY_HZ * emission_times)

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
