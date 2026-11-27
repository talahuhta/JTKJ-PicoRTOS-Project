import pandas as pd
import matplotlib.pyplot as plt

cols = ["time_ms", "ax", "ay", "az", "gx", "gy", "gz"]

df = pd.read_csv("imudata.csv", header=None, names=cols, on_bad_lines="skip")
df = df.apply(pd.to_numeric, errors="coerce").dropna()

df.plot(x="time_ms", y=["ax", "ay", "az"], title="Accelerometer (g)")
plt.xlabel("Time (ms)")
plt.ylabel("Acceleration (g)")
plt.grid(True)
plt.show()

# Plotting program made by Gemini
# https://share.gemini.google/XYGZKubHd6wn